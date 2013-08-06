/*      Saphyr, a C++ style compiler using LLVM
        Copyright (C) 2012, Justin Madru (justin.jdm64@gmail.com)

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Util.h"
#include "parserbase.h"

typedef llvm::CmpInst::Predicate Predicate;

void typeCastUp(RValue& lhs, RValue& rhs, CodeContext& context)
{
	auto ltype = lhs.stype();
	auto rtype = rhs.stype();

	if (ltype->matches(rtype)) {
		return;
	} else if (ltype->isArray() || rtype->isArray()) {
		context.addError("can not cast array types");
		return;
	}

	Value* val;
	if (ltype->isFloating()) {
		if (rtype->isFloating()) {
			if (ltype->isDouble()) {
				val = CastInst::CreateFPCast(rhs, *ltype, "", context.currBlock());
				rhs = RValue(val, ltype);
			} else {
				val = CastInst::CreateFPCast(lhs, *rtype, "", context.currBlock());
				lhs = RValue(val, rtype);
			}
		} else {
			val = new SIToFPInst(rhs, *ltype, "", context.currBlock());
			rhs = RValue(val, ltype);
		}
	} else if (rtype->isFloating()) {
		if (ltype->isFloating()) {
			if (rtype->isDouble()) {
				val = CastInst::CreateFPCast(lhs, *rtype, "", context.currBlock());
				lhs = RValue(val, rtype);
			} else {
				val = CastInst::CreateFPCast(rhs, *ltype, "", context.currBlock());
				rhs = RValue(val, ltype);
			}
		} else {
			val = new SIToFPInst(lhs, *rtype, "", context.currBlock());
			lhs = RValue(val, rtype);
		}
	} else {
		if (rtype->intSize() > ltype->intSize()) {
			val = CastInst::CreateIntegerCast(lhs, *rtype, true, "", context.currBlock());
			lhs = RValue(val, rtype);
		} else if (rtype->intSize() < ltype->intSize()) {
			val = CastInst::CreateIntegerCast(rhs, *ltype, true, "", context.currBlock());
			rhs = RValue(val, ltype);
		}
	}
}

void typeCastMatch(RValue& value, SType* type, CodeContext& context)
{
	auto valueType = value.stype();
	if (type->matches(valueType)) {
		return;
	} else if (type->isArray() || valueType->isArray()) {
		context.addError("can not cast array types");
		return;
	}

	Value* val;
	if (type->isFloating()) {
		if (valueType->isFloating())
			val = CastInst::CreateFPCast(value, *type, "", context.currBlock());
		else
			val = new SIToFPInst(value, *type, "", context.currBlock());
	} else if (type->isBool()) {
		// cast to bool is value != 0
		auto pred = getPredicate(ParserBase::TT_NEQ, valueType, context);
		auto op = valueType->isFloating()? Instruction::FCmp : Instruction::ICmp;
		val = CmpInst::Create(op, pred, value, Constant::getNullValue(*valueType), "", context.currBlock());
	} else if (valueType->isFloating()) {
		val = new FPToSIInst(value, *type, "", context.currBlock());
	} else {
		val = CastInst::CreateIntegerCast(value, *type, true, "", context.currBlock());
	}
	value = RValue(val, type);
}

Instruction::BinaryOps getOperator(int oper, SType* type, CodeContext& context)
{
	if (type->isArray()) {
		context.addError("can not perform operation on entire array");
		return Instruction::Add;
	}
	switch (oper) {
	case '*':
		return type->isFloating()? Instruction::FMul : Instruction::Mul;
	case '/':
		return type->isFloating()? Instruction::FDiv : Instruction::SDiv;
	case '%':
		return type->isFloating()? Instruction::FRem : Instruction::SRem;
	case '+':
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
		return Instruction::LShr;
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

Predicate getPredicate(int oper, SType* type, CodeContext& context)
{
	switch (oper) {
	case '<':
		return type->isFloating()? Predicate::FCMP_OLT : Predicate::ICMP_SLT;
	case '>':
		return type->isFloating()? Predicate::FCMP_OGT : Predicate::ICMP_SGT;
	case ParserBase::TT_LEQ:
		return type->isFloating()? Predicate::FCMP_OLE : Predicate::ICMP_SLE;
	case ParserBase::TT_GEQ:
		return type->isFloating()? Predicate::FCMP_OGE : Predicate::ICMP_SGE;
	case ParserBase::TT_NEQ:
		return type->isFloating()? Predicate::FCMP_ONE : Predicate::ICMP_NE;
	case ParserBase::TT_EQ:
		return type->isFloating()? Predicate::FCMP_OEQ : Predicate::ICMP_EQ;
	default:
		context.addError("unrecognized predicate " + to_string(oper));
		return Predicate::ICMP_EQ;
	}
}

bool isComplexExp(NodeType type)
{
	switch (type) {
	case NodeType::IntConst:
	case NodeType::FloatConst:
	case NodeType::Variable:
	case NodeType::ArrayVariable:
		return false;
	default:
		return true;
	}
}
