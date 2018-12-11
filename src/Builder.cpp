/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2017, Justin Madru (justin.jdm64@gmail.com)
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
#include "parser.h"
#include "Value.h"
#include "Builder.h"
#include "CodeContext.h"
#include "Instructions.h"
#include "CGNDataType.h"
#include "CGNExpression.h"
#include "CGNStatement.h"
#include "CGNImportStm.h"
#include "Instructions.h"
#include "Util.h"

SFunction Builder::CreateFunction(CodeContext& context, Token* name, NDataType* rtype, NParameterList* params, NStatementList* body, NAttributeList* attrs)
{
	auto funcType = getFuncType(context, rtype, params);
	if (!funcType)
		return SFunction();
	auto function = getFuncPrototype(context, name, funcType, attrs);
	if (!function || !body) {
		// no body means only function prototype
		return function;
	} else if (function.size()) {
		context.addError("function " + name->str + " already declared", name);
		return SFunction();
	}

	if (body->empty() || !body->back()->isTerminator()) {
		auto returnType = function.returnTy();
		if (returnType->isVoid())
			body->add(new NReturnStatement);
		else
			context.addError("no return for a non-void function", name);
	}

	context.startFuncBlock(function);

	int i = 0;
	set<string> names;
	for (auto arg = function.arg_begin(); arg != function.arg_end(); arg++, i++) {
		auto param = params->at(i);
		auto name = param->getName()->str;
		if (names.insert(name).second)
			arg->setName(name);
		else
			context.addError("function parameter " + name + " already declared", param->getName());
		CGNStatement visitor(context);
		visitor.storeValue(RValue(&*arg, function.getParam(i)));
		visitor.visit(param);
	}

	CGNStatement::run(context, body);
	context.endFuncBlock();
	return function;
}

void Builder::CreateClassFunction(CodeContext& context, NClassFunctionDecl* stm, bool prototype)
{
	validateAttrList(context, stm->getAttrs());

	auto name = stm->getName();
	auto clType = context.getClass();
	auto item = clType->getItem(name->str);
	if (item) {
		context.addError("class " + clType->str(&context) + " already defines symbol " + name->str, name);
		return;
	}

	auto rawClName = Token(*stm->getClass()->getName());
	rawClName.str = SUserType::raw(rawClName.str, context.getTemplateArgs());

	// add this parameter
	if (!NAttributeList::find(stm->getAttrs(), "static")) {
		auto thisToken = new Token(rawClName);
		auto thisPtr = new NParameter(new NPointerType(new NUserType(thisToken)), new Token("this"));
		stm->getParams()->addFront(thisPtr);
	}

	auto fnToken = *name;
	fnToken.str = rawClName.str + "_" + name->str;

	// add function to class type
	auto func = CreateFunction(context, &fnToken, stm->getRType(), stm->getParams(), prototype? nullptr : stm->getBody(), stm->getAttrs());
	if (func)
		clType->addFunction(name->str, func);
}

void Builder::CreateClassConstructor(CodeContext& context, NClassConstructor* stm, bool prototype)
{
	map<string,NMemberInitializer*> items;
	for (auto item : *stm->getInitList()) {
		auto token = item->getName();
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
		auto it = items.find(item.first);
		if (it != items.end())
			continue;

		auto stype = item.second.second.stype();
		SClassType* clTy;

		if (stype->isClass())
			clTy = static_cast<SClassType*>(stype);
		else if (stype->isArray() && stype->subType()->isClass())
			clTy = static_cast<SClassType*>(stype->subType());
		else
			continue;

		if (clTy->getConstructor()) {
			items.insert({item.first, new NMemberInitializer(new Token(*stm->getName(), item.first), new NExpressionList)});
		}
	}

	if (!items.empty()) {
		auto newBody = new NStatementList;
		newBody->reserve(items.size() + stm->getBody()->size());
		for (auto item : items)
			newBody->add(item.second);
		newBody->addAll(*stm->getBody());
		stm->getBody()->setDelete(false);
		stm->setBody(newBody);
	}

	if (stm->getBody()->empty())
		return;

	stm->setRType(new NBaseType(nullptr, ParserBase::TT_VOID));
	CreateClassFunction(context, stm, prototype);
}

void Builder::CreateClassDestructor(CodeContext& context, NClassDestructor* stm, bool prototype)
{
	auto clType = context.getClass();
	for (auto item : *clType) {
		auto ty = item.second.second.stype();
		if (!ty->isClass())
			continue;
		auto itemCl = static_cast<SClassType*>(ty);
		if (!itemCl->getDestructor())
			continue;
		stm->getBody()->add(new NDestructorCall(new NBaseVariable(new Token(item.first)), nullptr));
	}

	if (stm->getBody()->empty())
		return;

	stm->setRType(new NBaseType(nullptr, ParserBase::TT_VOID));
	stm->getName()->str = "null";

	CreateClassFunction(context, stm, prototype);
}

void Builder::CreateClass(CodeContext& context, NClassDeclaration* stm, function<void(int)> visitor)
{
	int structIdx = -1;
	int constrIdx = -1;
	int destrtIdx = -1;
	if (!stm->getMembers())
		stm->setMembers(new NClassMemberList);
	auto members = stm->getMembers();
	auto size = members ? members->size() : 0;
	for (size_t i = 0; i < size; i++) {
		switch (members->at(i)->memberType()) {
		case NClassMember::MemberType::STRUCT:
			if (structIdx > -1)
				context.addError("only one struct allowed in a class", members->at(i)->getName());
			else
				structIdx = i;
			break;
		case NClassMember::MemberType::CONSTRUCTOR:
			if (constrIdx > -1)
				context.addError("only one constructor allowed in a class", members->at(i)->getName());
			else
				constrIdx = i;
			break;
		case NClassMember::MemberType::DESTRUCTOR:
			if (destrtIdx > -1)
				context.addError("only one destructor allowed in a class", members->at(i)->getName());
			else
				destrtIdx = i;
			break;
		default:
			break;
		}
	}

	if (structIdx < 0) {
		auto group = new NVariableDeclGroupList;
		auto varList = new NVariableDeclList;
		auto structDecl = new NClassStructDecl(nullptr, group);
		structDecl->setClass(stm);
		varList->add(new NVariableDecl(new Token));
		group->add(new NVariableDeclGroup(new NBaseType(nullptr, ParserBase::TT_INT8), varList));
		members->add(structDecl);
		structIdx = members->size() - 1;
	}
	if (constrIdx < 0) {
		auto constr = new NClassConstructor(new Token(*stm->getName(), "this"), new NParameterList, new NInitializerList, new NStatementList);
		constr->setClass(stm);
		members->add(constr);
	}
	if (destrtIdx < 0) {
		auto destr = new NClassDestructor(new Token, new NStatementList);
		destr->setClass(stm);
		members->add(destr);
	}
	visitor(structIdx);
	context.setClass(nullptr);
}

SFunction Builder::getFuncPrototype(CodeContext& context, Token* name, SFunctionType* funcType, NAttributeList* attrs)
{
	auto funcName = name->str;
	auto sym = context.loadSymbolGlobal(funcName);
	if (!sym) {
		string mangleName;
		auto mangle = NAttributeList::find(attrs, "mangle");
		if (mangle) {
			auto mangleVal = NAttrValueList::find(mangle->getValues(), 0);
			if (mangleVal) {
				mangleName = mangleVal->str();
				sym = context.loadSymbolGlobal(mangleName);
				if (sym) {
					context.addError("cannot mangle function to existing symbol", *mangleVal);
					mangleName = "";
				}
			} else {
				context.addError("mangle attribute requires value", *mangle);
			}
		}

		auto realName = mangleName.size()? mangleName : funcName;
		auto func = Function::Create(*funcType, GlobalValue::ExternalLinkage, realName, context.getModule());
		auto function = SFunction::create(context, func, funcType, attrs);
		context.storeGlobalSymbol(function, funcName);
		if (mangleName.size())
			context.storeGlobalSymbol(function, mangleName);
		return function;
	} else if (!sym.isFunction()) {
		context.addError("variable " + funcName + " already defined", name);
		return SFunction();
	}

	auto function = static_cast<SFunction&>(sym);
	if (funcType != function.stype()) {
		context.addError("function type for " + funcName + " doesn't match definition", name);
		return SFunction();
	}
	return function;
}

SFunctionType* Builder::getFuncType(CodeContext& context, NDataType* rtype, NParameterList* params)
{
	NDataTypeList typeList(false);
	for (auto item : *params) {
		typeList.add(item->getType());
	}
	return getFuncType(context, rtype, &typeList);
}

SFunctionType* Builder::getFuncType(CodeContext& context, NDataType* retType, NDataTypeList* params)
{
	bool valid = true;
	vector<SType*> args;
	for (auto item : *params) {
		auto param = CGNDataType::run(context, item);
		if (!param) {
			valid = false;
		} else if (param->isUnsized()) {
			context.addError("parameter can not be " + param->str(&context) + " type", *item);
			valid = false;
		} else if (SType::validate(context, *item, param)) {
			args.push_back(param);
		}
	}

	auto returnType = CGNDataType::run(context, retType);
	if (!returnType) {
		return nullptr;
	} else if (returnType->isAuto()) {
		context.addError("function return type can not be auto", *retType);
		return nullptr;
	}

	return (returnType && valid)? SType::getFunction(context, returnType, args) : nullptr;
}

bool Builder::addMembers(NVariableDeclGroup* group, vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context)
{
	auto stype = CGNDataType::run(context, group->getType());
	if (!stype) {
		return false;
	} else if (stype->isUnsized()) {
		context.addError("struct members must not have " + stype->str(&context) + " type", *group->getType());
		return false;
	}
	bool valid = true;
	for (auto var : *group->getVars()) {
		if (var->hasInit())
			context.addError("structs don't support variable initialization", *var->getInitExp());
		auto name = var->getName()->str;
		auto res = memberNames.insert(name);
		if (!res.second) {
			context.addError("member name " + name + " already declared", var->getName());
			valid = false;
			continue;
		}
		structVector.push_back(make_pair(name, stype));
	}
	return valid;
}

bool Builder::isDeclared(CodeContext& context, Token* name, vector<SType*> templateArgs)
{
	if (SUserType::isDeclared(context, name->str, templateArgs)) {
		context.addError("type with name " + name->str + " already declared", name);
		return true;
	}
	return false;
}

void Builder::validateAttrList(CodeContext& context, NAttributeList* attrs)
{
	if (!attrs)
		return;

	map<string,NAttribute*> m;

	for (auto attr : *attrs) {
		auto name = attr->getName()->str;
		auto it = m.find(name);
		if (it != m.end()) {
			context.addError("duplicate attribute name: " + name, *attr);
		} else {
			m[name] = attr;
		}
	}
}

void Builder::CreateStruct(CodeContext& context, NStructDeclaration::CreateType ctype, Token* name, NVariableDeclGroupList* list)
{
	auto tArgs = context.getTemplateArgs();
	if (isDeclared(context, name, tArgs))
		return;

	auto structName = name->str;
	vector<pair<string, SType*> > structVars;
	set<string> memberNames;
	bool valid = true;
	if (list) {
		for (auto item : *list)
			valid &= addMembers(item, structVars, memberNames, context);
	}
	if (valid) {
		switch (ctype) {
		case NStructDeclaration::CreateType::STRUCT:
			SUserType::createStruct(context, structName, structVars, tArgs);
			return;
		case NStructDeclaration::CreateType::UNION:
			SUserType::createUnion(context, structName, structVars, tArgs);
			return;
		case NStructDeclaration::CreateType::CLASS:
			auto cl = SUserType::createClass(context, structName, structVars, tArgs);
			context.setClass(cl);
			return;
		}
	}
}

void Builder::CreateEnum(CodeContext& context, NEnumDeclaration* stm)
{
	if (isDeclared(context, stm->getName(), {}))
		return;

	int64_t val = 0;
	set<string> names;
	vector<pair<string,int64_t>> structure;
	bool valid = true;

	for (auto item : *stm->getVarList()) {
		auto name = item->getName()->str;
		auto res = names.insert(name);
		if (!res.second) {
			context.addError("enum member name " + name + " already declared", item->getName());
			valid = false;
			continue;
		}

		NExpression* initExp = nullptr;
		if (item->getInitExp()) {
			initExp = item->getInitExp();
		} else if (item->getInitList()) {
			auto initList = item->getInitList();
			if (initList->size() != 1) {
				context.addError("enum initializer list only supports a single value", item->getName());
				valid = false;
				continue;
			}
			initExp = initList->at(0);
		}

		if (initExp) {
			auto initVal = CGNExpression::run(context, initExp);
			if (!initVal) {
				continue;
			} else if (!isa<Constant>(initVal.value())) {
				context.addError("enum initializer must be a constant", *initExp);
				valid = false;
				continue;
			} else if (!isa<ConstantInt>(initVal.value())) {
				context.addError("enum initializer must be an int-like constant", *initExp);
				valid = false;
				continue;
			}
			val = static_cast<ConstantInt*>(initVal.value())->getSExtValue();
		}
		structure.push_back(make_pair(name, val++));
	}

	auto etype = stm->getBaseType()? CGNDataType::run(context, stm->getBaseType()) : SType::getInt(context, 32);
	if (!etype || !etype->isInteger() || etype->isBool()) {
		context.addError("enum base type must be an integer type", *stm->getBaseType());
		return;
	}

	if (valid)
		SUserType::createEnum(context, stm->getName()->str, structure, etype);
}

void Builder::CreateAlias(CodeContext& context, NAliasDeclaration* stm)
{
	if (isDeclared(context, stm->getName(), {}))
		return;

	auto realType = CGNDataType::run(context, stm->getType());
	if (!realType) {
		return;
	} else if (realType->isAuto()) {
		context.addError("can not create alias to auto type", *stm->getType());
		return;
	}

	SAliasType::createAlias(context, stm->getName()->str, realType);
}

void Builder::CreateGlobalVar(CodeContext& context, NGlobalVariableDecl* stm, bool declaration)
{
	auto initValue = CGNExpression::run(context, stm->getInitExp());
	if (initValue && !isa<Constant>(initValue.value())) {
		context.addError("global variables only support constant value initializer", stm->getName());
		return;
	}

	auto varType = CGNDataType::run(context, stm->getType());
	if (!varType) {
		return;
	} else if (varType->isAuto()) {
		if (!stm->getInitExp()) { // auto type requires initialization
			context.addError("auto variable type requires initialization", *stm->getType());
			return;
		} else if (!initValue) {
			return;
		}
		varType = initValue.stype();
	} else if (varType->isUnsized()) {
		context.addError("can't create variable for an unsized type: " + varType->str(&context), *stm->getType());
		return;
	} else if (!SType::validate(context, stm->getName(), varType)) {
		return;
	}

	if (initValue) {
		if (initValue.isNullPtr()) {
			Inst::CastTo(context, *stm->getInitExp(), initValue, varType);
		} else if (varType != initValue.stype() && SType::getMutable(context, varType) != SType::getMutable(context, initValue.stype())) {
			context.addError("global variable initialization requires exact type matching", *stm->getInitExp());
			return;
		}
	}

	if (varType->isConst() && !initValue) {
		context.addError("const variables require initialization", stm->getName());
		return;
	}

	auto name = stm->getName()->str;
	if (context.loadSymbolCurr(name)) {
		context.addError("variable " + name + " already defined", stm->getName());
		return;
	}

	auto var = new GlobalVariable(*context.getModule(), *varType, false, GlobalValue::ExternalLinkage, declaration? nullptr : (Constant*) initValue.value(), name);
	var->setConstant(varType->isConst());
	context.storeGlobalSymbol({var, varType}, name);
}

void Builder::LoadImport(CodeContext& context, NImportStm* stm)
{
	auto filename = Util::relative(context.currFile().parent_path() / stm->getName()->str);
	if (context.fileLoaded(filename)) {
		return;
	} else if (!exists(filename)) {
		context.addError("unable to import file: " + stm->getName()->str, *stm);
		return;
	}
	Parser parser(filename.string());
	if (parser.parse()) {
		auto err = parser.getError();
		context.addError(err.str, &err);
		return;
	}

	context.pushFile(filename);
	CGNImportStm::run(context, parser.getRoot());
	context.popFile();
}
