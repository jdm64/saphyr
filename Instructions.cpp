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
	} else if (ltype->isComplex() || rtype->isComplex()) {
		context.addError("can not cast complex types");
		return;
	} else if (ltype->isPointer() || rtype->isPointer()) {
		// different pointer types can't be cast automatically
		// the operations need to handle pointers specially
		return;
	}

	auto toType = SType::numericConv(context, ltype, rtype, upcast);
	CastTo(context, lhs, toType, upcast);
	CastTo(context, rhs, toType, upcast);
}

void Inst::CastTo(CodeContext& context, RValue& value, SType* type, bool upcast)
{
	auto valueType = value.stype();

	if (type == valueType) {
		// non-assignment and non-comparison operations with enum types
		// must result in an int type to keep enum variables within range.
		if (upcast && valueType->isEnum())
			value.castToSubtype();
		return;
	} else if (type->isComplex() || valueType->isComplex()) {
		context.addError("can not cast complex types");
		return;
	} else if (type->isPointer()) {
		if (value.isNullPtr()) {
			// NOTE: discard current null value and create
			// a new one using the right type
			value = RValue::getNullPtr(context, type);
			return;
		}
		context.addError("can't cast value to pointer type");
		return;
	} else if (type->isVec()) {
		// unwrap enum type
		if (valueType->isEnum())
			valueType = value.castToSubtype();

		if (valueType->isNumeric()) {
			CastTo(context, value, type->subType(), upcast);

			auto i32 = SType::getInt(context, 32);
			auto mask = RValue::getZero(context, SType::getVec(context, i32, type->size()));
			auto udef = UndefValue::get(*SType::getVec(context, type->subType(), 1));
			auto instEle = InsertElementInst::Create(udef, value, RValue::getZero(context, i32), "", context);
			auto retVal = new ShuffleVectorInst(instEle, udef, mask, "", context);

			value = RValue(retVal, type);
			return;
		} else if (valueType->isPointer()) {
			context.addError("can not cast pointer to vec type");
			return;
		} else if (type->size() != valueType->size()) {
			context.addError("can not cast vec types of different sizes");
			return;
		} else if (type->subType()->isBool()) {
			// cast to bool is value != 0
			auto pred = getPredicate(ParserBase::TT_NEQ, valueType, context);
			auto op = valueType->subType()->isFloating()? Instruction::FCmp : Instruction::ICmp;
			auto val = CmpInst::Create(op, pred, value, RValue::getZero(context, valueType), "", context);
			value = RValue(val, type);
			return;
		} else {
			auto op = getCastOp(valueType->subType(), type->subType());
			auto val = CastInst::Create(op, value, *type, "", context);
			value = RValue(val, type);
			return;
		}
	} else if (type->isEnum()) {
		// casting to enum would violate value constraints
		context.addError("can't cast to enum type");
		return;
	} else if (type->isBool()) {
		if (valueType->isVec()) {
			context.addError("can not cast vec type to bool");
			return;
		} else if (valueType->isEnum()) {
			valueType = value.castToSubtype();
		}

		// cast to bool is value != 0
		auto pred = getPredicate(ParserBase::TT_NEQ, valueType, context);
		auto op = valueType->isFloating()? Instruction::FCmp : Instruction::ICmp;
		auto val = CmpInst::Create(op, pred, value, RValue::getZero(context, valueType), "", context);
		value = RValue(val, type);
		return;
	}

	// unwrap enum type
	if (valueType->isEnum())
		valueType = value.castToSubtype();

	auto op = getCastOp(valueType, type);
	auto val = op != Instruction::AddrSpaceCast? CastInst::Create(op, value, *type, "", context) : value;
	value = RValue(val, type);
}

CastOps Inst::getCastOp(SType* from, SType* to)
{
	switch (from->isFloating() | to->isFloating() << 1) {
	case 0:
		// both int
		if (to->size() > from->size())
			return from->isUnsigned()? Instruction::ZExt : Instruction::SExt;
		else if (from->size() > to->size())
			return Instruction::Trunc;
		break;
	case 1:
		// from = float, to = int
		return to->isUnsigned()? Instruction::FPToUI : Instruction::FPToSI;
	case 2:
		// from = int, to = float
		return from->isUnsigned()? Instruction::UIToFP : Instruction::SIToFP;
	case 3:
		// both float
		return to->isDouble()? Instruction::FPExt : Instruction::FPTrunc;
	}
	return Instruction::AddrSpaceCast;
}

BinaryOps Inst::getOperator(int oper, SType* type, CodeContext& context)
{
	if (type->isVec()) {
		type = type->subType();
	} else if (type->isComplex()) {
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
	case ParserBase::TT_DEC:
		return type->isFloating()? Instruction::FAdd : Instruction::Add;
	case '-':
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
	} else if (type->isComplex()) {
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

RValue Inst::PointerMath(int type, RValue ptr, RValue val, CodeContext& context)
{
	if (type != ParserBase::TT_INC && type != ParserBase::TT_DEC) {
		context.addError("pointer arithmetic only valid using ++/-- operators");
		return ptr;
	}
	auto ptrVal = GetElementPtrInst::Create(ptr, val.value(), "", context);
	return RValue(ptrVal, ptr.stype());
}

RValue Inst::BinaryOp(int type, RValue lhs, RValue rhs, CodeContext& context)
{
	CastMatch(context, lhs, rhs, true);

	switch ((lhs.stype()->isPointer() << 1) | rhs.stype()->isPointer()) {
	default:
	case 0: // no pointer
	{
		auto llvmOp = getOperator(type, lhs.stype(), context);
		return RValue(BinaryOperator::Create(llvmOp, lhs, rhs, "", context), lhs.stype());
	}
	case 1: // lhs != ptr, rhs == ptr
		swap(lhs, rhs);
	case 2: // lhs == ptr, rhs != ptr
		return PointerMath(type, lhs, rhs, context);
	case 3: // both ptr
		context.addError("can't perform operation with two pointers");
		return lhs;
	}
}

RValue Inst::Branch(BasicBlock* trueBlock, BasicBlock* falseBlock, NExpression* condExp, CodeContext& context)
{
	auto condValue = condExp? condExp->genValue(context) : RValue::getNumVal(context, SType::getBool(context));
	CastTo(context, condValue, SType::getBool(context));
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

RValue Inst::Load(CodeContext& context, RValue value)
{
	return RValue(new LoadInst(value, "", context), value.stype());
}

RValue Inst::Deref(CodeContext& context, RValue value, bool recursive)
{
	auto retVal = RValue(value.value(), value.stype());
	while (retVal.stype()->isPointer()) {
		retVal = RValue(new LoadInst(retVal, "", context), retVal.stype()->subType());
		if (!recursive)
			break;
	}
	return retVal;
}
