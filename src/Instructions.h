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
#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__

#include "AST.h"
#include "CodeContext.h"

typedef CmpInst::Predicate Predicate;
typedef Instruction::BinaryOps BinaryOps;
typedef Instruction::CastOps CastOps;

class Inst
{
	static inline void castError(CodeContext& context, const string& msg, SType* from, SType* to, Token* token);

	static BinaryOps getOperator(int oper, Token* optToken, SType* type, CodeContext& context);

	static Predicate getPredicate(int oper, Token* token, SType* type, CodeContext& context);

	static void NumericCast(RValue& value, SType* from, SType* to, SType* final, CodeContext& context);

	static bool CastMatch(CodeContext& context, Token* optToken, RValue& lhs, RValue& rhs, bool upcast = false);

	static RValue PointerMath(int type, Token* optToken, RValue ptr, const RValue& val, CodeContext& context);

	static RValue CallMemberFunctionNonClass(CodeContext& context, NVariable* baseVar, RValue& baseVal, Token* funcName, NExpressionList* arguments);

public:
	static bool CastTo(CodeContext& context, Token* token, RValue& value, SType* type, bool upcast = false);

	static RValue CastAs(CodeContext& context, NArrowOperator* exp);

	static RValue BinaryOp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context);

	static RValue Branch(BasicBlock* trueBlock, BasicBlock* falseBlock, NExpression* condExp, CodeContext& context);

	static RValue Cmp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context);

	static RValue Load(CodeContext& context, RValue value);

	static RValue Deref(CodeContext& context, const RValue& value, bool recursive = false);

	static RValue SizeOf(CodeContext& context, SType* type, Token* token);

	static RValue SizeOf(CodeContext& context, Token* name);

	static RValue SizeOf(CodeContext& context, NArrowOperator* exp);

	inline static RValue GetElementPtr(CodeContext& context, const RValue& ptr, ArrayRef<Value*> idxs, SType* type)
	{
		auto ptrVal = GetElementPtrInst::Create(nullptr, ptr, idxs, "", context);
		return RValue(ptrVal, type);
	}

	static RValue CallFunction(CodeContext& context, SFunction& func, Token* name, NExpressionList* args, vector<Value*>& expList);

	static RValue CallMemberFunction(CodeContext& context, NVariable* baseVar, Token* funcName, NExpressionList* arguments);

	static RValue CallMemberFunctionClass(CodeContext& context, NVariable* baseVar, RValue& baseVal, Token* funcName, NExpressionList* arguments);

	static bool CallConstructor(CodeContext& context, RValue var, Token* token, NExpressionList* initList);

	static void CallDestructor(CodeContext& context, RValue value, Token* valueToken);

	static RValue LoadMemberVar(CodeContext& context, const string& name);

	static RValue LoadMemberVar(CodeContext& context, RValue baseVar, Token* baseToken, Token* memberName);

	static void InitVariable(CodeContext& context, RValue var, Token* token, NExpressionList* initList, RValue& initVal);

	static RValue StoreTemporary(CodeContext& context, RValue value);

	static RValue StoreTemporary(CodeContext& context, NExpression* exp);
};

#endif
