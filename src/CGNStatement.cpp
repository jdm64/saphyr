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
#include <llvm/Support/Casting.h>
#include "Value.h"
#include "AST.h"
#include "CGNStatement.h"
#include "parserbase.h"
#include "CodeContext.h"
#include "Instructions.h"
#include "Builder.h"
#include "CGNDataType.h"
#include "CGNVariable.h"
#include "CGNExpression.h"
#include "CGNImportStm.h"

void CGNStatement::visit(NStatement* stm)
{
	switch (stm->id()) {
	VISIT_CASE(NAliasDeclaration, stm)
	VISIT_CASE(NClassConstructor, stm)
	VISIT_CASE(NClassDeclaration, stm)
	VISIT_CASE(NClassDestructor, stm)
	VISIT_CASE(NClassFunctionDecl, stm)
	VISIT_CASE(NClassStructDecl, stm)
	VISIT_CASE(NDeleteStatement, stm)
	VISIT_CASE(NDestructorCall, stm)
	VISIT_CASE(NEnumDeclaration, stm)
	VISIT_CASE(NExpressionStm, stm)
	VISIT_CASE(NForStatement, stm)
	VISIT_CASE(NFunctionDeclaration, stm)
	VISIT_CASE(NGlobalVariableDecl, stm)
	VISIT_CASE(NGotoStatement, stm)
	VISIT_CASE(NIfStatement, stm)
	VISIT_CASE(NImportFileStm, stm)
	VISIT_CASE(NImportPkgStm, stm)
	VISIT_CASE(NLabelStatement, stm)
	VISIT_CASE(NLoopBranch, stm)
	VISIT_CASE(NLoopStatement, stm)
	VISIT_CASE(NMemberInitializer, stm)
	VISIT_CASE(NParameter, stm)
	VISIT_CASE(NReturnStatement, stm)
	VISIT_CASE(NStructDeclaration, stm)
	VISIT_CASE(NSwitchStatement, stm)
	VISIT_CASE(NVariableDecl, stm)
	VISIT_CASE(NVariableDeclGroup, stm)
	VISIT_CASE(NWhileStatement, stm)
	default:
		context.addError("NodeId::" + to_string(static_cast<int>(stm->id())) + " unrecognized in CGNStatement", nullptr);
	}
}

void CGNStatement::visit(NStatementList* list)
{
	for (auto item : *list)
		visit(item);
}

void CGNStatement::visitNImportFileStm(NImportFileStm* stm)
{
	Builder::LoadImport(context, stm);
}

void CGNStatement::visitNImportPkgStm(NImportPkgStm* stm)
{
	Builder::LoadImport(context, stm);
}

void CGNStatement::visitNExpressionStm(NExpressionStm* stm)
{
	CGNExpression::run(context, stm->getExp());
}

void CGNStatement::visitNParameter(NParameter* stm)
{
	auto stype = CGNDataType::run(context, stm->getType());
	auto stackAlloc = context.IB().CreateAlloca(*stype);
	context.IB().CreateStore(storedValue, stackAlloc);
	context.storeLocalSymbol({stackAlloc, stype}, stm->getName()->str, true);
}

void CGNStatement::visitNVariableDecl(NVariableDecl* stm)
{
	auto initList = CGNExpression::collect(context, stm->getInitList());
	if (!initList) {
		auto initExp = stm->getInitExp();
		if (initExp) {
			auto initValue = CGNExpression::run(context, initExp, false);
			initList = std::make_unique<VecRValue>();
			initList->push_back(initValue);
		}
	}

	auto varType = CGNDataType::run(context, stm->getType());
	if (!varType) {
		return;
	} else if (varType->isAuto()) {
		if (!initList || initList->size() != 1) { // auto type requires initialization
			context.addError("auto variable type requires initialization", *stm->getType());
			return;
		}
		varType = initList->at(0).stype();
		if (!varType) {
			return;
		}
	} else if (varType->isReference()) {
		if (!initList || initList->size() != 1) { // reference type requires initialization
			context.addError("reference variable type requires initialization", *stm->getType());
			return;
		} else if (varType->subType()->isAuto()) {
			auto isCRef = varType->isCopyRef();
			varType = initList->at(0).stype();
			if (!varType) {
				return;
			}
			varType = isCRef ? SType::getCopyRef(context, varType) : SType::getReference(context, varType);
		}
	} else if (varType->isUnsized()) {
		context.addError("can't create variable for an unsized type: " + varType->str(context), *stm->getType());
		return;
	} else if (!SType::validate(context, stm->getName(), varType)) {
		return;
	}

	if (initList && initList->size() == 1 && initList->at(0) && initList->at(0).stype()->isReference() && !varType->isPointer()) {
		initList->at(0) = Inst::Deref(context, initList->at(0));
	}

	auto name = stm->getName()->str;
	if (context.loadSymbolCurr(name).size()) {
		context.addError("variable " + name + " already defined", stm->getName());
		return;
	}

	auto var = RValue(context.IB().CreateAlloca(*varType, nullptr, name), varType);
	context.storeLocalSymbol(var, name);

	Inst::InitVariable(context, var, {}, initList.get(), stm->getName());
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
	if (Builder::StoreTemplate(context, stm))
		return;

	Builder::CreateStruct(context, stm->getType(), stm->getName(), stm->getVars());
}

void CGNStatement::visitNEnumDeclaration(NEnumDeclaration* stm)
{
	Builder::CreateEnum(context, stm);
}

void CGNStatement::visitNFunctionDeclaration(NFunctionDeclaration* stm)
{
	Builder::CreateFunction(context, stm->getName(), stm->getRType(), stm->getParams(), stm->getBody(), stm->getAttrs());
}

void CGNStatement::visitNClassStructDecl(NClassStructDecl* stm)
{
	auto cl = stm->getClass();
	auto stToken = cl->getName();
	auto stType = NStructDeclaration::CreateType::CLASS;
	Builder::CreateStruct(context, stType, stToken, stm->getVarList());
}

void CGNStatement::visitNClassFunctionDecl(NClassFunctionDecl* stm)
{
	Builder::CreateClassFunction(context, stm, false);
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

	auto item = currClass->getItem(stm->getName()->str);
	if (!item) {
		context.addError("invalid initializer, class variable not defined: " + stm->getName()->str, stm->getName());
		return;
	}

	auto var = Inst::LoadMemberVar(context, stm->getName()->str);
	auto exp = CGNExpression::collect(context, stm->getExp());
	Inst::InitVariable(context, var, {}, exp.get(), stm->getName());
}

void CGNStatement::visitNClassDeclaration(NClassDeclaration* stm)
{
	if (Builder::StoreTemplate(context, stm))
		return;

	Builder::CreateClass(context, stm, [this, stm](size_t structIdx) {
		visit(stm->getMembers()->at(structIdx));
		if (!context.getClass())
			return;


		auto errorCount = context.errorCount();
		for (size_t i = 0; i < stm->getMembers()->size(); i++) {
			if (i == structIdx)
				continue;
			CGNImportStm::run(context, stm->getMembers()->at(i));
		}
		if (errorCount != context.errorCount())
			return;
		for (size_t i = 0; i < stm->getMembers()->size(); i++) {
			if (i == structIdx)
				continue;
			visit(stm->getMembers()->at(i));
		}
	});
}

void CGNStatement::visitNReturnStatement(NReturnStatement* stm)
{
	auto func = context.currFunction();
	auto funcReturn = func.returnTy();

	if (funcReturn->isVoid()) {
		if (stm->getValue()) {
			context.addError("function " + func.name().str() + " declared void, but non-void return found", *stm->getValue());
			return;
		}
	} else if (!stm->getValue()) {
		context.addError("function " + func.name().str() + " declared non-void, but void return found", *stm);
		return;
	}
	auto returnVal = CGNExpression::run(context, stm->getValue());
	RValue retAlloc;
	if (returnVal) {
		Inst::CastTo(context, *stm->getValue(), returnVal, funcReturn);
		retAlloc = Inst::PtrOfLoad(context, returnVal);
	}

	Inst::CallDestructables(context, retAlloc.value(), *stm);

	context.IB().CreateRet(returnVal);

	context.pushBlock(context.createBlock());
}

void CGNStatement::visitNLoopStatement(NLoopStatement* stm)
{
	auto bodyBlock = context.createContinueBlock();
	auto endBlock = context.createBreakBlock();

	context.IB().CreateBr(bodyBlock);
	context.pushBlock(bodyBlock);

	context.pushLocalTable();
	visit(stm->getBody());
	context.popLocalTable();

	context.IB().CreateBr(bodyBlock);
	context.pushBlock(endBlock);

	context.popLoopBranchBlocks(BranchType::BREAK | BranchType::CONTINUE);
}

void CGNStatement::visitNWhileStatement(NWhileStatement* stm)
{
	auto condBlock = context.createContinueBlock();
	auto bodyBlock = context.createRedoBlock();
	auto endBlock = context.createBreakBlock();

	auto startBlock = stm->doWhile()? bodyBlock : condBlock;
	auto trueBlock = stm->until()? endBlock : bodyBlock;
	auto falseBlock = stm->until()? bodyBlock : endBlock;

	context.IB().CreateBr(startBlock);
	context.pushBlock(condBlock);

	context.pushLocalTable();
	Inst::Branch(trueBlock, falseBlock, stm->getCond(), context);
	context.pushBlock(bodyBlock);
	visit(stm->getBody());
	context.popLocalTable();

	context.IB().CreateBr(condBlock);
	context.pushBlock(endBlock);

	context.popLoopBranchBlocks(BranchType::BREAK | BranchType::CONTINUE | BranchType::REDO);
}

void CGNStatement::visitNSwitchStatement(NSwitchStatement* stm)
{
	auto switchValue = CGNExpression::run(context, stm->getValue());
	if (!switchValue)
		return;

	if (!switchValue.stype()->isInteger()) {
		context.addError("switch requires int type", *stm->getValue());
		return;
	}

	auto caseBlock = context.createBlock();
	auto endBlock = context.createBreakBlock();
	auto defaultBlock = endBlock;
	auto switchInst = context.IB().CreateSwitch(switchValue, defaultBlock, stm->getCases()->size());

	context.pushLocalTable();

	set<int64_t> unique;
	bool hasDefault = false;
	for (auto caseItem : *stm->getCases()) {
		if (caseItem->isValueCase()) {
			auto caseVal = CGNExpression::run(context, caseItem->getValue());
			if (!caseVal || !isa<ConstantInt>(caseVal.value())) {
				context.addError("case value must be a constant int", *caseItem->getValue());
			} else {
				Inst::CastTo(context, *caseItem->getValue(), caseVal, switchValue.stype());
				auto val = static_cast<ConstantInt*>(caseVal.value());
				if (!unique.insert(val->getSExtValue()).second)
					context.addError("switch case values are not unique", *caseItem->getValue());
				switchInst->addCase(val, caseBlock);
			}
		} else {
			if (hasDefault)
				context.addError("switch statement has more than one default", *caseItem);
			hasDefault = true;
			defaultBlock = caseBlock;
		}

		context.pushBlock(caseBlock);
		visit(caseItem->getBody());

		if (caseItem->isLastStmBranch()) {
			caseBlock = context.currBlock();
		} else {
			caseBlock = context.createBlock();
			context.IB().CreateBr(caseBlock);
			context.pushBlock(caseBlock);
		}
	}
	switchInst->setDefaultDest(defaultBlock);

	context.popLocalTable();

	// NOTE: the last case will create a dangling block which needs a terminator.
	context.IB().CreateBr(endBlock);

	context.popLoopBranchBlocks(BranchType::BREAK);
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
	context.IB().CreateBr(condBlock);

	context.pushBlock(condBlock);
	Inst::Branch(bodyBlock, endBlock, stm->getCond(), context);

	context.pushBlock(bodyBlock);
	visit(stm->getBody());
	context.IB().CreateBr(postBlock);

	context.pushBlock(postBlock);
	CGNExpression::run(context, stm->getPostExp());

	context.popLocalTable();

	context.IB().CreateBr(condBlock);
	context.pushBlock(endBlock);

	context.popLoopBranchBlocks(BranchType::BREAK | BranchType::CONTINUE | BranchType::REDO);
}

void CGNStatement::visitNIfStatement(NIfStatement* stm)
{
	auto ifBlock = context.createBlock();
	auto elseBlock = context.createBlock();
	auto endBlock = stm->getElseBody()? context.createBlock() : elseBlock;

	context.pushLocalTable();

	Inst::Branch(ifBlock, elseBlock, stm->getCond(), context);

	context.pushBlock(ifBlock);
	visit(stm->getBody());
	context.popLocalTable();
	context.IB().CreateBr(endBlock);

	context.pushBlock(elseBlock);
	if (stm->getElseBody()) {
		context.pushLocalTable();
		visit(stm->getElseBody());
		context.popLocalTable();
		context.IB().CreateBr(endBlock);
	}
	context.pushBlock(endBlock);
}

void CGNStatement::visitNLabelStatement(NLabelStatement* stm)
{
	// check if label already declared
	auto labelName = stm->getName()->str;
	auto label = context.getLabelBlock(labelName);
	if (label) {
		if (!label->isPlaceholder) {
			context.addError("label " + labelName + " already defined", stm->getName());
			return;
		}
		// a used label is no longer a placeholder
		label->isPlaceholder = false;
	} else {
		label = context.createLabelBlock(stm->getName(), false);
	}
	context.IB().CreateBr(label->block);
	context.pushBlock(label->block);
}

void CGNStatement::visitNGotoStatement(NGotoStatement* stm)
{
	auto skip = context.createBlock();
	auto label = context.getLabelBlock(stm->getName()->str);
	if (!label) {
		// trying to jump to a non-existant label. create place holder and
		// later check if it's used at the end of the function.
		label = context.createLabelBlock(stm->getName(), true);
	}
	context.IB().CreateBr(label->block);
	context.pushBlock(skip);
}

void CGNStatement::visitNLoopBranch(NLoopBranch* stm)
{
	pair<BasicBlock*,size_t> block;
	string typeName;
	auto brLevel = 1;

	if (stm->getLevel()) {
		auto levelVal = CGNExpression::run(context, stm->getLevel());
		if (!levelVal) {
			return;
		} else if (!isa<ConstantInt>(levelVal.value())) {
			context.addError("branch level must be constant int", *stm->getLevel());
			return;
		}
		brLevel = static_cast<ConstantInt*>(levelVal.value())->getSExtValue();
	}

	switch (stm->getType()) {
	case ParserBase::TT_CONTINUE:
		block = context.getContinueBlock(brLevel);
		if (!block.first) {
			typeName = "continue";
			goto error;
		}
		break;
	case ParserBase::TT_REDO:
		block = context.getRedoBlock(brLevel);
		if (!block.first) {
			typeName = "redo";
			goto error;
		}
		break;
	case ParserBase::TT_BREAK:
		block = context.getBreakBlock(brLevel);
		if (!block.first) {
			typeName = "break";
			goto error;
		}
		break;
	default:
		context.addError("undefined loop branch type: " + to_string(stm->getType()), *stm);
		return;
	}

	Inst::CallDestructables(context, nullptr, *stm, block.second);
	context.IB().CreateBr(block.first);
	context.pushBlock(context.createBlock());
	return;
error:
	context.addError(typeName + " invalid outside a loop/switch block", *stm);
}

void CGNStatement::visitNDeleteStatement(NDeleteStatement* stm)
{
	Token* varTok = *stm->getVar();
	auto arrSize = CGNExpression::run(context, stm->getArrSize());
	auto ptr = CGNExpression::run(context, stm->getVar());
	if (!ptr) {
		return;
	} else if (!ptr.stype()->isPointer()) {
		context.addError("delete requires pointer type", varTok);
		return;
	}

	Inst::CallDestructor(context, ptr, arrSize, varTok);

	auto func = Builder::getBuiltinFunc(context, varTok, BuiltinFuncType::Free);
	auto bitCast = context.IB().CreateBitCast(ptr, *func.getParam(0));

	if (context.config().count("print-debug"))
		Builder::AddDebugPrint(context, varTok, "[DEBUG] free(%i) at " + varTok->getLoc(), {bitCast});

	context.IB().CreateCall(func.funcType(), func.value(), {bitCast});
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
			context.addError("calling destructor only valid for classes", stm->getThisToken());
			return;
		}
	}

	Inst::CallDestructor(context, value, {}, stm->getThisToken());
}
