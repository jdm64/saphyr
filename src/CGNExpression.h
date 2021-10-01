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
	CodeContext& context;
	bool derefRef;

	explicit CGNExpression(CodeContext& context, bool derefRef = true)
	: context(context), derefRef(derefRef) {}

	RValue visitNVariable(NVariable*);

	RValue visitNAddressOf(NAddressOf* nVar);

	RValue visitNAssignment(NAssignment*);

	RValue visitNTernaryOperator(NTernaryOperator*);

	RValue visitNNewExpression(NNewExpression*);

	RValue visitNLambdaFunction(NLambdaFunction*);

	RValue visitNExprVariable(NExprVariable*);

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

public:

	static RValue run(CodeContext& context, NExpression* exp, bool derefRef = true)
	{
		CGNExpression runner(context, derefRef);
		return runner.visit(exp);
	}

	static void run(CodeContext& context, NExpressionList* list)
	{
		CGNExpression runner(context);
		return runner.visit(list);
	}

	static uPtr<VecRValue> collect(CodeContext& context, NExpressionList* list)
	{
		uPtr<VecRValue> ret;
		if (list) {
			ret = uPtr<VecRValue>(new VecRValue());
			CGNExpression runner(context);
			transform(list->begin(), list->end(), back_inserter(*ret), [&](auto i){ return runner.visit(i); });
		}
		return ret;
	}
};

#endif
