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
#include "Instructions.h"
#include "parserbase.h"

void Inst::CastMatch(CodeContext& context, RValue& lhs, RValue& rhs, bool upcast)
{
	auto ltype = lhs.stype();
	auto rtype = rhs.stype();

	if (ltype == rtype) {
		return;
	} else if (ltype->isComposite() || rtype->isComposite()) {
		context.addError("can not cast composite types");
		return;
	}

	auto toType = SType::numericConv(context, ltype, rtype, upcast);
	CastTo(lhs, toType, context);
	CastTo(rhs, toType, context);
}

void Inst::CastTo(RValue& value, SType* type, CodeContext& context)
{
	auto valueType = value.stype();

	if (type == valueType) {
		return;
	} else if (type->isComposite() || valueType->isComposite()) {
		context.addError("can not cast composite types");
		return;
	}

	if (type->isBool()) {
		// cast to bool is value != 0
		auto pred = getPredicate(ParserBase::TT_NEQ, valueType, context);
		auto op = valueType->isFloating()? Instruction::FCmp : Instruction::ICmp;
		auto val = CmpInst::Create(op, pred, value, RValue::getZero(valueType), "", context);
		value = RValue(val, type);
		return;
	}

	Instruction::CastOps op;
	switch (type->isFloating() + valueType->isFloating()) {
	case 0: // both int
		if (type->size() > valueType->size()) {
			op = valueType->isUnsigned()? Instruction::ZExt : Instruction::SExt;
		} else if (type->size() < valueType->size()) {
			op = Instruction::Trunc;
		} else {
			// same int size; no need to cast, just set internal type
			value = RValue(value.value(), type);
			return;
		}
		break;
	case 1: // int and float
		if (valueType->isInteger())
			op = valueType->isUnsigned()? Instruction::UIToFP : Instruction::SIToFP;
		else
			op = type->isUnsigned()? Instruction::FPToUI : Instruction::FPToSI;
		break;
	case 2: // both float
		op = type->isDouble()? Instruction::FPExt : Instruction::FPTrunc;
		break;
	default:
		// should never happen
		op = Instruction::SExt;
		break;
	}
	auto val = CastInst::Create(op, value, *type, "", context);
	value = RValue(val, type);
}

BinaryOps Inst::getOperator(int oper, SType* type, CodeContext& context)
{
	if (type->isVec()) {
		type = type->subType();
	} else if (type->isComposite()) {
		context.addError("can not perform operation on composite types");
		return Instruction::Add;
	}
	switch (oper) {
	case '*':
		return type->isFloating()? Instruction::FMul : Instruction::Mul;
	case '/':
		if (type->isFloating())
			return Instruction::FDiv;
		return type->isUnsigned()? Instruction::UDiv : Instruction::SDiv;
	case '%':
		if (type->isFloating())
			return Instruction::FRem;
		return type->isUnsigned()? Instruction::URem : Instruction::SRem;
	case '+':
	case ParserBase::TT_INC:
		return type->isFloating()? Instruction::FAdd : Instruction::Add;
	case '-':
	case ParserBase::TT_DEC:
		return type->isFloating()? Instruction::FSub : Instruction::Sub;
	case ParserBase::TT_LSHIFT:
		if (type->isFloating())
			context.addError("shift operator invalid for float types");
		return Instruction::Shl;
	case ParserBase::TT_RSHIFT:
		if (type->isFloating())
			context.addError("shift operator invalid for float types");
		return type->isUnsigned()? Instruction::LShr : Instruction::AShr;
	case '&':
		if (type->isFloating())
			context.addError("AND operator invalid for float types");
		return Instruction::And;
	case '|':
		if (type->isFloating())
			context.addError("OR operator invalid for float types");
		return Instruction::Or;
	case '^':
		if (type->isFloating())
			context.addError("XOR operator invalid for float types");
		return Instruction::Xor;
	default:
		context.addError("unrecognized operator " + to_string(oper));
		return Instruction::Add;
	}
}

Predicate Inst::getPredicate(int oper, SType* type, CodeContext& context)
{
	const static Predicate predArr[] = {
		Predicate::FCMP_OLT, Predicate::FCMP_OGT, Predicate::FCMP_OLE,
		Predicate::FCMP_OGE, Predicate::FCMP_ONE, Predicate::FCMP_OEQ,
		Predicate::ICMP_ULT, Predicate::ICMP_UGT, Predicate::ICMP_ULE,
		Predicate::ICMP_UGE, Predicate::ICMP_NE,  Predicate::ICMP_EQ,
		Predicate::ICMP_SLT, Predicate::ICMP_SGT, Predicate::ICMP_SLE,
		Predicate::ICMP_SGE, Predicate::ICMP_NE,  Predicate::ICMP_EQ
	};

	if (type->isVec()) {
		type = type->subType();
	} else if (type->isComposite()) {
		context.addError("can not perform operation on composite types");
		return Predicate::ICMP_EQ;
	}

	int offset;
	switch (oper) {
	case '<':
		offset = 0; break;
	case '>':
		offset = 1; break;
	case ParserBase::TT_LEQ:
		offset = 2; break;
	case ParserBase::TT_GEQ:
		offset = 3; break;
	case ParserBase::TT_NEQ:
		offset = 4; break;
	case ParserBase::TT_EQ:
		offset = 5; break;
	default:
		context.addError("unrecognized predicate " + to_string(oper));
		return Predicate::ICMP_EQ;
	}

	if (type->isFloating())
		return predArr[offset];
	offset += type->isUnsigned()? 6 : 12;
	return predArr[offset];
}

RValue Inst::BinaryOp(int type, RValue lhs, RValue rhs, CodeContext& context)
{
	CastMatch(context, lhs, rhs, true);
	auto llvmOp = getOperator(type, lhs.stype(), context);
	auto llvmVal = BinaryOperator::Create(llvmOp, lhs, rhs, "", context);
	return RValue(llvmVal, lhs.stype());
}

RValue Inst::Branch(BasicBlock* trueBlock, BasicBlock* falseBlock, NExpression* condExp, CodeContext& context)
{
	auto condValue = condExp? condExp->genValue(context) : RValue::getOne(SType::getBool(context));
	CastTo(condValue, SType::getBool(context), context);
	BranchInst::Create(trueBlock, falseBlock, condValue, context);
	return condValue;
}

RValue Inst::Cmp(int type, RValue lhs, RValue rhs, CodeContext& context)
{
	CastMatch(context, lhs, rhs);
	auto pred = getPredicate(type, lhs.stype(), context);
	auto cmpType = lhs.stype()->isVec()? lhs.stype()->subType() : lhs.stype();
	auto op = cmpType->isFloating()? Instruction::FCmp : Instruction::ICmp;
	auto cmp = CmpInst::Create(op, pred, lhs, rhs, "", context);
	auto retType = lhs.stype()->isVec()?
		SType::getVec(context, SType::getBool(context), lhs.stype()->size()) :
		SType::getBool(context);
	return RValue(cmp, retType);
}
