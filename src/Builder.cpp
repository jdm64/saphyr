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
#include "Function.h"
#include "Builder.h"
#include "CodeContext.h"

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
	auto func = Builder::CreateFunction(context, &fnToken, rtype, params, body);
	if (func)
		clType->addFunction(name->str, func);
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
		typeList.add(item->getTypeNode());
	}
	return getFuncType(context, name, rtype, &typeList);
}

SFunctionType* Builder::getFuncType(CodeContext& context, Token* name, NDataType* retType, NDataTypeList* params)
{
	bool valid = true;
	vector<SType*> args;
	for (auto item : *params) {
		auto param = item->getType(context);
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

	auto returnType = retType->getType(context);
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
	auto stype = group->getType()->getType(context);
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
