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

void Inst::castError(CodeContext& context, const string& msg, SType* from, SType* to, Token* token)
{
	context.addError(msg + " ( " + from->str(&context) + " to " + to->str(&context) + " )", token);
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
		castError(context, "Cannot cast complex types", valueType, type, token);
		return true;
	} else if (type->isPointer()) {
		if (!valueType->isPointer()) {
			context.addError("Cannot cast non-pointer to pointer", token);
			return true;
		} else if (type->subType()->isArray() && valueType->subType()->isArray()) {
			if (!SType::isConstEQ(context, type->subType()->subType(), valueType->subType()->subType())) {
				castError(context, "Cannot cast array pointers of different types", valueType, type, token);
				return true;
			} else if (type->subType()->size() > valueType->subType()->size()) {
				context.addError("Pointers to arrays only allowed to cast to smaller arrays", token);
				return true;
			}
			value = RValue(context.IB().CreateBitCast(value, *type), type);
			return false;
		} else if (value.isNullPtr()) {
			// NOTE: discard current null value and create
			// a new one using the right type
			value = RValue::getNullPtr(context, type);
			return false;
		} else if (type->subType()->isVoid()) {
			value = RValue(context.IB().CreateBitCast(value, *type), type);
			return false;
		}
		castError(context, "Cannot cast type to pointer", valueType, type, token);
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
			auto instEle = context.IB().CreateInsertElement(udef, value, RValue::getZero(context, i32).value());
			auto retVal = context.IB().CreateShuffleVector(instEle, udef, mask);

			value = RValue(retVal, type);
			return false;
		} else if (valueType->isPointer()) {
			castError(context, "Cannot cast vec to pointer", valueType, type, token);
			return true;
		} else if (type->size() != valueType->size()) {
			context.addError("can not cast vec types of different sizes", token);
			return true;
		} else if (type->subType()->isBool()) {
			// cast to bool is value != 0
			auto pred = getPredicate(ParserBase::TT_NEQ, token, valueType, context);
			auto val = valueType->subType()->isFloating() ?
				context.IB().CreateFCmp(pred, value, RValue::getZero(context, valueType)) :
				context.IB().CreateICmp(pred, value, RValue::getZero(context, valueType));
			value = RValue(val, type);
			return false;
		} else {
			NumericCast(value, valueType->subType(), type->subType(), type, context);
			return false;
		}
	} else if (type->isEnum()) {
		// casting to enum would violate value constraints
		castError(context, "Cannot cast to enum", valueType, type, token);
		return true;
	} else if (type->isBool()) {
		if (valueType->isVec()) {
			castError(context, "Cannot cast vec to bool", valueType, type, token);
			return true;
		} else if (valueType->isEnum()) {
			valueType = value.castToSubtype();
		}

		// cast to bool is value != 0
		auto pred = getPredicate(ParserBase::TT_NEQ, token, valueType, context);
		auto val = valueType->isFloating() ?
			context.IB().CreateFCmp(pred, value, RValue::getZero(context, valueType)) :
			context.IB().CreateICmp(pred, value, RValue::getZero(context, valueType));
		value = RValue(val, type);
		return false;
	}

	if (valueType->isPointer()) {
		castError(context, "Cannot cast pointer", valueType, type, token);
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
	auto args = exp->getArgs();
	if (!args) {
		context.addError("as operator requires type argument", *exp);
		return RValue();
	} else if (args->size() != 1) {
		context.addError("as operator takes only one type argument", *exp);
		return RValue();
	} else if (exp->getType() == NArrowOperator::DATA) {
		context.addError("as operator only operates on expression", *exp);
		return RValue();
	}
	auto to = CGNDataType::run(context, args->at(0));
	if (!to)
		return RValue();
	auto expr = CGNExpression::run(context, exp->getExp());
	if (!expr)
		return RValue();

	auto stype = expr.stype();
	if (stype == to) {
		return StoreTemporary(context, expr);
	} else if (SType::isConstEQ(context, stype, to)) {
		expr = RValue(expr.value(), to);
		return StoreTemporary(context, expr);
	}

	if (stype->isPointer()) {
		if (to->isPointer() && stype->subType()->isVoid()) {
			expr = RValue(context.IB().CreateBitCast(expr, *to), to);
			return StoreTemporary(context, expr);
		} else if (to->isInteger()) {
			expr = RValue(context.IB().CreatePtrToInt(expr, *to), to);
			return StoreTemporary(context, expr);
		}
	}

	CastTo(context, *exp, expr, to);
	return expr;
}

RValue Inst::MutCast(CodeContext& context, NArrowOperator* exp)
{
	auto args = exp->getArgs();
	if (args && args->size() != 0) {
		context.addError("mut operator takes no arguments", *exp);
		return RValue();
	} else if (exp->getType() == NArrowOperator::DATA) {
		context.addError("mut operator only operates on expression", *exp);
		return RValue();
	}
	auto varPtr = dynamic_cast<NVariable*>(exp->getExp());
	if (!varPtr) {
		context.addError("mut operator only operates on variable expression", *exp);
		return RValue();
	}
	auto expr = CGNVariable::run(context, varPtr);
	if (!expr)
		return RValue();

	return RValue(expr.value(), SType::getMutable(context, expr.stype()));
}

void Inst::NumericCast(RValue& value, SType* from, SType* to, SType* actual, CodeContext& context)
{
	CastOps op;
	switch (from->isFloating() | (to->isFloating() << 1)) {
	case 0:
		// both int
		if (to->size() > from->size()) {
			op = from->isUnsigned()? Instruction::ZExt : Instruction::SExt;
		} else if (to->size() < from->size()) {
			op = Instruction::Trunc;
		} else {
			value = RValue(value, actual);
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
	auto val = context.IB().CreateCast(op, value, *actual);
	value = RValue(val, actual);
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
	case 1: // lhs != ptr, rhs == ptr
		swap(lhs, rhs);
		/* no break */
	case 2: // lhs == ptr, rhs != ptr
		return PointerMath(type, optToken, lhs, rhs, context);
	case 3: // both ptr
		context.addError("can't perform operation with two pointers", optToken);
		return lhs;
	default: // 0 - no pointer
		auto llvmOp = getOperator(type, optToken, lhs.stype(), context);
		return RValue(context.IB().CreateBinOp(llvmOp, lhs, rhs), lhs.stype());
	}
}

RValue Inst::Branch(BasicBlock* trueBlock, BasicBlock* falseBlock, NExpression* condExp, CodeContext& context)
{
	auto condValue = condExp? CGNExpression::run(context, condExp) : RValue::getNumVal(context, SType::getBool(context));
	Token* token = condExp ? *condExp : static_cast<Token*>(nullptr);
	CastTo(context, token, condValue, SType::getBool(context));
	context.IB().CreateCondBr(condValue, trueBlock, falseBlock);
	return condValue;
}

RValue Inst::Cmp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context)
{
	if (CastMatch(context, optToken, lhs, rhs))
		return RValue();
	auto pred = getPredicate(type, optToken, lhs.stype(), context);
	auto cmpType = lhs.stype()->isVec()? lhs.stype()->subType() : lhs.stype();
	auto cmp = cmpType->isFloating() ?
		context.IB().CreateFCmp(pred, lhs, rhs) :
		context.IB().CreateICmp(pred, lhs, rhs);
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

	return RValue(context.IB().CreateLoad(value), value.stype());
}

RValue Inst::Deref(CodeContext& context, const RValue& value, bool recursive)
{
	auto retVal = RValue(value.value(), value.stype());
	while (retVal.stype()->isPointer()) {
		retVal = RValue(context.IB().CreateLoad(retVal), retVal.stype()->subType());
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
	return RValue::getNumVal(context, SType::allocSize(context, type), 64, true);
}

RValue Inst::SizeOf(CodeContext& context, Token* name)
{
	auto nameStr = name->str;
	if (nameStr == "this") {
		unique_ptr<NThisType> thisType(new NThisType(new Token(*name)));
		return SizeOf(context, CGNDataType::run(context, thisType.get()), name);
	}
	bool hasErrors = false;
	auto isType = SUserType::lookup(context, name, {}, hasErrors);
	auto isVar = context.loadSymbol(nameStr);
	SType* stype = nullptr;

	if ((isType && isVar.size()) || isVar.size() > 1) {
		context.addError(nameStr + " is ambigious, both a type and a variable", name);
	} else if (isType) {
		stype = isType;
	} else if (isVar.size()) {
		stype = isVar[0].stype();
	} else {
		context.addError("type " + nameStr + " is not declared", name);
	}
	return SizeOf(context, stype, name);
}

RValue Inst::SizeOf(CodeContext& context, NArrowOperator* exp)
{
	if (exp->getArgs() && exp->getArgs()->size() != 0) {
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

RValue Inst::LenOp(CodeContext& context, NArrowOperator* op)
{
	if (op->getArgs() && op->getArgs()->size() != 0) {
		context.addError("len operator takes no arguments", *op);
		return RValue();
	}

	SType* dtype = nullptr;
	switch (op->getType()) {
	case NArrowOperator::DATA:
		dtype = CGNDataType::run(context, op->getDataType());
		break;
	case NArrowOperator::EXP:
		dtype = CGNExpression::run(context, op->getExp()).stype();
		break;
	default:
		// shouldn't happen
		return RValue();
	}

	if (!dtype)
		return RValue();

	if (dtype->isArray() || dtype->isEnum()) {
		return RValue::getNumVal(context, dtype->size());
	}

	context.addError("len operator invalid for " + dtype->str(&context) + " type", op->getName());
	return RValue();
}

RValue Inst::CallFunction(CodeContext& context, VecSFunc& funcs, Token* name, VecRValue& args)
{
	auto argCount = args.size();
	VecSFunc sizeMatch;
	copy_if(funcs.begin(), funcs.end(), back_inserter(sizeMatch), [=](auto i){ return i.numParams() == argCount; });

	if (sizeMatch.empty()) {
		// TODO make better error message with all functions
		context.addError("argument count for " + funcs[0].name().str() + " function invalid, "
			+ to_string(argCount) + " arguments given, but " + to_string(funcs[0].numParams()) + " required.", name);
		return {};
	}

	SFunction func;
	if (sizeMatch.size() == 1) {
		func = sizeMatch[0];
	} else {
		VecSFunc paramMatch;
		int bestCount = 1;
		for (auto mFunc : sizeMatch) {
			int matchCount = 0;
			for (size_t i = 0; i < argCount; i++) {
				if (mFunc.getParam(i) == args[i].stype()) {
					matchCount++;
				}
			}
			if (matchCount == bestCount) {
				paramMatch.push_back(mFunc);
			} else if (matchCount > bestCount) {
				bestCount = matchCount;
				paramMatch.clear();
				paramMatch.push_back(mFunc);
			}
		}
		if (paramMatch.size() != 1) {
			string argStr;
			bool second = false;
			for (auto arg : args) {
				if (second)
					argStr += ",";
				else
					second = true;
				argStr += arg.stype()->str(&context);
			}
			string msg = "arguments ambigious for overloaded function:\n\t";
			msg += "args:\n\t\t" + argStr + "\n\t" +
				"functions:";
			for (auto mFunc : sizeMatch)
				msg += "\n\t\t" + mFunc.stype()->str(&context);
			context.addError(msg, name);
			return {};
		}
		func = paramMatch[0];
	}

	for (size_t i = 0; i < func.numParams(); i++) {
		CastTo(context, name, args[i], func.getParam(i));
	}

	vector<Value*> values;
	copy(args.begin(), args.end(), back_inserter(values));
	auto call = context.IB().CreateCall(func.value(), values);
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
	auto className = type->subType()->str(&context);
	auto clType = static_cast<SClassType*>(type->subType());
	auto sym = clType->getItem(funcName->str);
	if (!sym) {
		context.addError("class " + className + " has no symbol " + funcName->str, funcName);
		return RValue();
	}

	VecSFunc funcs;
	auto isStatic = true;
	for (auto item : *sym) {
		if (item.second.isFunction()) {
			auto func = static_cast<SFunction&>(item.second);
			isStatic &= func.isStatic();
			funcs.push_back(func);
		}
	}
	if (funcs.empty()) {
		return CallMemberFunctionNonClass(context, baseVar, baseVal, funcName, arguments);
	}

	if (!isStatic && baseVal.isUndef()) {
		context.addError("unable to call non-static class function from a static function", funcName);
		return {};
	}

	auto args = CGNExpression::collect(context, arguments);
	if (!isStatic)
		args->insert(args->begin(), baseVal);
	return CallFunction(context, funcs, funcName, *args.get());
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

	VecSFunc funcs;
	funcs.push_back(static_cast<SFunction&>(sym));
	auto args = CGNExpression::collect(context, arguments);
	return CallFunction(context, funcs, funcName, *args.get());
}

bool Inst::CallConstructor(CodeContext& context, RValue var, RValue arrSize, VecRValue* initList, Token* token)
{
	bool isArr = false;
	auto varType = var.stype();
	SClassType* clType;

	if (varType->isClass()) {
		clType = static_cast<SClassType*>(varType);
	} else if (varType->isArray() && varType->subType()->isClass()) {
		isArr = true;
		clType = static_cast<SClassType*>(varType->subType());
	} else {
		return false;
	}

	auto funcs = clType->getConstructor();
	if (funcs.empty())
		return false;

	Value* endPtr;
	Value* nextPtr;
	if (isArr) {
		auto startBlock = context.currBlock();
		vector<Value*> idxs;
		auto zero = RValue::getZero(context, SType::getInt(context, 32));
		idxs.push_back(zero);
		idxs.push_back(zero);
		auto startPtr = context.IB().CreateGEP(nullptr, var, idxs);
		arrSize = arrSize ? arrSize : RValue::getNumVal(context, varType->size(), 64);
		endPtr = context.IB().CreateGEP(nullptr, startPtr, arrSize);

		auto block = context.createBlock();
		context.IB().CreateBr(block);
		context.pushBlock(block);

		auto phi = context.IB().CreatePHI(startPtr->getType(), 2);
		nextPtr = context.IB().CreateGEP(nullptr, phi, RValue::getNumVal(context, 1, 64));
		phi->addIncoming(startPtr, startBlock);
		phi->addIncoming(nextPtr, block);
		var = RValue(phi, clType);
	}

	VecRValue args;
	args.push_back({var.value(), SType::getPointer(context, var.stype())});
	if (initList)
		args.insert(args.end(), initList->begin(), initList->end());
	CallFunction(context, funcs, token, args);

	if (isArr) {
		auto cmp = context.IB().CreateICmpEQ(nextPtr, endPtr);
		auto block = context.createBlock();
		context.IB().CreateCondBr(cmp, block, context.currBlock());
		context.pushBlock(block);
	}
	return true;
}

void Inst::CallDestructor(CodeContext& context, RValue value, Token* valueToken)
{
	auto clType = static_cast<SClassType*>(value.stype()->subType());
	auto func = clType->getDestructor();
	if (!func)
		return;

	VecSFunc funcs;
	funcs.push_back(func);
	VecRValue args;
	args.push_back(value);
	CallFunction(context, funcs, valueToken, args);
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
	auto baseName = varType->str(&context);
	if (varType->isStruct()) {
		auto structType = static_cast<SStructType*>(varType);
		auto item = structType->getItem(member);
		if (!item) {
			context.addError(baseName + " doesn't have member " + member, memberName);
			return RValue();
		} else if (item->size() > 1) {
			context.addError("member is ambigious: " + member, memberName);
			return {};
		} else if (item->at(0).second.isFunction()) {
			return item->at(0).second;
		} else if (baseVar.isUndef()) {
			context.addError("cannot access member variable from a static context", memberName);
			return RValue();
		}

		vector<Value*> indexes;
		indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
		indexes.push_back(RValue::getNumVal(context, item->at(0).first));

		return GetElementPtr(context, baseVar, indexes, item->at(0).second.stype());
	} else if (varType->isUnion()) {
		auto unionType = static_cast<SUnionType*>(varType);
		auto item = unionType->getItem(member);
		if (!item) {
			context.addError(baseName + " doesn't have member " + member, memberName);
			return RValue();
		}

		auto ptr = SType::getPointer(context, item);
		auto castEl = context.IB().CreateBitCast(baseVar, *ptr);
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

void Inst::InitVariable(CodeContext& context, RValue var, const RValue& arrSize, VecRValue* initList, Token* token)
{
	if (CallConstructor(context, var, arrSize, initList, token))
		return;

	if (initList) {
		auto varType = var.stype();
		if (initList->empty()) {
			// no constructor and empty initializer; do zero initialization
			context.IB().CreateStore(RValue::getZero(context, varType), var);
			return;
		} else if (initList->size() > 1) {
			context.addError("invalid variable initializer", token);
			return;
		}
		auto initVal = initList->at(0);
		if (initVal) {
			CastTo(context, token, initVal, varType);
			context.IB().CreateStore(initVal, var);
		}
	}
}

RValue Inst::StoreTemporary(CodeContext& context, RValue value)
{
	auto stackAlloc = context.IB().CreateAlloca(value.type());
	context.IB().CreateStore(value, stackAlloc);
	return RValue(stackAlloc, value.stype());
}

RValue Inst::StoreTemporary(CodeContext& context, NExpression* exp)
{
	auto value = CGNExpression::run(context, exp);
	if (!value)
		return RValue();
	return StoreTemporary(context, value);
}
