/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2014, Justin Madru (justin.jdm64@gmail.com)
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
#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <llvm/IR/Function.h>
#include "Value.h"

class SFunction : public RValue
{
	Function* funcValue() const
	{
		return static_cast<Function*>(value());
	}

	SFunctionType* funcStype() const
	{
		return static_cast<SFunctionType*>(stype());
	}

	SFunction(Function* function, SFunctionType* type)
	: RValue(function, type) {};

public:
	static SFunction create(CodeContext& context, const string& name, SFunctionType* type);

	SFunction()
	: RValue() {};

	operator Function*() const
	{
		return funcValue();
	}

	int size() const
	{
		return funcValue()->size();
	}

	StringRef name() const
	{
		return funcValue()->getName();
	}

	SType* returnTy() const
	{
		return funcStype()->returnTy();
	}

	int numParams() const
	{
		return funcStype()->numParams();
	}

	SType* getParam(int index) const
	{
		return funcStype()->getParam(index);
	}

	Function::arg_iterator arg_begin() const
	{
		return funcValue()->arg_begin();
	}

	Function::arg_iterator arg_end() const
	{
		return funcValue()->arg_end();
	}
};

#endif
