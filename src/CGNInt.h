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
#ifndef __CG_NINT_H__
#define __CG_NINT_H__

class CGNInt
{
	typedef APSInt (CGNInt::*classPtr)(NIntLikeConst*);

	static classPtr *vtable;

	CodeContext& context;

	explicit CGNInt(CodeContext& context)
	: context(context) {}

	APSInt visitNBoolConst(NBoolConst*);

	APSInt visitNIntConst(NIntConst*);

	APSInt visitNCharConst(NCharConst*);

	APSInt visit(NIntLikeConst*);

	static classPtr* buildVTable();

public:

	static APSInt run(CodeContext& context, NIntLikeConst* intConst)
	{
		CGNInt runner(context);
		return runner.visit(intConst);
	}
};

#endif
