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
#include "CodeContext.h"
#include "Instructions.h"
#include "CGNDataType.h"
#include "CGNExpression.h"
#include "CGNStatement.h"
#include "CGNImportStm.h"
#include "Instructions.h"
#include "Util.h"
#include "Builder.h"

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
			body->add(new NReturnStatement(name->copy()));
		else
			context.addError("no return for a non-void function", name);
	}

	context.startFuncBlock(function);

	int i = 0;
	set<string> names;
	for (auto arg = function.arg_begin(); arg != function.arg_end(); arg++, i++) {
		auto param = params->at(i);
		auto paramName = param->getName()->str;
		if (names.insert(paramName).second)
			arg->setName(paramName);
		else
			context.addError("function parameter " + paramName + " already declared", param->getName());
		CGNStatement visitor(context);
		visitor.storeValue(RValue(&*arg, function.getParam(i)));
		visitor.visit(param);
	}

	CGNStatement::run(context, body);
	context.endFuncBlock();
	return function;
}

void Builder::AddOperatorOverload(CodeContext& context, NClassFunctionDecl* stm, SClassType* clType, SFunction func)
{
	auto oper = NAttributeList::find(stm->getAttrs(), "oper");
	if (!oper)
		return;

	auto val = NAttribute::find(oper, 0);
	if (!val) {
		context.addError("operator overload attribute requires value", *oper);
		return;
	}

	static set<string> asgOps = {"=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|=", "\?\?=",
		"==", "!=", "<", "<=", ">", ">="
	};

	auto op = val->str();
	if (asgOps.find(op) == asgOps.end()) {
		context.addError("unsupported operator: " + op, *val);
		return;
	}

	if (func.numParams() != 2) {
		context.addError("overloaded operator '" + op +"' requires a single parameter function", *val);
		return;
	}

	auto existing = clType->getItem(op);
	if (existing) {
		for (auto item : *existing) {
			if (item.second.stype() == func.stype()) {
				context.addError("overloaded operator functions must be unique", *val);
				return;
			}
		}
	}

	clType->addFunction(op, func);
}

void Builder::CreateClassFunction(CodeContext& context, NClassFunctionDecl* stm, bool prototype)
{
	validateAttrList(context, stm->getAttrs());

	auto name = stm->getName();
	auto clType = context.getClass();
	auto item = clType->getItem(name->str);
	if (item && !item->at(0).second.isFunction()) {
		context.addError("class " + clType->str(context) + " already defines symbol " + name->str, name);
		return;
	}

	// add this parameter
	bool hasThis = false;
	if (!NAttributeList::find(stm->getAttrs(), "static")) {
		auto thisToken = new Token(clType->raw());
		auto thisPtr = new NParameter(new NPointerType(new NUserType(thisToken)), new Token("this"));
		stm->getParams()->addFront(thisPtr);
		hasThis = true;
	}

	// add function to class type
	auto func = CreateFunction(context, name, stm->getRType(), stm->getParams(), prototype? nullptr : stm->getBody(), stm->getAttrs());
	if (func && !clType->hasItem(name->str, func)) {
		clType->addFunction(name->str, func);
		AddOperatorOverload(context, stm, clType, func);
	}
	if (hasThis)
		delete stm->getParams()->popFront();
}

bool Builder::SetupClassConstructor(CodeContext& context, NClassConstructor* stm, bool prototype)
{
	if (prototype && (stm->getBody()->size() || stm->getInitList()->size()))
		return true;

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

	if (!prototype)
		stm->getInitList()->clear();

	auto classTy = context.getClass();
	for (auto item : *classTy) {
		auto it = items.find(item.first);
		if (it != items.end())
			continue;

		// all these types should be fields
		auto stype = item.second[0].second.stype();
		SClassType* clTy;

		if (stype->isClass())
			clTy = static_cast<SClassType*>(stype);
		else if (stype->isArray() && stype->subType()->isClass())
			clTy = static_cast<SClassType*>(stype->subType());
		else
			continue;

		if (clTy->getConstructor().size()) {
			items.insert({item.first, new NMemberInitializer(new Token(*stm->getName(), item.first), new NExpressionList)});
		}
	}

	if (!prototype && items.size()) {
		auto newBody = new NStatementList;
		newBody->reserve(items.size() + stm->getBody()->size());
		for (auto item : items)
			newBody->add(item.second);
		newBody->addAll(*stm->getBody());
		stm->getBody()->setDelete(false);
		stm->setBody(newBody);
	}

	return stm->getBody()->size() || items.size();
}

void Builder::CreateClassConstructor(CodeContext& context, NClassConstructor* stm, bool prototype)
{
	if (!stm->getRType())
		stm->setRType(new NBaseType(nullptr, ParserBase::TT_VOID));

	if (SetupClassConstructor(context, stm, prototype))
		CreateClassFunction(context, stm, prototype);
}

bool Builder::SetupClassDestructor(CodeContext& context, NClassDestructor* stm, bool prototype)
{
	if (prototype && stm->getBody()->size())
		return true;

	// TODO check if destructor exists
	auto clType = context.getClass();
	for (auto item : *clType) {
		// only looking for fields so first item will do
		auto ty = item.second[0].second.stype();
		if (!ty->isClass())
			continue;
		auto itemCl = static_cast<SClassType*>(ty);
		if (!itemCl->getDestructor())
			continue;

		if (!prototype)
			stm->getBody()->add(new NDestructorCall(new NBaseVariable(new Token(item.first)), nullptr));
		else
			return true;
	}
	return stm->getBody()->size();
}

void Builder::CreateClassDestructor(CodeContext& context, NClassDestructor* stm, bool prototype)
{
	stm->getName()->str = "null";
	if (!stm->getRType())
		stm->setRType(new NBaseType(nullptr, ParserBase::TT_VOID));

	if (SetupClassDestructor(context, stm, prototype))
		CreateClassFunction(context, stm, prototype);
}

void Builder::CreateClass(CodeContext& context, NClassDeclaration* stm, const function<void(int)>& visitor)
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
	context.setThis(nullptr);
}

SFunction Builder::getFuncPrototype(CodeContext& context, Token* name, SFunctionType* funcType, NAttributeList* attrs, bool allowMangle)
{
	string rawName;
	string funcName = name->str;

	if (allowMangle) {
		bool fullMangle = false;
		auto mangle = NAttributeList::find(attrs, "mangle");
		if (mangle) {
			auto mangleVal = NAttribute::find(mangle, 0);
			if (mangleVal) {
				rawName = mangleVal->str();
				fullMangle = NAttribute::find(mangle, 1) != nullptr;
			} else {
				context.addError("mangle attribute requires value", *mangle);
			}
		}
		if (context.getClass()) {
			auto clName = context.getClass()->raw();
			if (!rawName.empty()) {
				if (!fullMangle) {
					rawName = clName + "_" + rawName;
				} else if (context.getClass()->isTemplated()) {
					context.addError("cannot use fullname mangling with templated class functions", *mangle);
				}
			}
			funcName = clName + "_" + funcName;
		}
	}

	if (rawName.empty()) {
		rawName = funcName;
	}
	auto isOverride = rawName != funcName;

	auto syms = context.loadSymbolGlobal(funcName);
	if (syms.size()) {
		// find function that already matches
		auto isFunction = false;
		for (auto sym : syms) {
			if (!sym.isFunction())
				continue;

			auto function = static_cast<SFunction&>(sym);
			if (funcType == function.stype())
				return function;
			isFunction = true;
		}

		if (!isFunction || !isOverride) {
			if (isFunction) {
				context.addError("function type for " + funcName + " doesn't match definition", name);
			} else {
				context.addError("variable " + funcName + " already defined", name);
			}
			return {};
		}

		// function ok to override -- fall through
	}

	if (isOverride && context.loadSymbolGlobal(rawName).size()) {
		context.addError("cannot mangle function to existing symbol " + rawName, name);
		return {};
	}

	auto func = Function::Create(*funcType, GlobalValue::ExternalLinkage, rawName, context.getModule());
	auto function = SFunction::create(context, func, funcType, attrs);
	context.storeGlobalSymbol(function, rawName);
	if (isOverride)
		context.storeGlobalSymbol(function, funcName);
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
	VecSType args;
	for (auto item : *params) {
		auto param = CGNDataType::run(context, item);
		if (!param) {
			valid = false;
		} else if (param->isUnsized()) {
			context.addError("parameter can not be " + param->str(context) + " type", *item);
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

SFunction Builder::getBuiltinFunc(CodeContext& context, const Token* source, BuiltinFuncType builtin)
{
	VecRValue syms;

	switch (builtin) {
	case BuiltinFuncType::Free:
		syms = context.loadSymbol("free");
		if (syms.empty()) {
			auto bytePtr = SType::getPointer(context, SType::getInt(context, 8));
			auto retType = SType::getVoid(context);
			auto funcType = SType::getFunction(context, retType, {bytePtr});
			Token freeName(*source, "free");

			return getFuncPrototype(context, &freeName, funcType, nullptr, false);
		}
		return static_cast<SFunction&>(syms[0]);
	case BuiltinFuncType::Malloc:
		syms = context.loadSymbol("malloc");
		if (syms.empty()) {
			auto i64 = SType::getInt(context, 64);
			auto retType = SType::getPointer(context, SType::getInt(context, 8));
			auto funcType = SType::getFunction(context, retType, {i64});
			Token mallocName(*source, "malloc");

			return getFuncPrototype(context, &mallocName, funcType, nullptr, false);
		}
		return static_cast<SFunction&>(syms[0]);
	case BuiltinFuncType::Printf:
		syms = context.loadSymbol("printf");
		if (syms.empty()) {
			// TODO refactor once SFunction supports varargs
			auto bytePtr = SType::getPointer(context, SType::getArray(context, SType::getInt(context, 8), 0));
			auto i32 = SType::getInt(context, 32);
			auto funcType = FunctionType::get(i32->type(), {bytePtr->type()}, true);
			auto sFuncType = SType::getFunction(context, i32, {bytePtr});

			auto func = Function::Create(funcType, GlobalValue::ExternalLinkage, "printf", context.getModule());
			auto function = SFunction::create(context, func, sFuncType, nullptr);

			context.storeGlobalSymbol(function, "printf");
			return function;
		}
		return static_cast<SFunction&>(syms[0]);
	default:
		return {};
	}
}

void Builder::AddDebugPrint(CodeContext& context, Token* source, const string& msg, vector<Value*> args)
{
	auto printf = Builder::getBuiltinFunc(context, source, BuiltinFuncType::Printf);

	NStringLiteral nMsg(new Token("\"" + msg + "\n\""));
	auto msgVal = CGNExpression::run(context, &nMsg);
	args.insert(args.begin(), msgVal);

	context.IB().CreateCall(printf.value(), args);
}

bool Builder::addMembers(NStructDeclaration::CreateType ctype, NVariableDeclGroup* group, vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context)
{
	auto stype = CGNDataType::run(context, group->getType());
	if (!stype) {
		return false;
	} else if (stype->isUnsized()) {
		context.addError("unsized struct member not allowed: " + stype->str(context), *group->getType());
		return false;
	} else if (ctype != NStructDeclaration::CreateType::CLASS && (stype->isConstructable() || stype->isDestructable())) {
		context.addError("struct/union types can not contain constructable/destructable members", *group->getType());
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

bool Builder::isDeclared(CodeContext& context, Token* name, VecSType templateArgs)
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

bool Builder::StoreTemplate(CodeContext& context, NTemplatedDeclaration* stm)
{
	if (!context.inTemplate()) {
		auto name = stm->getName()->str;
		if (SUserType::isDeclared(context, name, {}) || context.getTemplate(name)) {
			context.addError("type with name " + name + " already declared", stm->getName());
			return true;
		} else if (stm->getTemplateParams()) {
			context.storeTemplate(name, stm);
			return true;
		}
	}
	return false;
}

void Builder::CreateStruct(CodeContext& context, NStructDeclaration::CreateType ctype, Token* name, NVariableDeclGroupList* list)
{
	auto tArgs = context.getTemplateArgs();
	if (isDeclared(context, name, tArgs))
		return;

	auto structName = name->str;
	vector<pair<string, SType*> > structVars;
	set<string> memberNames;

	STemplatedType* userType;
	switch (ctype) {
	case NStructDeclaration::CreateType::STRUCT:
		userType = SUserType::createStruct(context, structName, tArgs);
		break;
	case NStructDeclaration::CreateType::UNION:
		userType = SUserType::createUnion(context, structName, tArgs);
		break;
	case NStructDeclaration::CreateType::CLASS:
		userType = SUserType::createClass(context, structName, tArgs);
		break;
	default:
		break;
	}
	context.setThis(userType);

	auto isClass = userType->isClass();
	if (isClass)
		context.setClass(static_cast<SClassType*>(userType));

	if (list) {
		for_each(list->begin(), list->end(), [&](auto i){ return addMembers(ctype, i, structVars, memberNames, context); });
		SUserType::setBody(context, userType, structVars);
	}

	if (!isClass)
		context.setThis(nullptr);
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
		context.addError("can't create variable for an unsized type: " + varType->str(context), *stm->getType());
		return;
	} else if (!SType::validate(context, stm->getName(), varType)) {
		return;
	}

	if (initValue) {
		if (initValue.isNullPtr()) {
			Inst::CastTo(context, *stm->getInitExp(), initValue, varType);
		} else if (!SType::isConstEQ(context, varType, initValue.stype())) {
			context.addError("global variable initialization requires exact type matching", *stm->getInitExp());
			return;
		}
	}

	if (varType->isConst() && !initValue) {
		context.addError("const variables require initialization", stm->getName());
		return;
	}

	auto name = stm->getName()->str;
	if (context.loadSymbolCurr(name).size()) {
		context.addError("variable " + name + " already defined", stm->getName());
		return;
	}

	auto var = new GlobalVariable(*context.getModule(), *varType, false, GlobalValue::ExternalLinkage, declaration? nullptr : (Constant*) initValue.value(), name);
	var->setConstant(varType->isConst());
	context.storeGlobalSymbol({var, varType}, name);
}

void Builder::LoadImport(CodeContext& context, NImportFileStm* stm)
{
	path filename;
	if (stm->id() == NodeId::NImportPkgStm) {
		auto imp = static_cast<NImportPkgStm*>(stm);
		filename = Util::getDataDir();
		for (auto seg : *imp->getSegments())
			filename /= seg->str;
		filename = filename.string() + ".syp";
	} else {
		filename = context.currFile().parent_path() / stm->getName()->str;
	}

	if (!exists(filename)) {
		context.addError("unable to import: " + stm->getName()->str, stm->getName());
		return;
	}

	filename = canonical(filename);
	if (context.fileLoaded(filename)) {
		return;
	}

	Parser parser(filename.string());
	auto path = Util::relative(filename).string();
	if (path.compare(0, 2, "..") == 0) {
		string pkgPath = ".local/share/saphyr/pkgs";
		auto idx = path.find(pkgPath);
		if (idx != string::npos)
			path = "<pkg>/" + path.substr(idx + pkgPath.size() + 1);
	}
	parser.setFilename(path);

	if (parser.parse()) {
		auto err = parser.getError();
		context.addError(err.str, &err);
		return;
	}

	context.pushFile(filename);
	CGNImportStm::run(context, parser.getRoot());
	context.popFile();
}
