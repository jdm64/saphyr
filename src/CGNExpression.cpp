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
#include <llvm/IR/CFG.h>
#include "Value.h"
#include "AST.h"
#include "CGNExpression.h"
#include "CodeContext.h"
#include "parserbase.h"
#include "Instructions.h"
#include "CGNDataType.h"
#include "CGNVariable.h"
#include "Builder.h"

RValue CGNExpression::visit(NExpression* exp)
{
	if (!exp)
		return RValue();
	switch (exp->id()) {
	VISIT_CASE2_RETURN(NArrayVariable, NVariable, exp)
	VISIT_CASE2_RETURN(NArrowOperator, NVariable, exp)
	VISIT_CASE2_RETURN(NBaseVariable, NVariable, exp)
	VISIT_CASE2_RETURN(NDereference, NVariable, exp)
	VISIT_CASE2_RETURN(NMemberVariable, NVariable, exp)
	VISIT_CASE_RETURN(NAddressOf, exp)
	VISIT_CASE_RETURN(NAssignment, exp)
	VISIT_CASE_RETURN(NBinaryMathOperator, exp)
	VISIT_CASE_RETURN(NBoolConst, exp)
	VISIT_CASE_RETURN(NCharConst, exp)
	VISIT_CASE_RETURN(NCompareOperator, exp)
	VISIT_CASE_RETURN(NExprVariable, exp)
	VISIT_CASE_RETURN(NFloatConst, exp)
	VISIT_CASE_RETURN(NFunctionCall, exp)
	VISIT_CASE_RETURN(NIncrement, exp)
	VISIT_CASE_RETURN(NIntConst, exp)
	VISIT_CASE_RETURN(NLogicalOperator, exp)
	VISIT_CASE_RETURN(NMemberFunctionCall, exp)
	VISIT_CASE_RETURN(NNewExpression, exp)
	VISIT_CASE_RETURN(NNullCoalescing, exp)
	VISIT_CASE_RETURN(NNullPointer, exp)
	VISIT_CASE_RETURN(NStringLiteral, exp)
	VISIT_CASE_RETURN(NTernaryOperator, exp)
	VISIT_CASE_RETURN(NUnaryMathOperator, exp)
	default:
		context.addError("NodeId::" + to_string(static_cast<int>(exp->id())) + " unrecognized in CGNExpression", *exp);
		return RValue();
	}
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

RValue CGNExpression::visitNExprVariable(NExprVariable* exp)
{
	return visit(exp->getExp());
}

RValue CGNExpression::visitNAssignment(NAssignment* exp)
{
	auto lhsVar = CGNVariable::run(context, exp->getLhs());

	if (!lhsVar) {
		return RValue();
	} else if (lhsVar.stype()->isConst()) {
		context.addError("assignment not allowed for constant variable", *exp->getLhs());
		return RValue();
	}

	BasicBlock* endBlock = nullptr;

	if (exp->getOp() == ParserBase::TT_DQ_MARK) {
		auto condExp = Inst::Load(context, lhsVar);
		Inst::CastTo(context, *exp, condExp, SType::getBool(context));

		auto trueBlock = context.createBlock();
		endBlock = context.createBlock();

		context.IB().CreateCondBr(condExp, endBlock, trueBlock);
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
	context.IB().CreateStore(rhsExp, lhsVar);

	if (exp->getOp() == ParserBase::TT_DQ_MARK) {
		context.IB().CreateBr(endBlock);
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

		context.IB().CreateCondBr(condExp, trueBlock, falseBlock);

		context.pushBlock(trueBlock);
		trueExp = visit(exp->getTrueVal());
		auto tBlk = context.currBlock();
		context.IB().CreateBr(endBlock);

		context.pushBlock(falseBlock);
		falseExp = visit(exp->getFalseVal());
		auto fBlk = context.currBlock();
		context.IB().CreateBr(endBlock);

		context.pushBlock(endBlock);
		if (!trueExp || !falseExp)
			return {};
		auto result = context.IB().CreatePHI(trueExp.type(), 2);
		result->addIncoming(trueExp, tBlk);
		result->addIncoming(falseExp, fBlk);
		retVal = RValue(result, trueExp.stype());
	} else {
		trueExp = visit(exp->getTrueVal());
		falseExp = visit(exp->getFalseVal());
		auto select = context.IB().CreateSelect(condExp, trueExp, falseExp);
		retVal = RValue(select, trueExp.stype());
	}

	if (trueExp.stype() != falseExp.stype())
		context.addError("return types of ternary must match", *exp);
	return retVal;
}

RValue CGNExpression::visitNNewExpression(NNewExpression* exp)
{
	RValue funcVal;
	auto syms = context.loadSymbol("malloc");
	if (syms.empty()) {
		vector<SType*> args;
		args.push_back(SType::getInt(context, 64));
		auto retType = SType::getPointer(context, SType::getInt(context, 8));
		auto funcType = SType::getFunction(context, retType, args);
		Token mallocName("malloc");

		funcVal = Builder::getFuncPrototype(context, &mallocName, funcType, nullptr, false);
	} else if (!syms[0].isFunction() || syms.size() > 1) {
		context.addError("Compiler Error: malloc not function", *exp);
		return {};
	} else {
		funcVal = syms[0];
	}

	RValue sizeBytes;
	RValue sizeArr;
	auto nType = CGNDataTypeNew::run(context, exp->getType(), sizeBytes, sizeArr);
	if (!nType) {
		return RValue();
	} else if (nType->isUnsized()) {
		context.addError("can't call new on " + nType->str(&context) + " type", *exp->getType());
		return RValue();
	}

	vector<Value*> exp_list;
	exp_list.push_back(sizeBytes);

	auto func = static_cast<SFunction&>(funcVal);
	auto call = context.IB().CreateCall(func.value(), exp_list);
	auto ptr = RValue(call, func.returnTy());
	auto ptrType = SType::getPointer(context, nType);
	auto rPtr = RValue(context.IB().CreateBitCast(ptr, *ptrType), ptrType);

	// setup type so the variable is initialized using the base type
	RValue ptr2 = RValue(rPtr.value(), nType);
	auto args = CGNExpression::collect(context, exp->getArgs());
	Inst::InitVariable(context, ptr2, sizeArr, args.get(), *exp->getType());

	return rPtr;
}

RValue CGNExpression::visitNLogicalOperator(NLogicalOperator* exp)
{
	auto firstBlock = context.createBlock();
	auto secondBlock = context.createBlock();
	auto trueBlock = (exp->getOp() == ParserBase::TT_LOG_AND)? firstBlock : secondBlock;
	auto falseBlock = (exp->getOp() == ParserBase::TT_LOG_AND)? secondBlock : firstBlock;

	auto lhsExp = Inst::Branch(trueBlock, falseBlock, exp->getLhs(), context);
	auto lBlk = context.currBlock();

	context.pushBlock(firstBlock);
	auto rhsExp = visit(exp->getRhs());
	Inst::CastTo(context, *exp->getRhs(), rhsExp, SType::getBool(context));
	auto rBlk = context.currBlock();
	context.IB().CreateBr(secondBlock);

	context.pushBlock(secondBlock);
	auto result = context.IB().CreatePHI(Type::getInt1Ty(context), 2);
	result->addIncoming(lhsExp, lBlk);
	result->addIncoming(rhsExp, rBlk);

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
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		auto lBlk = context.currBlock();
		context.IB().CreateCondBr(condition, endBlock, falseBlock);

		context.pushBlock(falseBlock);
		rhsExp = visit(exp->getRhs());
		auto rBlk = context.currBlock();
		context.IB().CreateBr(endBlock);

		context.pushBlock(endBlock);
		auto result = context.IB().CreatePHI(lhsExp.type(), 2);
		result->addIncoming(lhsExp, lBlk);
		result->addIncoming(rhsExp, rBlk);

		retVal = RValue(result, lhsExp.stype());
	} else {
		rhsExp = visit(exp->getRhs());
		auto select = context.IB().CreateSelect(condition, lhsExp, rhsExp);
		retVal = RValue(select, lhsExp.stype());
	}

	if (lhsExp.stype() != rhsExp.stype())
		context.addError("return types of null coalescing operator must match", *exp);
	return retVal;
}

RValue CGNExpression::visitNUnaryMathOperator(NUnaryMathOperator* exp)
{
	auto unaryExp = visit(exp->getExp());
	auto type = unaryExp.stype();
	if (!type)
		return {};

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
	auto syms = context.loadSymbol(funcName);
	if (syms.empty()) {
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
		context.addError("symbol " + funcName + " not defined", *exp);
		return {};
	}

	VecSFunc funcs;
	for (auto sym : syms) {
		auto deSym = Inst::Deref(context, sym, true);
		if (deSym.isFunction())
			funcs.push_back(static_cast<SFunction&>(deSym));
	}

	if (funcs.empty()) {
		context.addError("symbol " + funcName + " doesn't reference a function", *exp);
		return RValue();
	}

	auto args = CGNExpression::collect(context, exp->getArguments());
	return Inst::CallFunction(context, funcs, exp->getName(), *args.get());
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
	context.IB().CreateStore(result, varPtr);

	return exp->postfix()? varVal : RValue(result, type);
}

RValue CGNExpression::visitNBoolConst(NBoolConst* exp)
{
	auto val = exp->getValue() ? ConstantInt::getTrue(context) : ConstantInt::getFalse(context);
	return RValue(val, SType::getBool(context));
}

RValue CGNExpression::visitNNullPointer(NNullPointer* exp)
{
	return RValue::getNullPtr(context, SType::getInt(context, 8));
}

RValue CGNExpression::visitNStringLiteral(NStringLiteral* exp)
{
	auto strVal = exp->getStrVal();
	auto arrData = ConstantDataArray::getString(context, strVal, true);
	auto arrTy = SType::getConst(context, SType::getArray(context, SType::getInt(context, 8), strVal.size() + 1));
	auto arrTyPtr = SType::getPointer(context, arrTy);
	auto gVar = new GlobalVariable(*context.getModule(), *arrTy, true, GlobalValue::PrivateLinkage, arrData);
	auto zero = ConstantInt::get(*SType::getInt(context, 32), 0);

	std::vector<Constant*> idxs;
	idxs.push_back(zero);

	auto strPtr = ConstantExpr::getGetElementPtr(gVar->getType(), gVar, idxs);
	auto strLit = RValue(strPtr, arrTyPtr);

	auto zeroSizePtr = SType::getPointer(context, SType::getConst(context, SType::getArray(context, SType::getInt(context, 8), 0)));
	Inst::CastTo(context, *exp, strLit, zeroSizePtr);
	return strLit;
}

RValue CGNExpression::visitNIntConst(NIntConst* exp)
{
	static const map<string, SType*> suffix = {
		{"i8", SType::getInt(context, 8)},
		{"u8", SType::getInt(context, 8, true)},
		{"i16", SType::getInt(context, 16)},
		{"u16", SType::getInt(context, 16, true)},
		{"i32", SType::getInt(context, 32)},
		{"u32", SType::getInt(context, 32, true)},
		{"i64", SType::getInt(context, 64)},
		{"u64", SType::getInt(context, 64, true)} };
	auto type = SType::getInt(context, 32); // default is int32

	auto data = NConstant::getValueAndSuffix(exp->getStrVal());
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid integer suffix: " + data[1], *exp);
		else
			type = suf->second;
	}
	auto base = exp->getBase();
	string intVal(data[0], base == 10? 0:2);
	auto val = ConstantInt::get(static_cast<IntegerType*>(type->type()), intVal, base);
	return RValue(val, type);
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
	auto strVal = exp->getStrVal();
	char cVal = strVal.at(0);
	if (cVal == '\\' && strVal.length() > 1) {
		switch (strVal.at(1)) {
		case '0': cVal = '\0'; break;
		case 'a': cVal = '\a'; break;
		case 'b': cVal = '\b'; break;
		case 'e': cVal =   27; break;
		case 'f': cVal = '\f'; break;
		case 'n': cVal = '\n'; break;
		case 'r': cVal = '\r'; break;
		case 't': cVal = '\t'; break;
		case 'v': cVal = '\v'; break;
		default: cVal = strVal.at(1);
		}
	}
	auto type = SType::getInt(context, 8, true);
	auto val = ConstantInt::get(type->type(), cVal);
	return RValue(val, type);
}
