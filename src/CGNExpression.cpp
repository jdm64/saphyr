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
#include "Value.h"
#include "AST.h"
#include "CGNExpression.h"
#include "CodeContext.h"
#include "parserbase.h"
#include "Instructions.h"
#include "CGNInt.h"
#include "CGNDataType.h"
#include "CGNVariable.h"
#include "Builder.h"

#define TABLE_ADD(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartExpression)] = reinterpret_cast<classPtr>(&CGNExpression::visit##ID)
#define TABLE_ADD2(ID, FUNC) table[NODEID_DIFF(NodeId::ID, NodeId::StartExpression)] = reinterpret_cast<classPtr>(&CGNExpression::visit##FUNC)

CGNExpression::classPtr* CGNExpression::buildVTable()
{
	auto table = new CGNExpression::classPtr[NODEID_DIFF(NodeId::EndExpression, NodeId::StartExpression)];
	TABLE_ADD2(NArrayVariable, NVariable);
	TABLE_ADD2(NBaseVariable, NVariable);
	TABLE_ADD2(NDereference, NVariable);
	TABLE_ADD2(NMemberVariable, NVariable);
	TABLE_ADD(NAddressOf);
	TABLE_ADD(NAssignment);
	TABLE_ADD(NBinaryMathOperator);
	TABLE_ADD(NBoolConst);
	TABLE_ADD(NCharConst);
	TABLE_ADD(NCompareOperator);
	TABLE_ADD(NFloatConst);
	TABLE_ADD(NFunctionCall);
	TABLE_ADD(NIncrement);
	TABLE_ADD(NIntConst);
	TABLE_ADD(NLogicalOperator);
	TABLE_ADD(NMemberFunctionCall);
	TABLE_ADD(NNewExpression);
	TABLE_ADD(NNullCoalescing);
	TABLE_ADD(NNullPointer);
	TABLE_ADD(NSizeOfOperator);
	TABLE_ADD(NStringLiteral);
	TABLE_ADD(NTernaryOperator);
	TABLE_ADD(NUnaryMathOperator);
	return table;
}

CGNExpression::classPtr* CGNExpression::vtable = CGNExpression::buildVTable();

RValue CGNExpression::visit(NExpression* exp)
{
	if (!exp)
		return RValue();
	return (this->*vtable[NODEID_DIFF(exp->id(), NodeId::StartExpression)])(exp);
}

void CGNExpression::visit(NExpressionList* list)
{
	for (auto item : *list)
		visit(item);
}

RValue CGNExpression::visitNVariable(NVariable* nVar)
{
	auto var = CGNVariable::run(context, nVar);
	return Inst::Load(context, var);
}

RValue CGNExpression::visitNAddressOf(NAddressOf* nVar)
{
	auto var = CGNVariable::run(context, nVar);
	return var? RValue(var, SType::getPointer(context, var.stype())) : var;
}

RValue CGNExpression::visitNAssignment(NAssignment* exp)
{
	auto lhsVar = CGNVariable::run(context, exp->getLhs());

	if (!lhsVar)
		return RValue();

	BasicBlock* endBlock = nullptr;

	if (exp->getOp() == ParserBase::TT_DQ_MARK) {
		auto condExp = Inst::Load(context, lhsVar);
		Inst::CastTo(context, *exp, condExp, SType::getBool(context));

		auto trueBlock = context.createBlock();
		endBlock = context.createBlock();

		BranchInst::Create(endBlock, trueBlock, condExp, context);
		context.pushBlock(trueBlock);
	}

	auto rhsExp = visit(exp->getRhs());
	if (!rhsExp)
		return RValue();

	if (exp->getOp() != '=' && exp->getOp() != ParserBase::TT_DQ_MARK) {
		auto lhsLocal = Inst::Load(context, lhsVar);
		rhsExp = Inst::BinaryOp(exp->getOp(), *exp, lhsLocal, rhsExp, context);
	}
	Inst::CastTo(context, *exp->getRhs(), rhsExp, lhsVar.stype());
	new StoreInst(rhsExp, lhsVar, context);

	if (exp->getOp() == ParserBase::TT_DQ_MARK) {
		BranchInst::Create(endBlock, context);
		context.pushBlock(endBlock);
	}

	return rhsExp;
}

RValue CGNExpression::visitNTernaryOperator(NTernaryOperator* exp)
{
	auto condExp = visit(exp->getCondition());
	Inst::CastTo(context, *exp->getCondition(), condExp, SType::getBool(context));

	RValue trueExp, falseExp, retVal;
	if (exp->getTrueVal()->isComplex() || exp->getFalseVal()->isComplex()) {
		auto trueBlock = context.createBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(trueBlock, falseBlock, condExp, context);

		context.pushBlock(trueBlock);
		trueExp = visit(exp->getTrueVal());
		BranchInst::Create(endBlock, context);

		context.pushBlock(falseBlock);
		falseExp = visit(exp->getFalseVal());
		BranchInst::Create(endBlock, context);

		context.pushBlock(endBlock);
		auto result = PHINode::Create(trueExp.type(), 2, "", context);
		result->addIncoming(trueExp, trueBlock);
		result->addIncoming(falseExp, falseBlock);
		retVal = RValue(result, trueExp.stype());
	} else {
		trueExp = visit(exp->getTrueVal());
		falseExp = visit(exp->getFalseVal());
		auto select = SelectInst::Create(condExp, trueExp, falseExp, "", context);
		retVal = RValue(select, trueExp.stype());
	}

	if (trueExp.stype() != falseExp.stype())
		context.addError("return types of ternary must match", *exp);
	return retVal;
}

RValue CGNExpression::visitNNewExpression(NNewExpression* exp)
{
	auto funcVal = context.loadSymbol("malloc");
	if (!funcVal) {
		vector<SType*> args;
		args.push_back(SType::getInt(context, 64));
		auto retType = SType::getPointer(context, SType::getInt(context, 8));
		auto funcType = SType::getFunction(context, retType, args);
		Token mallocName("malloc");

		funcVal = Builder::getFuncPrototype(context, &mallocName, funcType);
	} else if (!funcVal.isFunction()) {
		context.addError("Compiler Error: malloc not function", *exp);
		return RValue();
	}

	RValue size;
	auto nType = CGNDataTypeNew::run(context, exp->getType(), size);
	if (!nType) {
		return RValue();
	} else if (nType->isUnsized()) {
		context.addError("can't call new on " + nType->str(&context) + " type", *exp->getType());
		return RValue();
	}

	vector<Value*> exp_list;
	exp_list.push_back(size);

	auto func = static_cast<SFunction&>(funcVal);
	auto call = CallInst::Create(func, exp_list, "", context);
	auto ptr = RValue(call, func.returnTy());
	auto ptrType = SType::getPointer(context, nType);
	auto rPtr = RValue(new BitCastInst(ptr, *ptrType, "", context), ptrType);

	// setup type so the variable is initialized using the base type
	RValue tmp, ptr2 = RValue(rPtr.value(), nType);
	Inst::InitVariable(context, ptr2, *exp->getType(), exp->getArgs(), tmp);

	return rPtr;
}

RValue CGNExpression::visitNLogicalOperator(NLogicalOperator* exp)
{
	auto saveBlock = context.currBlock();
	auto firstBlock = context.createBlock();
	auto secondBlock = context.createBlock();
	auto trueBlock = (exp->getOp() == ParserBase::TT_LOG_AND)? firstBlock : secondBlock;
	auto falseBlock = (exp->getOp() == ParserBase::TT_LOG_AND)? secondBlock : firstBlock;

	auto lhsExp = Inst::Branch(trueBlock, falseBlock, exp->getLhs(), context);

	context.pushBlock(firstBlock);
	auto rhsExp = visit(exp->getRhs());
	Inst::CastTo(context, *exp->getRhs(), rhsExp, SType::getBool(context));
	BranchInst::Create(secondBlock, context);

	context.pushBlock(secondBlock);
	auto result = PHINode::Create(Type::getInt1Ty(context), 2, "", context);
	result->addIncoming(lhsExp, saveBlock);
	result->addIncoming(rhsExp, firstBlock);

	return RValue(result, SType::getBool(context));
}

RValue CGNExpression::visitNCompareOperator(NCompareOperator* exp)
{
	auto lhsExp = visit(exp->getLhs());
	auto rhsExp = visit(exp->getRhs());

	return Inst::Cmp(exp->getOp(), *exp, lhsExp, rhsExp, context);
}

RValue CGNExpression::visitNBinaryMathOperator(NBinaryMathOperator* exp)
{
	auto lhsExp = visit(exp->getLhs());
	auto rhsExp = visit(exp->getRhs());

	return Inst::BinaryOp(exp->getOp(), *exp, lhsExp, rhsExp, context);
}

RValue CGNExpression::visitNNullCoalescing(NNullCoalescing* exp)
{
	RValue rhsExp, retVal;
	auto lhsExp = visit(exp->getLhs());
	auto condition = lhsExp;

	Inst::CastTo(context, *exp->getLhs(), condition, SType::getBool(context), false);
	if (exp->getRhs()->isComplex()) {
		auto trueBlock = context.currBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(endBlock, falseBlock, condition, context);

		context.pushBlock(falseBlock);
		rhsExp = visit(exp->getRhs());
		BranchInst::Create(endBlock, context);

		context.pushBlock(endBlock);
		auto result = PHINode::Create(lhsExp.type(), 2, "", context);
		result->addIncoming(lhsExp, trueBlock);
		result->addIncoming(rhsExp, falseBlock);

		retVal = RValue(result, lhsExp.stype());
	} else {
		rhsExp = visit(exp->getRhs());
		auto select = SelectInst::Create(condition, lhsExp, rhsExp, "", context);
		retVal = RValue(select, lhsExp.stype());
	}

	if (lhsExp.stype() != rhsExp.stype())
		context.addError("return types of null coalescing operator must match", *exp);
	return retVal;
}

RValue CGNExpression::visitNSizeOfOperator(NSizeOfOperator* exp)
{
	switch (exp->getType()) {
	case NSizeOfOperator::DATA:
		return Inst::SizeOf(context, CGNDataType::run(context, exp->getDataType()), *exp);
	case NSizeOfOperator::EXP:
		if (exp->getExp()->id() == NodeId::NBaseVariable) {
			return Inst::SizeOf(context, *exp);
		}
		return Inst::SizeOf(context, CGNExpression::run(context, exp->getExp()).stype(), *exp);
	default:
		// shouldn't happen
		return RValue();
	}
}

RValue CGNExpression::visitNUnaryMathOperator(NUnaryMathOperator* exp)
{
	auto unaryExp = visit(exp->getExp());
	auto type = unaryExp.stype();

	switch (exp->getOp()) {
	case '+':
	case '-':
		return Inst::BinaryOp(exp->getOp(), *exp, RValue::getZero(context, type), unaryExp, context);
	case '!':
		return Inst::Cmp(ParserBase::TT_EQ, *exp, RValue::getZero(context, type), unaryExp, context);
	case '~':
		return Inst::BinaryOp('^', *exp, RValue::getAllOne(context, type), unaryExp, context);
	default:
		context.addError("invalid unary operator " + to_string(exp->getOp()), *exp);
		return RValue();
	}
}

RValue CGNExpression::visitNFunctionCall(NFunctionCall* exp)
{
	auto funcName = exp->getName()->str;
	auto sym = context.loadSymbol(funcName);
	if (!sym) {
		auto cl = context.getClass();
		if (cl) {
			auto item = cl->getItem(funcName);
			if (item) {
				if (context.currFunction().isStatic()) {
					auto classVal = RValue::getUndef(SType::getPointer(context, cl));
					return Inst::CallMemberFunctionClass(context, nullptr, classVal, exp->getName(), exp->getArguments());
				} else {
					unique_ptr<NBaseVariable> thisVar(new NBaseVariable(new Token("this")));
					return Inst::CallMemberFunction(context, thisVar.get(), exp->getName(), exp->getArguments());
				}
			}
		}
	}

	if (!sym) {
		context.addError("symbol " + funcName + " not defined", *exp);
		return sym;
	}
	auto deSym = Inst::Deref(context, sym, true);
	if (!deSym.isFunction()) {
		context.addError("symbol " + funcName + " doesn't reference a function", *exp);
		return RValue();
	}

	auto func = static_cast<SFunction&>(deSym);
	vector<Value*> exp_list;
	return Inst::CallFunction(context, func, exp->getName(), exp->getArguments(), exp_list);
}

RValue CGNExpression::visitNMemberFunctionCall(NMemberFunctionCall* exp)
{
	return Inst::CallMemberFunction(context, exp->getBaseVar(), exp->getName(), exp->getArguments());
}

RValue CGNExpression::visitNIncrement(NIncrement* exp)
{
	auto varPtr = CGNVariable::run(context, exp->getVar());
	auto varVal = Inst::Load(context, varPtr);
	if (!varVal)
		return RValue();

	auto type = varVal.stype();
	if (type->isPointer()) {
		auto subT = type->subType();
		if (subT->isFunction()) {
			context.addError("Increment/Decrement invalid for function pointer", *exp);
			return RValue();
		} else if (subT->isUnsized()) {
			context.addError("Increment/Decrement invalid for " + subT->str(&context) + " pointer", *exp);
			return RValue();
		}
	} else if (type->isEnum()) {
		context.addError("Increment/Decrement invalid for enum type", *exp);
		return RValue();
	}
	auto incType = type->isPointer()? SType::getInt(context, 32) : type;

	auto result = Inst::BinaryOp(exp->getOp(), *exp, varVal, RValue::getNumVal(context, incType, exp->getOp() == ParserBase::TT_INC? 1:-1), context);
	new StoreInst(result, varPtr, context);

	return exp->postfix()? varVal : RValue(result, type);
}

RValue CGNExpression::visitNBoolConst(NBoolConst* exp)
{
	return RValue::getValue(context, CGNInt::run(context, exp));
}

RValue CGNExpression::visitNNullPointer(NNullPointer* exp)
{
	return RValue::getNullPtr(context, SType::getInt(context, 8));
}

RValue CGNExpression::visitNStringLiteral(NStringLiteral* exp)
{
	auto strVal = exp->getStrVal();
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

RValue CGNExpression::visitNIntConst(NIntConst* exp)
{
	return RValue::getValue(context, CGNInt::run(context, exp));
}

RValue CGNExpression::visitNFloatConst(NFloatConst* exp)
{
	static const map<string, SType*> suffix = {
		{"f", SType::getFloat(context)},
		{"d", SType::getFloat(context, true)} };
	auto type = SType::getFloat(context, true);

	auto data = NConstant::getValueAndSuffix(exp->getStrVal());
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid float suffix: " + data[1], *exp);
		else
			type = suf->second;
	}
	auto fp = ConstantFP::get(*type, data[0]);
	return RValue(fp, type);
}

RValue CGNExpression::visitNCharConst(NCharConst* exp)
{
	return RValue::getValue(context, CGNInt::run(context, exp));
}
