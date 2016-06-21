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
#include "Value.h"
#include "AST.h"
#include "CGNStatement.h"
#include "parserbase.h"
#include "CodeContext.h"
#include "Instructions.h"
#include "Builder.h"
#include "CGNDataType.h"
#include "CGNVariable.h"
#include "CGNInt.h"
#include "CGNExpression.h"
#include "CGNImportStm.h"

#define TABLE_ADD(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartStatement)] = reinterpret_cast<classPtr>(&CGNStatement::visit##ID)

CGNStatement::classPtr* CGNStatement::buildVTable()
{
	auto table = new CGNStatement::classPtr[NODEID_DIFF(NodeId::EndStatement, NodeId::StartStatement)];
	TABLE_ADD(NAliasDeclaration);
	TABLE_ADD(NClassConstructor);
	TABLE_ADD(NClassDeclaration);
	TABLE_ADD(NClassDestructor);
	TABLE_ADD(NClassFunctionDecl);
	TABLE_ADD(NClassStructDecl);
	TABLE_ADD(NDeleteStatement);
	TABLE_ADD(NDestructorCall);
	TABLE_ADD(NEnumDeclaration);
	TABLE_ADD(NExpressionStm);
	TABLE_ADD(NForStatement);
	TABLE_ADD(NFunctionDeclaration);
	TABLE_ADD(NGlobalVariableDecl);
	TABLE_ADD(NGotoStatement);
	TABLE_ADD(NIfStatement);
	TABLE_ADD(NImportStm);
	TABLE_ADD(NLabelStatement);
	TABLE_ADD(NLoopBranch);
	TABLE_ADD(NLoopStatement);
	TABLE_ADD(NMemberInitializer);
	TABLE_ADD(NParameter);
	TABLE_ADD(NReturnStatement);
	TABLE_ADD(NStructDeclaration);
	TABLE_ADD(NSwitchStatement);
	TABLE_ADD(NVariableDecl);
	TABLE_ADD(NVariableDeclGroup);
	TABLE_ADD(NWhileStatement);
	return table;
}

CGNStatement::classPtr* CGNStatement::vtable = CGNStatement::buildVTable();

void CGNStatement::visit(NStatement* stm)
{
	(this->*vtable[NODEID_DIFF(stm->id(), NodeId::StartStatement)])(stm);
}

void CGNStatement::visit(NStatementList* list)
{
	for (auto item : *list)
		visit(item);
}

void CGNStatement::visitNImportStm(NImportStm* stm)
{
	auto idx = context.getFilename().rfind('/');
	string prefix = idx != string::npos? context.getFilename().substr(0, idx + 1) : "";
	string filename = prefix + stm->getName()->str;

	Builder::LoadImport(context, filename);
}

void CGNStatement::visitNExpressionStm(NExpressionStm* stm)
{
	CGNExpression::run(context, stm->getExp());
}

void CGNStatement::visitNParameter(NParameter* stm)
{
	auto stype = CGNDataType::run(context, stm->getType());
	auto stackAlloc = new AllocaInst(*stype, "", context);
	new StoreInst(storedValue, stackAlloc, context);
	context.storeLocalSymbol({stackAlloc, stype}, stm->getName());
}

void CGNStatement::visitNVariableDecl(NVariableDecl* stm)
{
	auto initValue = CGNExpression::run(context, stm->getInitExp());
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

	auto name = stm->getName();
	if (context.loadSymbolCurr(name)) {
		context.addError("variable " + name + " already defined", stm->getNameToken());
		return;
	}

	auto var = RValue(new AllocaInst(*varType, name, context), varType);
	context.storeLocalSymbol(var, name);

	Inst::InitVariable(context, var, stm->getNameToken(), stm->getInitList(), initValue);
}

void CGNStatement::visitNVariableDeclGroup(NVariableDeclGroup* stm)
{
	for (auto variable : *stm->getVars()) {
		variable->setDataType(stm->getType());
		visit(variable);
	}
}

void CGNStatement::visitNGlobalVariableDecl(NGlobalVariableDecl* stm)
{
	Builder::CreateGlobalVar(context, stm, false);
}

void CGNStatement::visitNAliasDeclaration(NAliasDeclaration* stm)
{
	Builder::CreateAlias(context, stm);
}

void CGNStatement::visitNStructDeclaration(NStructDeclaration* stm)
{
	Builder::CreateStruct(context, stm->getType(), stm->getNameToken(), stm->getVars());
}

void CGNStatement::visitNEnumDeclaration(NEnumDeclaration* stm)
{
	Builder::CreateEnum(context, stm);
}

void CGNStatement::visitNFunctionDeclaration(NFunctionDeclaration* stm)
{
	Builder::CreateFunction(context, stm->getNameToken(), stm->getRType(), stm->getParams(), stm->getBody());
}

void CGNStatement::visitNClassStructDecl(NClassStructDecl* stm)
{
	auto stToken = stm->getClass()->getNameToken();
	auto stType = NStructDeclaration::CreateType::CLASS;
	Builder::CreateStruct(context, stType, stToken, stm->getVarList());
}

void CGNStatement::visitNClassFunctionDecl(NClassFunctionDecl* stm)
{
	Builder::CreateClassFunction(context, stm->getNameToken(), stm->getClass(), stm->getRType(), stm->getParams(), stm->getBody());
}

void CGNStatement::visitNClassConstructor(NClassConstructor* stm)
{
	Builder::CreateClassConstructor(context, stm, false);
}

void CGNStatement::visitNClassDestructor(NClassDestructor* stm)
{
	Builder::CreateClassDestructor(context, stm, false);
}

void CGNStatement::visitNMemberInitializer(NMemberInitializer* stm)
{
	auto currClass = context.getClass();
	if (!currClass)
		return;

	auto item = currClass->getItem(stm->getNameToken()->str);
	if (!item) {
		context.addError("invalid initializer, class variable not defined: " + stm->getNameToken()->str, stm->getNameToken());
		return;
	}

	auto var = Inst::LoadMemberVar(context, stm->getNameToken()->str);
	RValue empty;
	Inst::InitVariable(context, var, stm->getNameToken(), stm->getExp(), empty);
}

void CGNStatement::visitNClassDeclaration(NClassDeclaration* stm)
{
	Builder::CreateClass(context, stm, [=](int structIdx){
		visit(stm->getList()->at(structIdx));
		context.setClass(static_cast<SClassType*>(SUserType::lookup(context, stm->getName())));

		for (int i = 0; i < stm->getList()->size(); i++) {
			if (i == structIdx)
				continue;
			visit(stm->getList()->at(i));
		}
		context.setClass(nullptr);
	});
}

void CGNStatement::visitNReturnStatement(NReturnStatement* stm)
{
	auto func = context.currFunction();
	auto funcReturn = func.returnTy();

	if (funcReturn->isVoid()) {
		if (stm->getValue()) {
			context.addError("function " + func.name().str() + " declared void, but non-void return found", stm->getToken());
			return;
		}
	} else if (!stm->getValue()) {
		context.addError("function " + func.name().str() + " declared non-void, but void return found", stm->getToken());
		return;
	}
	auto returnVal = CGNExpression::run(context, stm->getValue());
	if (returnVal)
		Inst::CastTo(context, stm->getToken(), returnVal, funcReturn);
	ReturnInst::Create(context, returnVal, context);
	context.pushBlock(context.createBlock());
}

void CGNStatement::visitNLoopStatement(NLoopStatement* stm)
{
	auto bodyBlock = context.createContinueBlock();
	auto endBlock = context.createBreakBlock();

	context.pushLocalTable();

	BranchInst::Create(bodyBlock, context);
	context.pushBlock(bodyBlock);
	visit(stm->getBody());
	BranchInst::Create(bodyBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

void CGNStatement::visitNWhileStatement(NWhileStatement* stm)
{
	auto condBlock = context.createContinueBlock();
	auto bodyBlock = context.createRedoBlock();
	auto endBlock = context.createBreakBlock();

	auto startBlock = stm->doWhile()? bodyBlock : condBlock;
	auto trueBlock = stm->until()? endBlock : bodyBlock;
	auto falseBlock = stm->until()? bodyBlock : endBlock;

	context.pushLocalTable();

	BranchInst::Create(startBlock, context);

	context.pushBlock(condBlock);
	Inst::Branch(trueBlock, falseBlock, stm->getCond(), stm->getLParen(), context);

	context.pushBlock(bodyBlock);
	visit(stm->getBody());
	BranchInst::Create(condBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

void CGNStatement::visitNSwitchStatement(NSwitchStatement* stm)
{
	auto switchValue = CGNExpression::run(context, stm->getValue());
	Inst::CastTo(context, stm->getLParen(), switchValue, SType::getInt(context, 32));

	auto caseBlock = context.createBlock();
	auto endBlock = context.createBreakBlock(), defaultBlock = endBlock;
	auto switchInst = SwitchInst::Create(switchValue, defaultBlock, stm->getCases()->size(), context);

	context.pushLocalTable();

	set<int64_t> unique;
	bool hasDefault = false;
	for (auto caseItem : *stm->getCases()) {
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
		visit(caseItem->getBody());

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

void CGNStatement::visitNForStatement(NForStatement* stm)
{
	auto condBlock = context.createBlock();
	auto bodyBlock = context.createRedoBlock();
	auto postBlock = context.createContinueBlock();
	auto endBlock = context.createBreakBlock();

	context.pushLocalTable();

	visit(stm->getPreStm());
	BranchInst::Create(condBlock, context);

	context.pushBlock(condBlock);
	Inst::Branch(bodyBlock, endBlock, stm->getCond(), stm->getSemiCol2(), context);

	context.pushBlock(bodyBlock);
	visit(stm->getBody());
	BranchInst::Create(postBlock, context);

	context.pushBlock(postBlock);
	CGNExpression::run(context, stm->getPostExp());
	BranchInst::Create(condBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

void CGNStatement::visitNIfStatement(NIfStatement* stm)
{
	auto ifBlock = context.createBlock();
	auto elseBlock = context.createBlock();
	auto endBlock = stm->getElseBody()? context.createBlock() : elseBlock;

	context.pushLocalTable();

	Inst::Branch(ifBlock, elseBlock, stm->getCond(), stm->getToken(), context);

	context.pushBlock(ifBlock);
	visit(stm->getBody());
	BranchInst::Create(endBlock, context);

	context.popLocalTable();
	context.pushLocalTable();

	context.pushBlock(elseBlock);
	if (stm->getElseBody()) {
		visit(stm->getElseBody());
		BranchInst::Create(endBlock, context);
	}
	context.pushBlock(endBlock);
	context.popLocalTable();
}

void CGNStatement::visitNLabelStatement(NLabelStatement* stm)
{
	// check if label already declared
	auto labelName = stm->getName();
	auto label = context.getLabelBlock(labelName);
	if (label) {
		if (!label->isPlaceholder) {
			context.addError("label " + labelName + " already defined", stm->getNameToken());
			return;
		}
		// a used label is no longer a placeholder
		label->isPlaceholder = false;
	} else {
		label = context.createLabelBlock(stm->getNameToken(), false);
	}
	BranchInst::Create(label->block, context);
	context.pushBlock(label->block);
}

void CGNStatement::visitNGotoStatement(NGotoStatement* stm)
{
	auto labelName = stm->getName();
	auto skip = context.createBlock();
	auto label = context.getLabelBlock(labelName);
	if (!label) {
		// trying to jump to a non-existant label. create place holder and
		// later check if it's used at the end of the function.
		label = context.createLabelBlock(stm->getNameToken(), true);
	}
	BranchInst::Create(label->block, context);
	context.pushBlock(skip);
}

void CGNStatement::visitNLoopBranch(NLoopBranch* stm)
{
	BasicBlock* block;
	string typeName;
	auto brLevel = stm->getLevel()? CGNInt::run(context, stm->getLevel()).getSExtValue() : 1;

	switch (stm->getType()) {
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
		context.addError("undefined loop branch type: " + to_string(stm->getType()), stm->getToken());
		return;
	}
	BranchInst::Create(block, context);
	context.pushBlock(context.createBlock());
	return;
error:
	context.addError(typeName + " invalid outside a loop/switch block", stm->getToken());
}

void CGNStatement::visitNDeleteStatement(NDeleteStatement* stm)
{
	static string freeName = "free";

	auto bytePtr = SType::getPointer(context, SType::getInt(context, 8));
	auto func = context.loadSymbol(freeName);
	if (!func) {
		vector<SType*> args;
		args.push_back(bytePtr);
		auto retType = SType::getVoid(context);
		auto funcType = SType::getFunction(context, retType, args);

		func = Builder::CreateFunction(context, freeName, funcType);
	} else if (!func.isFunction()) {
		context.addError("Compiler Error: free not function", stm->getToken());
		return;
	}

	auto ptr = CGNExpression::run(context, stm->getVar());
	if (!ptr) {
		return;
	} else if (!ptr.stype()->isPointer()) {
		context.addError("delete requires pointer type", stm->getToken());
		return;
	}

	if (ptr.stype()->subType()->isClass())
		Inst::CallDestructor(context, ptr, stm->getToken());

	vector<Value*> exp_list;
	exp_list.push_back(new BitCastInst(ptr, *bytePtr, "", context));

	CallInst::Create(static_cast<SFunction&>(func), exp_list, "", context);
}

void CGNStatement::visitNDestructorCall(NDestructorCall* stm)
{
	auto value = CGNVariable::run(context, stm->getVar());
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
			context.addError("calling destructor only valid for classes", stm->getToken());
			return;
		}
	}

	Inst::CallDestructor(context, value, stm->getToken());
}
