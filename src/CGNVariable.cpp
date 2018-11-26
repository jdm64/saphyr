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
#include "CGNVariable.h"
#include "Instructions.h"
#include "CGNExpression.h"

RValue CGNVariable::visit(NVariable* type)
{
	switch (type->id()) {
	VISIT_CASE_RETURN(NAddressOf, type)
	VISIT_CASE_RETURN(NArrayVariable, type)
	VISIT_CASE_RETURN(NArrowOperator, type)
	VISIT_CASE_RETURN(NBaseVariable, type)
	VISIT_CASE_RETURN(NDereference, type)
	VISIT_CASE_RETURN(NExprVariable, type)
	VISIT_CASE_RETURN(NFunctionCall, type)
	VISIT_CASE_RETURN(NMemberFunctionCall, type)
	VISIT_CASE_RETURN(NMemberVariable, type)
	default:
		context.addError("NodeId::" + to_string(static_cast<int>(type->id())) + " unrecognized in CGNVariable", *type);
		return RValue();
	}
}

RValue CGNVariable::visitNBaseVariable(NBaseVariable* baseVar)
{
	auto varName = baseVar->getName()->str;

	// check current function
	auto var = context.loadSymbolLocal(varName);
	if (var)
		return var;

	// check class variables
	auto currClass = context.getClass();
	if (currClass) {
		auto item = currClass->getItem(varName);
		if (item) {
			if (context.currFunction().isStatic()) {
				context.addError("use of class member invalid in static function", *baseVar);
				return RValue();
			}
			return Inst::LoadMemberVar(context, varName);
		}
	}

	// check global variables
	var = context.loadSymbolGlobal(varName);
	if (var)
		return var;

	// check user types
	bool hasErrors = false;
	auto userVar = SUserType::lookup(context, baseVar->getName(), {}, hasErrors);
	if (!userVar) {
		if (context.currFunction().isStatic()) {
			context.addError("use of class member invalid in static function", *baseVar);
		} else if (!hasErrors) {
			context.addError("variable " + varName + " not declared", *baseVar);
		}
		return var;
	}
	return RValue::getUndef(userVar);
}

RValue CGNVariable::visitNArrayVariable(NArrayVariable* nArrVar)
{
	auto indexVal = CGNExpression::run(context, nArrVar->getIndex());

	if (!indexVal) {
		return indexVal;
	} else if (!(indexVal.stype()->isNumeric() || indexVal.stype()->isEnum())) {
		context.addError("array index is not able to be cast to an int", *nArrVar->getIndex());
		return RValue();
	}

	auto var = visit(nArrVar->getArrayVar());
	if (!var)
		return var;
	var = Inst::Deref(context, var, true);

	if (!var.stype()->isSequence()) {
		context.addError(var.stype()->str(&context) + " is not an array or vec", *nArrVar->getArrayVar());
		return RValue();
	}
	Inst::CastTo(context, *nArrVar->getIndex(), indexVal, SType::getInt(context, 64));

	vector<Value*> indexes;
	indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
	indexes.push_back(indexVal);

	return Inst::GetElementPtr(context, var, indexes, var.stype()->subType());
}

RValue CGNVariable::visitNArrowOperator(NArrowOperator* exp)
{
	auto name = exp->getName()->str;
	if (name == "size") {
		return Inst::SizeOf(context, exp);
	} else if (name == "as") {
		return Inst::CastAs(context, exp);
	} else if (name == "mut") {
		return Inst::MutCast(context, exp);
	} else if (name == "len") {
		return Inst::LenOp(context, exp);
	}
	context.addError("invalid arrow op name: " + name, *exp);
	return RValue();
}

RValue CGNVariable::visitNMemberVariable(NMemberVariable* memVar)
{
	auto var = visit(memVar->getBaseVar());
	if (!var)
		return RValue();

	var = Inst::Deref(context, var, true);
	return Inst::LoadMemberVar(context, var, *memVar->getBaseVar(), memVar->getMemberName());
}

RValue CGNVariable::visitNDereference(NDereference* nVar)
{
	auto var = visit(nVar->getVar());
	if (!var) {
		return var;
	} else if (!var.stype()->isPointer()) {
		context.addError("cannot dereference " + var.stype()->str(&context), *nVar);
		return RValue();
	}
	return Inst::Deref(context, var);
}

RValue CGNVariable::visitNAddressOf(NAddressOf* var)
{
	return visit(var->getVar());
}

RValue CGNVariable::visitNFunctionCall(NFunctionCall* var)
{
	return Inst::StoreTemporary(context, var);
}

RValue CGNVariable::visitNExprVariable(NExprVariable* var)
{
	return Inst::StoreTemporary(context, var->getExp());
}

RValue CGNVariable::visitNMemberFunctionCall(NMemberFunctionCall* var)
{
	return Inst::StoreTemporary(context, var);
}
