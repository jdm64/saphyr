/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2014, Justin Madru (justin.jdm64@gmail.com)
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
#include "LLVM_Defines.h"

#include "parserbase.h"
#include "AST.h"
#include "Instructions.h"
#include "Builder.h"
#include "CGNDataType.h"
#include "CGNInt.h"
#include "CGNVariable.h"
#include "CGNExpression.h"

void NParameter::genCode(CodeContext& context)
{
	auto stype = CGNDataType::run(context, type);
	auto stackAlloc = new AllocaInst(*stype, "", context);
	new StoreInst(arg, stackAlloc, context);
	context.storeLocalSymbol({stackAlloc, stype}, getName());
}

void NVariableDecl::genCode(CodeContext& context)
{
	auto initValue = CGNExpression::run(context, initExp);
	auto varType = CGNDataType::run(context, type);

	if (!varType) {
		return;
	} else if (varType->isAuto()) {
		if (!initExp) { // auto type requires initialization
			auto token = static_cast<NNamedType*>(type)->getToken();
			context.addError("auto variable type requires initialization", token);
			return;
		} else if (!initValue) {
			return;
		}
		varType = initValue.stype();
	} else if (!SType::validate(context, getNameToken(), varType)) {
		return;
	}

	auto name = getName();
	if (context.loadSymbolCurr(name)) {
		context.addError("variable " + name + " already defined", getNameToken());
		return;
	}

	auto var = RValue(new AllocaInst(*varType, name, context), varType);
	context.storeLocalSymbol(var, name);

	Inst::InitVariable(context, var, getNameToken(), initList, initValue);
}

void NGlobalVariableDecl::genCode(CodeContext& context)
{
	Builder::CreateGlobalVar(context, this);
}

void NAliasDeclaration::genCode(CodeContext& context)
{
	Builder::CreateAlias(context, this);
}

void NStructDeclaration::genCode(CodeContext& context)
{
	Builder::CreateStruct(context, ctype, getNameToken(), list);
}

void NEnumDeclaration::genCode(CodeContext& context)
{
	Builder::CreateEnum(context, this);
}

void NFunctionDeclaration::genCode(CodeContext& context)
{
	Builder::CreateFunction(context, getNameToken(), rtype, params, body);
}

void NClassStructDecl::genCode(CodeContext& context)
{
	auto stToken = theClass->getNameToken();
	auto stType = NStructDeclaration::CreateType::CLASS;
	Builder::CreateStruct(context, stType, stToken, list);
}

void NClassFunctionDecl::genCode(CodeContext& context)
{
	Builder::CreateClassFunction(context, name, theClass, rtype, params, body);
}

void NClassConstructor::genCode(CodeContext& context)
{
	Builder::CreateClassConstructor(context, this);
}

void NClassDestructor::genCode(CodeContext& context)
{
	Builder::CreateClassDestructor(context, this);
}

void NMemberInitializer::genCode(CodeContext& context)
{
	auto currClass = context.getClass();
	if (!currClass)
		return;

	auto item = currClass->getItem(name->str);
	if (!item) {
		context.addError("invalid initializer, class variable not defined: " + name->str, name);
		return;
	}

	auto var = Inst::LoadMemberVar(context, name->str);
	RValue empty;
	Inst::InitVariable(context, var, name, expression, empty);
}

void NClassDeclaration::genCode(CodeContext& context)
{
	int structIdx = -1;
	int constrIdx = -1;
	int destrtIdx = -1;
	for (int i = 0; i < list->size(); i++) {
		switch (list->at(i)->memberType()) {
		case NClassMember::MemberType::STRUCT:
			if (structIdx > -1)
				context.addError("only one struct allowed in a class", list->at(i)->getNameToken());
			else
				structIdx = i;
			break;
		case NClassMember::MemberType::CONSTRUCTOR:
			if (constrIdx > -1)
				context.addError("only one constructor allowed in a class", list->at(i)->getNameToken());
			else
				constrIdx = i;
			break;
		case NClassMember::MemberType::DESTRUCTOR:
			if (destrtIdx > -1)
				context.addError("only one destructor allowed in a class", list->at(i)->getNameToken());
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
		structDecl->setClass(this);
		varList->add(new NVariableDecl(new Token));
		group->add(new NVariableDeclGroup(new NBaseType(nullptr, ParserBase::TT_INT8), varList));
		list->add(structDecl);
		structIdx = list->size() - 1;
	}
	if (constrIdx < 0) {
		auto constr = new NClassConstructor(new Token("this"), new NParameterList, new NInitializerList, new NStatementList);
		constr->setClass(this);
		list->add(constr);
	}
	if (destrtIdx < 0) {
		auto destr = new NClassDestructor(new Token, new NStatementList);
		destr->setClass(this);
		list->add(destr);
	}

	list->at(structIdx)->genCode(context);
	context.setClass(static_cast<SClassType*>(SUserType::lookup(context, getName())));

	for (int i = 0; i < list->size(); i++) {
		if (i == structIdx)
			continue;
		list->at(i)->genCode(context);
	}
	context.setClass(nullptr);
}

void NReturnStatement::genCode(CodeContext& context)
{
	auto func = context.currFunction();
	auto funcReturn = func.returnTy();

	if (funcReturn->isVoid()) {
		if (value) {
			context.addError("function " + func.name().str() + " declared void, but non-void return found", retToken);
			return;
		}
	} else if (!value) {
		context.addError("function " + func.name().str() + " declared non-void, but void return found", retToken);
		return;
	}
	auto returnVal = CGNExpression::run(context, value);
	if (returnVal)
		Inst::CastTo(context, retToken, returnVal, funcReturn);
	ReturnInst::Create(context, returnVal, context);
	context.pushBlock(context.createBlock());
}

void NLoopStatement::genCode(CodeContext& context)
{
	auto bodyBlock = context.createContinueBlock();
	auto endBlock = context.createBreakBlock();

	context.pushLocalTable();

	BranchInst::Create(bodyBlock, context);
	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(bodyBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

void NWhileStatement::genCode(CodeContext& context)
{
	auto condBlock = context.createContinueBlock();
	auto bodyBlock = context.createRedoBlock();
	auto endBlock = context.createBreakBlock();

	auto startBlock = isDoWhile? bodyBlock : condBlock;
	auto trueBlock = isUntil? endBlock : bodyBlock;
	auto falseBlock = isUntil? bodyBlock : endBlock;

	context.pushLocalTable();

	BranchInst::Create(startBlock, context);

	context.pushBlock(condBlock);
	Inst::Branch(trueBlock, falseBlock, condition, lparen, context);

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(condBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

void NSwitchStatement::genCode(CodeContext& context)
{
	auto switchValue = CGNExpression::run(context, value);
	Inst::CastTo(context, lparen, switchValue, SType::getInt(context, 32));

	auto caseBlock = context.createBlock();
	auto endBlock = context.createBreakBlock(), defaultBlock = endBlock;
	auto switchInst = SwitchInst::Create(switchValue, defaultBlock, cases->size(), context);

	context.pushLocalTable();

	set<int64_t> unique;
	bool hasDefault = false;
	for (auto caseItem : *cases) {
		if (caseItem->isValueCase()) {
			auto val = ConstantInt::get(context, CGNInt::run(context, caseItem->getValue()));
			if (!unique.insert(val->getSExtValue()).second)
				context.addError("switch case values are not unique", caseItem->getToken());
			switchInst->addCase(val, caseBlock);
		} else {
			if (hasDefault)
				context.addError("switch statement has more than one default", caseItem->getToken());
			hasDefault = true;
			defaultBlock = caseBlock;
		}

		context.pushBlock(caseBlock);
		caseItem->genCode(context);

		if (caseItem->isLastStmBranch()) {
			caseBlock = context.currBlock();
		} else {
			caseBlock = context.createBlock();
			BranchInst::Create(caseBlock, context);
			context.pushBlock(caseBlock);
		}
	}
	switchInst->setDefaultDest(defaultBlock);

	// NOTE: the last case will create a dangling block which needs a terminator.
	BranchInst::Create(endBlock, context);

	context.popLocalTable();
	context.popBreakBlock();
	context.pushBlock(endBlock);
}

void NForStatement::genCode(CodeContext& context)
{
	auto condBlock = context.createBlock();
	auto bodyBlock = context.createRedoBlock();
	auto postBlock = context.createContinueBlock();
	auto endBlock = context.createBreakBlock();

	context.pushLocalTable();

	preStm->genCode(context);
	BranchInst::Create(condBlock, context);

	context.pushBlock(condBlock);
	Inst::Branch(bodyBlock, endBlock, condition, semiCol2, context);

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(postBlock, context);

	context.pushBlock(postBlock);
	CGNExpression::run(context, postExp);
	BranchInst::Create(condBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

void NIfStatement::genCode(CodeContext& context)
{
	auto ifBlock = context.createBlock();
	auto elseBlock = context.createBlock();
	auto endBlock = elseBody? context.createBlock() : elseBlock;

	context.pushLocalTable();

	Inst::Branch(ifBlock, elseBlock, condition, lparen, context);

	context.pushBlock(ifBlock);
	body->genCode(context);
	BranchInst::Create(endBlock, context);

	context.popLocalTable();
	context.pushLocalTable();

	context.pushBlock(elseBlock);
	if (elseBody) {
		elseBody->genCode(context);
		BranchInst::Create(endBlock, context);
	}
	context.pushBlock(endBlock);
	context.popLocalTable();
}

void NLabelStatement::genCode(CodeContext& context)
{
	// check if label already declared
	auto labelName = getName();
	auto label = context.getLabelBlock(labelName);
	if (label) {
		if (!label->isPlaceholder) {
			context.addError("label " + labelName + " already defined", getNameToken());
			return;
		}
		// a used label is no longer a placeholder
		label->isPlaceholder = false;
	} else {
		label = context.createLabelBlock(getNameToken(), false);
	}
	BranchInst::Create(label->block, context);
	context.pushBlock(label->block);
}

void NGotoStatement::genCode(CodeContext& context)
{
	auto labelName = getName();
	auto skip = context.createBlock();
	auto label = context.getLabelBlock(labelName);
	if (!label) {
		// trying to jump to a non-existant label. create place holder and
		// later check if it's used at the end of the function.
		label = context.createLabelBlock(getNameToken(), true);
	}
	BranchInst::Create(label->block, context);
	context.pushBlock(skip);
}

void NLoopBranch::genCode(CodeContext& context)
{
	BasicBlock* block;
	string typeName;
	auto brLevel = level? CGNInt::run(context, level).getSExtValue() : 1;

	switch (type) {
	case ParserBase::TT_CONTINUE:
		block = context.getContinueBlock(brLevel);
		if (!block) {
			typeName = "continue";
			goto error;
		}
		break;
	case ParserBase::TT_REDO:
		block = context.getRedoBlock(brLevel);
		if (!block) {
			typeName = "redo";
			goto error;
		}
		break;
	case ParserBase::TT_BREAK:
		block = context.getBreakBlock(brLevel);
		if (!block) {
			typeName = "break";
			goto error;
		}
		break;
	default:
		context.addError("undefined loop branch type: " + to_string(type), token);
		return;
	}
	BranchInst::Create(block, context);
	context.pushBlock(context.createBlock());
	return;
error:
	context.addError(typeName + " invalid outside a loop/switch block", token);
}

void NDeleteStatement::genCode(CodeContext& context)
{
	static string freeName = "free";

	auto bytePtr = SType::getPointer(context, SType::getInt(context, 8));
	auto func = context.loadSymbol(freeName);
	if (!func) {
		vector<SType*> args;
		args.push_back(bytePtr);
		auto retType = SType::getVoid(context);
		auto funcType = SType::getFunction(context, retType, args);

		func = SFunction::create(context, freeName, funcType);
	} else if (!func.isFunction()) {
		context.addError("Compiler Error: free not function", token);
		return;
	}

	auto ptr = CGNExpression::run(context, variable);
	if (!ptr) {
		return;
	} else if (!ptr.stype()->isPointer()) {
		context.addError("delete requires pointer type", token);
		return;
	}

	if (ptr.stype()->subType()->isClass())
		Inst::CallDestructor(context, ptr, token);

	vector<Value*> exp_list;
	exp_list.push_back(new BitCastInst(ptr, *bytePtr, "", context));

	CallInst::Create(static_cast<SFunction&>(func), exp_list, "", context);
}

void NDestructorCall::genCode(CodeContext& context)
{
	auto value = CGNVariable::run(context, baseVar);
	if (!value)
		return;

	value = RValue(value, SType::getPointer(context, value.stype()));

	auto type = value.stype();
	while (true) {
		auto sub = type->subType();
		if (sub->isClass()) {
			break;
		} else if (sub->isPointer()) {
			value = Inst::Deref(context, value);
			type = value.stype();
		} else {
			context.addError("calling destructor only valid for classes", thisToken);
			return;
		}
	}

	Inst::CallDestructor(context, value, thisToken);
}

void NExpressionStm::genCode(CodeContext& context)
{
	CGNExpression::run(context, exp);
}
