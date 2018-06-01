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
#ifndef __CGNVARIABLE_H__
#define __CGNVARIABLE_H__

class CGNVariable
{
	typedef RValue (CGNVariable::*classPtr)(NVariable*);

	static classPtr *vtable;

	CodeContext& context;

	explicit CGNVariable(CodeContext& context)
	: context(context) {}

	RValue visitNBaseVariable(NBaseVariable* baseVar);

	RValue visitNArrayVariable(NArrayVariable* nArrVar);

	RValue visitNArrowOperator(NArrowOperator* exp);

	RValue visitNMemberVariable(NMemberVariable* memVar);

	RValue visitNDereference(NDereference* nVar);

	RValue visitNAddressOf(NAddressOf* var);

	RValue visitNFunctionCall(NFunctionCall* var);

	RValue visitNExprVariable(NExprVariable* var);

	RValue visitNMemberFunctionCall(NMemberFunctionCall* var);

	RValue visit(NVariable* type);

	RValue MutCast(NArrowOperator* exp);

	static classPtr* buildVTable();

public:

	static RValue run(CodeContext& context, NVariable* exp)
	{
		CGNVariable runner(context);
		return runner.visit(exp);
	}
};

#endif
