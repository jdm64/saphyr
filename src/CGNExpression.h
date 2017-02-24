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
#ifndef __CG_NEXPRESSION_H__
#define __CG_NEXPRESSION_H__

class CGNExpression
{
	typedef RValue (CGNExpression::*classPtr)(NExpression*);

	static classPtr *vtable;

	CodeContext& context;

	explicit CGNExpression(CodeContext& context)
	: context(context) {}

	RValue visitNVariable(NVariable*);

	RValue visitNAddressOf(NAddressOf* nVar);

	RValue visitNArrowOperator(NArrowOperator*);

	RValue visitNAssignment(NAssignment*);

	RValue visitNTernaryOperator(NTernaryOperator*);

	RValue visitNNewExpression(NNewExpression*);

	RValue visitNLogicalOperator(NLogicalOperator*);

	RValue visitNCompareOperator(NCompareOperator*);

	RValue visitNBinaryMathOperator(NBinaryMathOperator*);

	RValue visitNNullCoalescing(NNullCoalescing*);

	RValue visitNUnaryMathOperator(NUnaryMathOperator*);

	RValue visitNFunctionCall(NFunctionCall*);

	RValue visitNMemberFunctionCall(NMemberFunctionCall*);

	RValue visitNIncrement(NIncrement*);

	RValue visitNBoolConst(NBoolConst*);

	RValue visitNNullPointer(NNullPointer*);

	RValue visitNStringLiteral(NStringLiteral*);

	RValue visitNIntConst(NIntConst*);

	RValue visitNFloatConst(NFloatConst*);

	RValue visitNCharConst(NCharConst*);

	void visit(NExpressionList*);

	RValue visit(NExpression*);

	static classPtr* buildVTable();

public:

	static RValue run(CodeContext& context, NExpression* exp)
	{
		CGNExpression runner(context);
		return runner.visit(exp);
	}

	static void run(CodeContext& context, NExpressionList* list)
	{
		CGNExpression runner(context);
		return runner.visit(list);
	}
};

#endif
