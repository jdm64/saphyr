/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2016, Justin Madru (justin.jdm64@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AST.h"
#include "parserbase.h"
#include "Function.h"
#include "Builder.h"
#include "CodeContext.h"
#include "Instructions.h"
#include "CGNDataType.h"

SFunction Builder::CreateFunction(CodeContext& context, Token* name, NDataType* rtype, NParameterList* params, NStatementList* body)
{
	auto function = lookupFunction(context, name, rtype, params);
	if (!function || !body) { // no body means only function prototype
		return function;
	} else if (function.size()) {
		context.addError("function " + name->str + " already declared", name);
		return function;
	}

	if (body->empty() || !body->back()->isTerminator()) {
		auto returnType = function.returnTy();
		if (returnType->isVoid())
			body->add(new NReturnStatement);
		else
			context.addError("no return for a non-void function", name);
	}

	if (getFuncType(context, name, rtype, params) != function.stype()) {
		context.addError("function type for " + name->str + " doesn't match definition", name);
		return function;
	}

	context.startFuncBlock(function);

	int i = 0;
	set<string> names;
	for (auto arg = function.arg_begin(); arg != function.arg_end(); arg++, i++) {
		auto param = params->at(i);
		auto name = param->getName();
		if (names.insert(name).second)
			arg->setName(name);
		else
			context.addError("function parameter " + name + " already declared", param->getNameToken());
		param->setArgument(RValue(&*arg, function.getParam(i)));
		param->genCode(context);
	}

	body->genCode(context);
	context.endFuncBlock();
	return function;
}

void Builder::CreateClassFunction(CodeContext& context, Token* name, NClassDeclaration* theClass, NDataType* rtype, NParameterList* params, NStatementList* body)
{
	auto clType = context.getClass();
	auto item = clType->getItem(name->str);
	if (item) {
		context.addError("class " + theClass->getName() + " already defines symbol " + name->str, name);
		return;
	}

	// add this parameter
	auto thisToken = new Token(*theClass->getNameToken());
	auto thisPtr = new NParameter(new NPointerType(new NUserType(thisToken)), new Token("this"));
	params->addFront(thisPtr);

	auto fnToken = *name;
	fnToken.str = theClass->getName() + "_" + name->str;

	// add function to class type
	auto func = CreateFunction(context, &fnToken, rtype, params, body);
	if (func)
		clType->addFunction(name->str, func);
}

void Builder::CreateClassConstructor(CodeContext& context, NClassConstructor* stm)
{
	map<string,NMemberInitializer*> items;
	for (auto item : *stm->getInitList()) {
		auto token = item->getNameToken();
		auto it = items.find(token->str);
		if (it != items.end()) {
			context.addError("initializer for " + token->str + " already defined", token);
			continue;
		}
		items.insert({token->str, item});
	}
	stm->getInitList()->clear();

	auto classTy = context.getClass();
	for (auto item : *classTy) {
		auto stype = item.second.second.stype();
		if (!stype->isClass())
			continue;
		auto it = items.find(item.first);
		if (it != items.end())
			continue;
		auto clTy = static_cast<SClassType*>(stype);
		if (clTy->getItem("this")) {
			items.insert({item.first, new NMemberInitializer(new Token(item.first), new NExpressionList)});
		}
	}

	if (!items.empty()) {
		auto newBody = new NStatementList;
		newBody->reserve(items.size() + stm->getBody()->size());
		for (auto item : items)
			newBody->add(item.second);
		newBody->addAll(*stm->getBody());
		stm->getBody()->setDelete(false);
		delete stm->getBody();
		stm->setBody(newBody);
	}

	if (stm->getBody()->empty())
		return;

	NBaseType voidType(nullptr, ParserBase::TT_VOID);
	CreateClassFunction(context, stm->getNameToken(), stm->getClass(), &voidType, stm->getParams(), stm->getBody());
}

void Builder::CreateClassDestructor(CodeContext& context, NClassDestructor* stm)
{
	auto clType = context.getClass();
	for (auto item : *clType) {
		auto ty = item.second.second.stype();
		if (!ty->isClass())
			continue;
		auto itemCl = static_cast<SClassType*>(ty);
		if (!itemCl->getItem("null"))
			continue;
		stm->getBody()->add(new NDestructorCall(new NBaseVariable(new Token(item.first)), nullptr));
	}

	if (stm->getBody()->empty())
		return;

	NBaseType voidType(nullptr, ParserBase::TT_VOID);
	NParameterList params;

	auto nullTok = *stm->getNameToken();
	nullTok.str = "null";

	CreateClassFunction(context, &nullTok, stm->getClass(), &voidType, &params, stm->getBody());
}

SFunction Builder::lookupFunction(CodeContext& context, Token* name, NDataType* rtype, NParameterList* params)
{
	auto funcName = name->str;
	auto sym = context.loadSymbolGlobal(funcName);
	if (sym) {
		if (sym.isFunction())
			return static_cast<SFunction&>(sym);
		context.addError("variable " + funcName + " already defined", name);
		return SFunction();
	}
	auto funcType = getFuncType(context, name, rtype, params);
	return funcType? SFunction::create(context, funcName, funcType) : SFunction();
}

SFunctionType* Builder::getFuncType(CodeContext& context, Token* name, NDataType* rtype, NParameterList* params)
{
	NDataTypeList typeList(false);
	for (auto item : *params) {
		typeList.add(item->getType());
	}
	return getFuncType(context, name, rtype, &typeList);
}

SFunctionType* Builder::getFuncType(CodeContext& context, Token* name, NDataType* retType, NDataTypeList* params)
{
	bool valid = true;
	vector<SType*> args;
	for (auto item : *params) {
		auto param = CGNDataType::run(context, item);
		if (!param) {
			valid = false;
		} else if (param->isAuto()) {
			auto token = static_cast<NNamedType*>(item)->getToken();
			context.addError("parameter can not be auto type", token);
			valid = false;
		} else if (SType::validate(context, name, param)) {
			args.push_back(param);
		}
	}

	auto returnType = CGNDataType::run(context, retType);
	if (!returnType) {
		return nullptr;
	} else if (returnType->isAuto()) {
		auto token = static_cast<NNamedType*>(retType)->getToken();
		context.addError("function return type can not be auto", token);
		return nullptr;
	}

	return (returnType && valid)? SType::getFunction(context, returnType, args) : nullptr;
}

bool Builder::addMembers(NVariableDeclGroup* group, vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context)
{
	auto stype = CGNDataType::run(context, group->getType());
	if (!stype) {
		return false;
	} else if (stype->isAuto()) {
		auto token = static_cast<NNamedType*>(group->getType())->getToken();
		context.addError("struct members must not have auto type", token);
		return false;
	}
	bool valid = true;
	for (auto var : *group->getVars()) {
		if (var->hasInit())
			context.addError("structs don't support variable initialization", var->getEqToken());
		auto name = var->getName();
		auto res = memberNames.insert(name);
		if (!res.second) {
			auto token = static_cast<NDeclaration*>(var)->getNameToken();
			context.addError("member name " + name + " already declared", token);
			valid = false;
			continue;
		}
		structVector.push_back(make_pair(name, stype));
	}
	return valid;
}

void Builder::CreateStruct(CodeContext& context, NStructDeclaration::CreateType ctype, Token* name, NVariableDeclGroupList* list)
{
	auto structName = name->str;
	auto utype = SUserType::lookup(context, structName);
	if (utype) {
		context.addError(structName + " type already declared", name);
		return;
	}
	vector<pair<string, SType*> > structVars;
	set<string> memberNames;
	bool valid = true;
	for (auto item : *list)
		valid &= addMembers(item, structVars, memberNames, context);
	if (valid) {
		switch (ctype) {
		case NStructDeclaration::CreateType::STRUCT:
			SUserType::createStruct(context, structName, structVars);
			return;
		case NStructDeclaration::CreateType::UNION:
			SUserType::createUnion(context, structName, structVars);
			return;
		case NStructDeclaration::CreateType::CLASS:
			SUserType::createClass(context, structName, structVars);
			return;
		}
	}
}

void Builder::CreateEnum(CodeContext& context, NEnumDeclaration* stm)
{
	int64_t val = 0;
	set<string> names;
	vector<pair<string,int64_t>> structure;

	for (auto item : *stm->getVarList()) {
		auto name = item->getName();
		auto res = names.insert(name);
		if (!res.second) {
			auto token = static_cast<NDeclaration*>(item)->getNameToken();
			context.addError("enum member name " + name + " already declared", token);
			continue;
		}
		if (item->hasInit()) {
			auto initExp = item->getInitExp();
			if (!initExp->isConstant()) {
				context.addError("enum initializer must be a constant", item->getEqToken());
				continue;
			}
			auto constVal = static_cast<NConstant*>(initExp);
			if (!constVal->isIntConst()) {
				context.addError("enum initializer must be an int-like constant", constVal->getToken());
				continue;
			}
			auto intVal = static_cast<NIntLikeConst*>(constVal);
			val = intVal->getIntVal(context).getSExtValue();
		}
		structure.push_back(make_pair(name, val++));
	}

	auto etype = stm->getBaseType()? CGNDataType::run(context, stm->getBaseType()) : SType::getInt(context, 32);
	if (!etype || !etype->isInteger() || etype->isBool()) {
		context.addError("enum base type must be an integer type", stm->getLBrac());
		return;
	}

	SUserType::createEnum(context, stm->getName(), structure, etype);
}

void Builder::CreateAlias(CodeContext& context, NAliasDeclaration* stm)
{
	auto realType = CGNDataType::run(context, stm->getType());
	if (!realType) {
		return;
	} else if (realType->isAuto()) {
		auto token = static_cast<NNamedType*>(stm->getType())->getToken();
		context.addError("can not create alias to auto type", token);
		return;
	}

	SAliasType::createAlias(context, stm->getName(), realType);
}

void Builder::CreateGlobalVar(CodeContext& context, NGlobalVariableDecl* stm)
{
	if (stm->getInitExp() && !stm->getInitExp()->isConstant()) {
		context.addError("global variables only support constant value initializer", stm->getEqToken());
		return;
	}
	auto initValue = stm->getInitExp()? stm->getInitExp()->genValue(context) : RValue();
	auto varType = CGNDataType::run(context, stm->getType());

	if (!varType) {
		return;
	} else if (varType->isAuto()) {
		if (!stm->getInitExp()) { // auto type requires initialization
			auto token = static_cast<NNamedType*>(stm->getType())->getToken();
			context.addError("auto variable type requires initialization", token);
			return;
		} else if (!initValue) {
			return;
		}
		varType = initValue.stype();
	} else if (!SType::validate(context, stm->getNameToken(), varType)) {
		return;
	}

	if (initValue) {
		if (initValue.isNullPtr()) {
			Inst::CastTo(context, stm->getEqToken(), initValue, varType);
		} else if (varType != initValue.stype()) {
			context.addError("global variable initialization requires exact type matching", stm->getEqToken());
			return;
		}
	}

	auto name = stm->getName();
	if (context.loadSymbolCurr(name)) {
		context.addError("variable " + name + " already defined", stm->getNameToken());
		return;
	}

	auto var = new GlobalVariable(*context.getModule(), *varType, false, GlobalValue::ExternalLinkage, (Constant*) initValue.value(), name);
	context.storeGlobalSymbol({var, varType}, name);
}
