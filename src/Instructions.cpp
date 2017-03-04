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
#include "Instructions.h"
#include "parserbase.h"
#include "CGNDataType.h"
#include "CGNVariable.h"
#include "CGNExpression.h"

void Inst::castError(CodeContext& context, SType* from, SType* to, Token* token)
{
	context.addError("can not cast " + from->str(&context) + " to " + to->str(&context), token);
}

bool Inst::CastMatch(CodeContext& context, Token* optToken, RValue& lhs, RValue& rhs, bool upcast)
{
	if (!lhs || !rhs)
		return true;

	auto ltype = lhs.stype();
	auto rtype = rhs.stype();

	if (ltype == rtype) {
		return false;
	} else if (ltype->isComplex() || rtype->isComplex()) {
		context.addError("can not cast between " + ltype->str(&context) + " and " + rtype->str(&context) + " types", optToken);
		return true;
	} else if (ltype->isPointer() || rtype->isPointer()) {
		// different pointer types can't be cast automatically
		// the operations need to handle pointers specially
		return false;
	}

	auto toType = SType::numericConv(context, optToken, ltype, rtype, upcast);
	return CastTo(context, optToken, lhs, toType, upcast) || CastTo(context, optToken, rhs, toType, upcast);
}

bool Inst::CastTo(CodeContext& context, Token* token, RValue& value, SType* type, bool upcast)
{
	if (!value)
		return true;

	auto valueType = value.stype();

	if (type == valueType) {
		// non-assignment and non-comparison operations with enum types
		// must result in an int type to keep enum variables within range.
		if (upcast && valueType->isEnum())
			value.castToSubtype();
		return false;
	} else if (type->isComplex() || valueType->isComplex()) {
		castError(context, valueType, type, token);
		return true;
	} else if (type->isPointer()) {
		if (!valueType->isPointer()) {
			context.addError("Cannot cast non-pointer to pointer", token);
			return true;
		} else if (type->subType()->isArray() && valueType->subType()->isArray()) {
			if (type->subType()->subType() != valueType->subType()->subType()) {
				context.addError("Cannot cast array pointers of different types", token);
				return true;
			} else if (type->subType()->size() > valueType->subType()->size()) {
				context.addError("Pointers to arrays only allowed to cast to smaller arrays", token);
				return true;
			}
			value = RValue(new BitCastInst(value, *type, "", context), type);
			return false;
		} else if (value.isNullPtr()) {
			// NOTE: discard current null value and create
			// a new one using the right type
			value = RValue::getNullPtr(context, type);
			return false;
		} else if (type->subType()->isVoid()) {
			value = RValue(new BitCastInst(value, *type, "", context), type);
			return false;
		}
		castError(context, valueType, type, token);
		return true;
	} else if (type->isVec()) {
		// unwrap enum type
		if (valueType->isEnum())
			valueType = value.castToSubtype();

		if (valueType->isNumeric()) {
			CastTo(context, token, value, type->subType(), upcast);

			auto i32 = SType::getInt(context, 32);
			auto mask = RValue::getZero(context, SType::getVec(context, i32, type->size()));
			auto udef = UndefValue::get(*SType::getVec(context, type->subType(), 1));
			auto instEle = InsertElementInst::Create(udef, value, RValue::getZero(context, i32), "", context);
			auto retVal = new ShuffleVectorInst(instEle, udef, mask, "", context);

			value = RValue(retVal, type);
			return false;
		} else if (valueType->isPointer()) {
			castError(context, valueType, type, token);
			return true;
		} else if (type->size() != valueType->size()) {
			context.addError("can not cast vec types of different sizes", token);
			return true;
		} else if (type->subType()->isBool()) {
			// cast to bool is value != 0
			auto pred = getPredicate(ParserBase::TT_NEQ, token, valueType, context);
			auto op = valueType->subType()->isFloating()? Instruction::FCmp : Instruction::ICmp;
			auto val = CmpInst::Create(op, pred, value, RValue::getZero(context, valueType), "", context);
			value = RValue(val, type);
			return false;
		} else {
			NumericCast(value, valueType->subType(), type->subType(), type, context);
			return false;
		}
	} else if (type->isEnum()) {
		// casting to enum would violate value constraints
		castError(context, valueType, type, token);
		return true;
	} else if (type->isBool()) {
		if (valueType->isVec()) {
			castError(context, valueType, type, token);
			return true;
		} else if (valueType->isEnum()) {
			valueType = value.castToSubtype();
		}

		// cast to bool is value != 0
		auto pred = getPredicate(ParserBase::TT_NEQ, token, valueType, context);
		auto op = valueType->isFloating()? Instruction::FCmp : Instruction::ICmp;
		auto val = CmpInst::Create(op, pred, value, RValue::getZero(context, valueType), "", context);
		value = RValue(val, type);
		return false;
	}

	if (valueType->isPointer()) {
		castError(context, valueType, type, token);
		return true;
	}

	// unwrap enum type
	if (valueType->isEnum())
		valueType = value.castToSubtype();

	NumericCast(value, valueType, type, type, context);
	return false;
}

RValue Inst::CastAs(CodeContext& context, NArrowOperator* exp)
{
	auto toType = exp->getArg();
	if (!toType) {
		context.addError("as operator requires type argument", *exp);
		return RValue();
	} else if (exp->getType() == NArrowOperator::DATA) {
		context.addError("as operator only operates on expression", *exp);
		return RValue();
	}
	auto to = CGNDataType::run(context, toType);
	if (!to)
		return RValue();
	auto expr = CGNExpression::run(context, exp->getExp());
	if (!expr)
		return RValue();

	// casting @void to real pointer
	auto stype = expr.stype();
	if (stype->isPointer() && stype->subType()->isVoid() && to->isPointer()) {
		return RValue(new BitCastInst(expr, *to, "", context), to);
	}

	CastTo(context, *exp, expr, to);
	return expr;
}

void Inst::NumericCast(RValue& value, SType* from, SType* to, SType* final, CodeContext& context)
{
	CastOps op;
	switch (from->isFloating() | to->isFloating() << 1) {
	case 0:
		// both int
		if (to->size() > from->size()) {
			op = from->isUnsigned()? Instruction::ZExt : Instruction::SExt;
		} else if (to->size() < from->size()) {
			op = Instruction::Trunc;
		} else {
			value = RValue(value, final);
			return;
		}
		break;
	case 1:
		// from = float, to = int
		op = to->isUnsigned()? Instruction::FPToUI : Instruction::FPToSI;
		break;
	case 2:
		// from = int, to = float
		op = from->isUnsigned()? Instruction::UIToFP : Instruction::SIToFP;
		break;
	case 3:
		// both float
		op = to->isDouble()? Instruction::FPExt : Instruction::FPTrunc;
		break;
	default:
		context.addError("Compiler Error: invalid cast op", nullptr);
		return;
	}
	auto val = CastInst::Create(op, value, *final, "", context);
	value = RValue(val, final);
}

BinaryOps Inst::getOperator(int oper, Token* optToken, SType* type, CodeContext& context)
{
	if (type->isVec()) {
		type = type->subType();
	} else if (type->isComplex()) {
		context.addError("can not perform operation on composite types", optToken);
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
			context.addError("shift operator invalid for float types", optToken);
		return Instruction::Shl;
	case ParserBase::TT_RSHIFT:
		if (type->isFloating())
			context.addError("shift operator invalid for float types", optToken);
		return type->isUnsigned()? Instruction::LShr : Instruction::AShr;
	case '&':
		if (type->isFloating())
			context.addError("AND operator invalid for float types", optToken);
		return Instruction::And;
	case '|':
		if (type->isFloating())
			context.addError("OR operator invalid for float types", optToken);
		return Instruction::Or;
	case '^':
		if (type->isFloating())
			context.addError("XOR operator invalid for float types", optToken);
		return Instruction::Xor;
	default:
		context.addError("unrecognized operator " + to_string(oper), optToken);
		return Instruction::Add;
	}
}

Predicate Inst::getPredicate(int oper, Token* token, SType* type, CodeContext& context)
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
		context.addError("can not perform operation on composite types", token);
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
		context.addError("unrecognized predicate " + to_string(oper), token);
		return Predicate::ICMP_EQ;
	}

	if (type->isFloating())
		return predArr[offset];
	offset += type->isUnsigned()? 6 : 12;
	return predArr[offset];
}

RValue Inst::PointerMath(int type, Token* optToken, RValue ptr, const RValue& val, CodeContext& context)
{
	if (type != ParserBase::TT_INC && type != ParserBase::TT_DEC) {
		context.addError("pointer arithmetic only valid using ++/-- operators", optToken);
		return ptr;
	}
	return GetElementPtr(context, ptr, val.value(), ptr.stype());
}

RValue Inst::BinaryOp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context)
{
	if (CastMatch(context, optToken, lhs, rhs, true))
		return RValue();

	switch ((lhs.stype()->isPointer() << 1) | rhs.stype()->isPointer()) {
	default:
	case 0: // no pointer
	{
		auto llvmOp = getOperator(type, optToken, lhs.stype(), context);
		return RValue(BinaryOperator::Create(llvmOp, lhs, rhs, "", context), lhs.stype());
	}
	case 1: // lhs != ptr, rhs == ptr
		swap(lhs, rhs);
		/* no break */
	case 2: // lhs == ptr, rhs != ptr
		return PointerMath(type, optToken, lhs, rhs, context);
	case 3: // both ptr
		context.addError("can't perform operation with two pointers", optToken);
		return lhs;
	}
}

RValue Inst::Branch(BasicBlock* trueBlock, BasicBlock* falseBlock, NExpression* condExp, CodeContext& context)
{
	auto condValue = condExp? CGNExpression::run(context, condExp) : RValue::getNumVal(context, SType::getBool(context));
	Token* token = nullptr;
	if (condExp)
		token = *condExp;
	CastTo(context, token, condValue, SType::getBool(context));
	BranchInst::Create(trueBlock, falseBlock, condValue, context);
	return condValue;
}

RValue Inst::Cmp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context)
{
	if (CastMatch(context, optToken, lhs, rhs))
		return RValue();
	auto pred = getPredicate(type, optToken, lhs.stype(), context);
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
	if (!value)
		return value;
	else if (value.isFunction())
		// don't require address-of operator for converting
		// a function into a function pointer
		return RValue(value.value(), SType::getPointer(context, value.stype()));
	else if (value.type() == value.stype()->type())
		return value;

	return RValue(new LoadInst(value, "", context), value.stype());
}

RValue Inst::Deref(CodeContext& context, const RValue& value, bool recursive)
{
	auto retVal = RValue(value.value(), value.stype());
	while (retVal.stype()->isPointer()) {
		retVal = RValue(new LoadInst(retVal, "", context), retVal.stype()->subType());
		if (!recursive)
			break;
	}
	return retVal;
}

RValue Inst::SizeOf(CodeContext& context, SType* type, Token* token)
{
	if (!type) {
		return RValue();
	} else if (type->isUnsized()) {
		context.addError("size of " + type->str(&context) + " is invalid", token);
		return RValue();
	}
	return RValue::getNumVal(context, SType::getInt(context, 64, true), SType::allocSize(context, type));
}

RValue Inst::SizeOf(CodeContext& context, Token* name)
{
	auto nameStr = name->str;
	if (nameStr == "this") {
		unique_ptr<NThisType> thisType(new NThisType(new Token(*name)));
		return SizeOf(context, CGNDataType::run(context, thisType.get()), name);
	}
	auto isType = SUserType::lookup(context, nameStr);
	auto isVar = context.loadSymbol(nameStr);
	SType* stype = nullptr;

	if (isType && isVar) {
		context.addError(nameStr + " is ambigious, both a type and a variable", name);
	} else if (isType) {
		stype = isType;
	} else if (isVar) {
		stype = isVar.stype();
	} else {
		context.addError("type " + nameStr + " is not declared", name);
	}
	return SizeOf(context, stype, name);
}

RValue Inst::SizeOf(CodeContext& context, NArrowOperator* exp)
{
	if (exp->getArg()) {
		context.addError("size operator takes no arguments", *exp);
		return RValue();
	}

	switch (exp->getType()) {
	case NArrowOperator::DATA:
		return SizeOf(context, CGNDataType::run(context, exp->getDataType()), *exp);
	case NArrowOperator::EXP:
		// TODO generalize this logic
		if (exp->getExp()->id() == NodeId::NExprVariable) {
			return SizeOf(context, *exp->getExp());
		} else if (exp->getExp()->id() == NodeId::NBaseVariable) {
			return SizeOf(context, *exp);
		}
		return SizeOf(context, CGNExpression::run(context, exp->getExp()).stype(), *exp);
	default:
		// shouldn't happen
		return RValue();
	}
}

RValue Inst::CallFunction(CodeContext& context, SFunction& func, Token* name, NExpressionList* args, vector<Value*>& expList)
{
	// NOTE args can be null
	auto argCount = (args? args->size() : 0) + expList.size();
	auto paramCount = func.numParams();
	if (argCount != paramCount) {
		context.addError("argument count for " + func.name().str() + " function invalid, "
			+ to_string(argCount) + " arguments given, but " + to_string(paramCount) + " required.", name);
		return RValue();
	}
	int i = expList.size();
	if (args) {
		for (auto arg : *args) {
			auto argExp = CGNExpression::run(context, arg);
			CastTo(context, name, argExp, func.getParam(i++));
			expList.push_back(argExp);
		}
	}
	auto call = CallInst::Create(func, expList, "", context);
	return RValue(call, func.returnTy());
}

RValue Inst::CallMemberFunction(CodeContext& context, NVariable* baseVar, Token* funcName, NExpressionList* arguments)
{
	auto baseVal = CGNVariable::run(context, baseVar);
	if (!baseVal)
		return RValue();

	baseVal = RValue(baseVal, SType::getPointer(context, baseVal.stype()));

	auto type = baseVal.stype();
	while (true) {
		auto sub = type->subType();
		if (sub->isClass()) {
			return CallMemberFunctionClass(context, baseVar, baseVal, funcName, arguments);
		} else if (sub->isStruct() | sub->isUnion()) {
			return CallMemberFunctionNonClass(context, baseVar, baseVal, funcName, arguments);
		} else if (sub->isPointer()) {
			baseVal = Deref(context, baseVal);
			type = baseVal.stype();
		} else {
			context.addError("member function call requires class or class pointer", funcName);
			return RValue();
		}
	}
}

RValue Inst::CallMemberFunctionClass(CodeContext& context, NVariable* baseVar, RValue& baseVal, Token* funcName, NExpressionList* arguments)
{
	auto type = baseVal.stype();
	auto className = SUserType::lookup(context, type->subType());
	auto clType = static_cast<SClassType*>(type->subType());
	auto sym = clType->getItem(funcName->str);
	if (!sym) {
		context.addError("class " + className + " has no symbol " + funcName->str, funcName);
		return RValue();
	} else if (!sym->second.isFunction()) {
		return CallMemberFunctionNonClass(context, baseVar, baseVal, funcName, arguments);
	}

	auto func = static_cast<SFunction&>(sym->second);
	vector<Value*> exp_list;
	if (!func.isStatic()) {
		if (baseVal.isUndef()) {
			context.addError("unable to call non-static class function from a static function", funcName);
			return RValue();
		}
		exp_list.push_back(baseVal);
	}
	return CallFunction(context, func, funcName, arguments, exp_list);
}

RValue Inst::CallMemberFunctionNonClass(CodeContext& context, NVariable* baseVar, RValue& baseVal, Token* funcName, NExpressionList* arguments)
{
	baseVal = {baseVal.value(), baseVal.stype()->subType()};
	auto sym = LoadMemberVar(context, baseVal, *baseVar, funcName);
	sym = Deref(context, sym);
	if (!sym || !sym.stype()->isFunction()) {
		context.addError("function or function pointer expected", funcName);
		return RValue();
	}
	auto func = static_cast<SFunction&>(sym);
	vector<Value*> exp_list;
	return CallFunction(context, func, funcName, arguments, exp_list);
}

bool Inst::CallConstructor(CodeContext& context, RValue var, Token* token, NExpressionList* initList)
{
	auto clType = static_cast<SClassType*>(var.stype());
	auto clItem = clType->getItem("this");
	if (!clItem)
		return false;

	auto func = static_cast<SFunction&>(clItem->second);
	vector<Value*> exp_list;
	exp_list.push_back(var);
	CallFunction(context, func, token, initList, exp_list);
	return true;
}

void Inst::CallDestructor(CodeContext& context, RValue value, Token* valueToken)
{
	auto type = value.stype();
	auto className = SUserType::lookup(context, type->subType());
	auto clType = static_cast<SClassType*>(type->subType());
	auto sym = clType->getItem("null");
	if (!sym)
		return;

	auto func = static_cast<SFunction&>(sym->second);
	vector<Value*> exp_list;
	exp_list.push_back(value);
	NExpressionList argList;
	CallFunction(context, func, valueToken, &argList, exp_list);
}

RValue Inst::LoadMemberVar(CodeContext& context, const string& name)
{
	auto baseVar = new NBaseVariable(new Token("this"));
	auto memName = new Token(name);
	unique_ptr<NMemberVariable> classVar(new NMemberVariable(baseVar, memName));
	return CGNVariable::run(context, &*classVar);
}

RValue Inst::LoadMemberVar(CodeContext& context, RValue baseVar, Token* baseToken, Token* memberName)
{
	auto varType = baseVar.stype();
	auto member = memberName->str;
	auto baseName = SUserType::lookup(context, varType);
	if (varType->isStruct()) {
		auto structType = static_cast<SStructType*>(varType);
		auto item = structType->getItem(member);
		if (!item) {
			context.addError(baseName + " doesn't have member " + member, memberName);
			return RValue();
		} else if (item->second.isFunction()) {
			return item->second;
		} else if (baseVar.isUndef()) {
			context.addError("cannot access member variable from a static context", memberName);
			return RValue();
		}

		vector<Value*> indexes;
		indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
		indexes.push_back(RValue::getNumVal(context, SType::getInt(context, 32), item->first));

		return GetElementPtr(context, baseVar, indexes, item->second.stype());
	} else if (varType->isUnion()) {
		auto unionType = static_cast<SUnionType*>(varType);
		auto item = unionType->getItem(member);
		if (!item) {
			context.addError(baseName + " doesn't have member " + member, memberName);
			return RValue();
		}

		auto ptr = SType::getPointer(context, item);
		auto castEl = new BitCastInst(baseVar, *ptr, "", context);
		return RValue(castEl, item);
	} else if (varType->isEnum()) {
		auto enumType = static_cast<SEnumType*>(varType);
		auto item = enumType->getItem(member);
		if (!item) {
			context.addError(baseName + " doesn't have member " + member, memberName);
			return RValue();
		}
		auto val = RValue::getValue(context, *item);
		return RValue(val.value(), enumType);
	}

	context.addError(varType->str(&context) + " is not a struct/union/enum", baseToken);
	return RValue();
}

void Inst::InitVariable(CodeContext& context, RValue var, Token* token, NExpressionList* initList, RValue& initVal)
{
	auto varType = var.stype();
	if (varType->isClass()) {
		if (CallConstructor(context, var, token, initList))
			return;
	}

	if (initList) {
		if (initList->empty()) {
			// no constructor and empty initializer; do zero initialization
			new StoreInst(RValue::getZero(context, varType), var, context);
		} else if (initList->size() > 1) {
			context.addError("invalid variable initializer", token);
		} else {
			initVal = CGNExpression::run(context, initList->at(0));
		}
	}

	if (initVal) {
		CastTo(context, token, initVal, varType);
		new StoreInst(initVal, var, context);
	}
}

RValue Inst::StoreTemporary(CodeContext& context, NExpression* exp)
{
	auto value = CGNExpression::run(context, exp);
	if (!value)
		return RValue();
	auto stackAlloc = new AllocaInst(value.type(), "", context);
	new StoreInst(value, stackAlloc, context);
	return RValue(stackAlloc, value.stype());
}
