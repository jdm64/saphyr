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
#ifndef __FM_NEXPRESSION_H__
#define __FM_NEXPRESSION_H__

class FMNExpression
{
	FormatContext& context;

	explicit FMNExpression(FormatContext& context)
	: context(context) {}

	string visitNArrayVariable(NArrayVariable*);

	string visitNArrowOperator(NArrowOperator*);

	string visitNBaseVariable(NBaseVariable*);

	string visitNDereference(NDereference*);

	string visitNMemberVariable(NMemberVariable*);

	string visitNAddressOf(NAddressOf*);

	string visitNAssignment(NAssignment*);

	string visitNTernaryOperator(NTernaryOperator*);

	string visitNNewExpression(NNewExpression*);

	string visitNExprVariable(NExprVariable*);

	string visitNLogicalOperator(NLogicalOperator*);

	string visitNCompareOperator(NCompareOperator*);

	string visitNBinaryMathOperator(NBinaryMathOperator*);

	string visitNNullCoalescing(NNullCoalescing*);

	string visitNUnaryMathOperator(NUnaryMathOperator*);

	string visitNFunctionCall(NFunctionCall*);

	string visitNMemberFunctionCall(NMemberFunctionCall*);

	string visitNIncrement(NIncrement*);

	string visitNBoolConst(NBoolConst*);

	string visitNNullPointer(NNullPointer*);

	string visitNStringLiteral(NStringLiteral*);

	string visitNIntConst(NIntConst*);

	string visitNFloatConst(NFloatConst*);

	string visitNCharConst(NCharConst*);

	string visit(NExpressionList*);

	string visit(NExpression*);

public:

	static string run(FormatContext& context, NExpression* exp)
	{
		FMNExpression runner(context);
		return runner.visit(exp);
	}

	static string run(FormatContext& context, NExpressionList* list)
	{
		FMNExpression runner(context);
		return runner.visit(list);
	}
};

#endif
