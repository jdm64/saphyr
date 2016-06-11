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

const string NExprVariable::STR_TMP_EXP = "temp expression";

RValue NVariable::genValue(CodeContext& context, RValue var)
{
	return Inst::Load(context, var);
}

RValue NBaseVariable::loadVar(CodeContext& context)
{
	auto varName = getName();

	// check current function
	auto var = context.loadSymbolLocal(varName);
	if (var)
		return var;

	// check class variables
	auto currClass = context.getClass();
	if (currClass) {
		auto item = currClass->getItem(varName);
		if (item) {
			return Inst::LoadMemberVar(context, varName);
		}
	}

	// check global variables
	var = context.loadSymbolGlobal(varName);
	if (var)
		return var;

	// check enums
	auto userVar = SUserType::lookup(context, varName);
	if (!userVar) {
		context.addError("variable " + varName + " not declared", name);
		return var;
	}
	return RValue(ConstantInt::getFalse(context), userVar);
}

RValue NArrayVariable::loadVar(CodeContext& context)
{
	auto indexVal = index->genValue(context);

	if (!indexVal) {
		return indexVal;
	} else if (!indexVal.stype()->isNumeric()) {
		context.addError("array index is not able to be cast to an int", brackTok);
		return RValue();
	}

	auto var = arrVar->loadVar(context);
	if (!var)
		return var;
	var = Inst::Deref(context, var, true);

	if (!var.stype()->isSequence()) {
		context.addError("variable " + getName() + " is not an array or vec", brackTok);
		return RValue();
	}
	Inst::CastTo(context, brackTok, indexVal, SType::getInt(context, 64));

	vector<Value*> indexes;
	indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
	indexes.push_back(indexVal);

	return Inst::GetElementPtr(context, var, indexes, var.stype()->subType());
}

RValue NMemberVariable::loadVar(CodeContext& context)
{
	auto var = baseVar->loadVar(context);
	if (!var)
		return RValue();

	var = Inst::Deref(context, var, true);
	return Inst::LoadMemberVar(context, getName(), var, dotToken, memberName);
}

RValue NDereference::loadVar(CodeContext& context)
{
	auto var = derefVar->loadVar(context);
	if (!var) {
		return var;
	} else if (!var.stype()->isPointer()) {
		context.addError("variable " + getName() + " can not be dereferenced", atTok);
		return RValue();
	}
	return Inst::Deref(context, var);
}

RValue NAddressOf::loadVar(CodeContext& context)
{
	return addVar->loadVar(context);
}

void NParameter::genCode(CodeContext& context)
{
	auto stype = CGNDataType::run(context, type);
	auto stackAlloc = new AllocaInst(*stype, "", context);
	new StoreInst(arg, stackAlloc, context);
	context.storeLocalSymbol({stackAlloc, stype}, getName());
}

void NVariableDecl::genCode(CodeContext& context)
{
	auto initValue = initExp? initExp->genValue(context) : RValue();
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
	auto returnVal = value? value->genValue(context) : RValue();
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

ConstantInt* NSwitchCase::getValue(CodeContext& context)
{
	return ConstantInt::get(context, CGNInt::run(context, value));
}

void NSwitchStatement::genCode(CodeContext& context)
{
	auto switchValue = value->genValue(context);
	Inst::CastTo(context, lparen, switchValue, SType::getInt(context, 32));

	auto caseBlock = context.createBlock();
	auto endBlock = context.createBreakBlock(), defaultBlock = endBlock;
	auto switchInst = SwitchInst::Create(switchValue, defaultBlock, cases->size(), context);

	context.pushLocalTable();

	set<int64_t> unique;
	bool hasDefault = false;
	for (auto caseItem : *cases) {
		if (caseItem->isValueCase()) {
			auto val = caseItem->getValue(context);
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
	postExp->genCode(context);
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

	auto ptr = variable->genValue(context);
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
	auto value = baseVar->loadVar(context);
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

RValue NAssignment::genValue(CodeContext& context)
{
	auto lhsVar = lhs->loadVar(context);

	if (!lhsVar)
		return RValue();

	BasicBlock* endBlock = nullptr;

	if (oper == ParserBase::TT_DQ_MARK) {
		auto condExp = Inst::Load(context, lhsVar);
		Inst::CastTo(context, opTok, condExp, SType::getBool(context));

		auto trueBlock = context.createBlock();
		endBlock = context.createBlock();

		BranchInst::Create(endBlock, trueBlock, condExp, context);
		context.pushBlock(trueBlock);
	}

	auto rhsExp = rhs->genValue(context);
	if (!rhsExp)
		return RValue();

	if (oper != '=' && oper != ParserBase::TT_DQ_MARK) {
		auto lhsLocal = Inst::Load(context, lhsVar);
		rhsExp = Inst::BinaryOp(oper, opTok, lhsLocal, rhsExp, context);
	}
	Inst::CastTo(context, opTok, rhsExp, lhsVar.stype());
	new StoreInst(rhsExp, lhsVar, context);

	if (oper == ParserBase::TT_DQ_MARK) {
		BranchInst::Create(endBlock, context);
		context.pushBlock(endBlock);
	}

	return rhsExp;
}

RValue NTernaryOperator::genValue(CodeContext& context)
{
	auto condExp = condition->genValue(context);
	Inst::CastTo(context, colTok, condExp, SType::getBool(context));

	RValue trueExp, falseExp, retVal;
	if (trueVal->isComplex() || falseVal->isComplex()) {
		auto trueBlock = context.createBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(trueBlock, falseBlock, condExp, context);

		context.pushBlock(trueBlock);
		trueExp = trueVal->genValue(context);
		BranchInst::Create(endBlock, context);

		context.pushBlock(falseBlock);
		falseExp = falseVal->genValue(context);
		BranchInst::Create(endBlock, context);

		context.pushBlock(endBlock);
		auto result = PHINode::Create(trueExp.type(), 2, "", context);
		result->addIncoming(trueExp, trueBlock);
		result->addIncoming(falseExp, falseBlock);
		retVal = RValue(result, trueExp.stype());
	} else {
		trueExp = trueVal->genValue(context);
		falseExp = falseVal->genValue(context);
		auto select = SelectInst::Create(condExp, trueExp, falseExp, "", context);
		retVal = RValue(select, trueExp.stype());
	}

	if (trueExp.stype() != falseExp.stype())
		context.addError("return types of ternary must match", colTok);
	return retVal;
}

RValue NNewExpression::genValue(CodeContext& context)
{
	static string mallocName = "malloc";

	auto funcVal = context.loadSymbol(mallocName);
	if (!funcVal) {
		vector<SType*> args;
		args.push_back(SType::getInt(context, 64));
		auto retType = SType::getPointer(context, SType::getInt(context, 8));
		auto funcType = SType::getFunction(context, retType, args);

		funcVal = SFunction::create(context, mallocName, funcType);
	} else if (!funcVal.isFunction()) {
		context.addError("Compiler Error: malloc not function", token);
		return RValue();
	}

	auto nType = CGNDataType::run(context, type);
	if (!nType) {
		return RValue();
	} else if (nType->isAuto()) {
		context.addError("can't call new on auto type", token);
		return RValue();
	} else if (nType->isVoid()) {
		context.addError("can't call new on void type", token);
		return RValue();
	}

	vector<Value*> exp_list;
	exp_list.push_back(Inst::SizeOf(context, token, nType));

	auto func = static_cast<SFunction&>(funcVal);
	auto call = CallInst::Create(func, exp_list, "", context);
	auto ptr = RValue(call, func.returnTy());
	auto ptrType = SType::getPointer(context, nType);

	return RValue(new BitCastInst(ptr, *ptrType, "", context), ptrType);
}

RValue NLogicalOperator::genValue(CodeContext& context)
{
	auto saveBlock = context.currBlock();
	auto firstBlock = context.createBlock();
	auto secondBlock = context.createBlock();
	auto trueBlock = (oper == ParserBase::TT_LOG_AND)? firstBlock : secondBlock;
	auto falseBlock = (oper == ParserBase::TT_LOG_AND)? secondBlock : firstBlock;

	auto lhsExp = Inst::Branch(trueBlock, falseBlock, lhs, opTok, context);

	context.pushBlock(firstBlock);
	auto rhsExp = rhs->genValue(context);
	Inst::CastTo(context, opTok, rhsExp, SType::getBool(context));
	BranchInst::Create(secondBlock, context);

	context.pushBlock(secondBlock);
	auto result = PHINode::Create(Type::getInt1Ty(context), 2, "", context);
	result->addIncoming(lhsExp, saveBlock);
	result->addIncoming(rhsExp, firstBlock);

	return RValue(result, SType::getBool(context));
}

RValue NCompareOperator::genValue(CodeContext& context)
{
	auto lhsExp = lhs->genValue(context);
	auto rhsExp = rhs->genValue(context);

	return Inst::Cmp(oper, opTok, lhsExp, rhsExp, context);
}

RValue NBinaryMathOperator::genValue(CodeContext& context)
{
	auto lhsExp = lhs->genValue(context);
	auto rhsExp = rhs->genValue(context);

	return Inst::BinaryOp(oper, opTok, lhsExp, rhsExp, context);
}

RValue NNullCoalescing::genValue(CodeContext& context)
{
	RValue rhsExp, retVal;
	auto lhsExp = lhs->genValue(context);
	auto condition = lhsExp;

	Inst::CastTo(context, opTok, condition, SType::getBool(context), false);
	if (rhs->isComplex()) {
		auto trueBlock = context.currBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(endBlock, falseBlock, condition, context);

		context.pushBlock(falseBlock);
		rhsExp = rhs->genValue(context);
		BranchInst::Create(endBlock, context);

		context.pushBlock(endBlock);
		auto result = PHINode::Create(lhsExp.type(), 2, "", context);
		result->addIncoming(lhsExp, trueBlock);
		result->addIncoming(rhsExp, falseBlock);

		retVal = RValue(result, lhsExp.stype());
	} else {
		rhsExp = rhs->genValue(context);
		auto select = SelectInst::Create(condition, lhsExp, rhsExp, "", context);
		retVal = RValue(select, lhsExp.stype());
	}

	if (lhsExp.stype() != rhsExp.stype())
		context.addError("return types of null coalescing operator must match", opTok);
	return retVal;
}

RValue NSizeOfOperator::genValue(CodeContext& context)
{
	switch (type) {
	case DATA:
		return Inst::SizeOf(context, sizeTok, dtype);
	case EXP:
		return Inst::SizeOf(context, sizeTok, exp);
	case NAME:
		return Inst::SizeOf(context, sizeTok, name->str);
	default:
		// shouldn't happen
		return RValue();
	}
}

RValue NUnaryMathOperator::genValue(CodeContext& context)
{
	auto unaryExp = unary->genValue(context);
	auto type = unaryExp.stype();

	switch (oper) {
	case '+':
	case '-':
		return Inst::BinaryOp(oper, opTok, RValue::getZero(context, type), unaryExp, context);
	case '!':
		return Inst::Cmp(ParserBase::TT_EQ, opTok, RValue::getZero(context, type), unaryExp, context);
	case '~':
		return Inst::BinaryOp('^', opTok, RValue::getAllOne(context, type), unaryExp, context);
	default:
		context.addError("invalid unary operator " + to_string(oper), opTok);
		return RValue();
	}
}

RValue NFunctionCall::genValue(CodeContext& context)
{
	auto funcName = getName();
	auto sym = context.loadSymbol(funcName);
	if (!sym) {
		context.addError("symbol " + funcName + " not defined", name);
		return sym;
	}
	auto deSym = Inst::Deref(context, sym, true);
	if (!deSym.isFunction()) {
		context.addError("symbol " + funcName + " doesn't reference a function", name);
		return RValue();
	}

	auto func = static_cast<SFunction&>(deSym);
	vector<Value*> exp_list;
	return Inst::CallFunction(context, func, name, arguments, exp_list);
}

RValue NFunctionCall::loadVar(CodeContext& context)
{
	auto value = genValue(context);
	if (!value)
		return RValue();
	auto stackAlloc = new AllocaInst(value.type(), "", context);
	new StoreInst(value, stackAlloc, context);
	return RValue(stackAlloc, value.stype());
}

RValue NMemberFunctionCall::genValue(CodeContext& context)
{
	return Inst::CallMemberFunction(context, baseVar, funcName, arguments);
}

RValue NMemberFunctionCall::loadVar(CodeContext& context)
{
	auto value = genValue(context);
	if (!value)
		return value;
	auto stackAlloc = new AllocaInst(value.type(), "", context);
	new StoreInst(value, stackAlloc, context);
	return RValue(stackAlloc, value.stype());
}

RValue NIncrement::genValue(CodeContext& context)
{
	auto varPtr = variable->loadVar(context);
	auto varVal = Inst::Load(context, varPtr);
	if (!varVal)
		return RValue();

	auto type = varVal.stype();
	if (type->isPointer() && type->subType()->isFunction()) {
		context.addError("Increment/Decrement invalid for function pointer", opTok);
		return RValue();
	} else if (type->isEnum()) {
		context.addError("Increment/Decrement invalid for enum type", opTok);
		return RValue();
	}
	auto incType = type->isPointer()? SType::getInt(context, 32) : type;

	auto result = Inst::BinaryOp(oper, opTok, varVal, RValue::getNumVal(context, incType, oper == ParserBase::TT_INC? 1:-1), context);
	new StoreInst(result, varPtr, context);

	return isPostfix? varVal : RValue(result, type);
}

RValue NIntLikeConst::genValue(CodeContext& context)
{
	return RValue::getValue(context, CGNInt::run(context, this));
}

RValue NNullPointer::genValue(CodeContext& context)
{
	return RValue::getNullPtr(context, SType::getInt(context, 8));
}

RValue NStringLiteral::genValue(CodeContext& context)
{
	auto strVal = getStrVal();
	auto arrData = ConstantDataArray::getString(context, strVal, true);
	auto arrTy = SType::getArray(context, SType::getInt(context, 8), strVal.size() + 1);
	auto arrTyPtr = SType::getPointer(context, arrTy);
	auto gVar = new GlobalVariable(*context.getModule(), *arrTy, true, GlobalValue::PrivateLinkage, arrData);
	auto zero = ConstantInt::get(*SType::getInt(context, 32), 0);

	std::vector<Constant*> idxs;
	idxs.push_back(zero);

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 7
	auto strPtr = ConstantExpr::getGetElementPtr(gVar->getType(), gVar, idxs);
#else
	auto strPtr = ConstantExpr::getGetElementPtr(gVar, idxs);
#endif

	return RValue(strPtr, arrTyPtr);
}

RValue NFloatConst::genValue(CodeContext& context)
{
	static const map<string, SType*> suffix = {
		{"f", SType::getFloat(context)},
		{"d", SType::getFloat(context, true)} };
	auto type = SType::getFloat(context, true);

	auto data = NConstant::getValueAndSuffix(getStrVal());
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid float suffix: " + data[1], value);
		else
			type = suf->second;
	}
	auto fp = ConstantFP::get(*type, data[0]);
	return RValue(fp, type);
}
