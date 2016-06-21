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
#ifndef __VALUE_H__
#define __VALUE_H__

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include "Type.h"

class RValue
{
	Value* val;
	SType* ty;

public:
	RValue()
	: RValue(nullptr, nullptr) {}

	RValue(Value* value, SType* type)
	: val(value), ty(type) {}

	static RValue getZero(CodeContext &context, SType* type);

	static RValue getNullPtr(CodeContext &context, SType* type);

	static RValue getNumVal(CodeContext &context, SType* type, int64_t value = 1);

	static RValue getAllOne(CodeContext &context, SType* type);

	static RValue getValue(CodeContext &context, const APSInt& intVal);

	operator bool() const
	{
		return val;
	}

	operator Value*() const
	{
		return val;
	}

	Value* value() const
	{
		return val;
	}

	Type* type() const
	{
		return val? val->getType() : nullptr;
	}

	SType* stype() const
	{
		return ty;
	}

	bool isFunction() const
	{
		auto type = stype();
		return type && type->isFunction();
	}

	bool isNullPtr()
	{
		return val? isa<ConstantPointerNull>(val) : false;
	}

	bool isConst() const
	{
		return val? isa<Constant>(val) : false;
	}

	SType* castToSubtype()
	{
		auto sub = ty->subType();
		if (sub)
			ty = sub;
		return ty;
	}
};

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

public:
	SFunction(Function* function, SFunctionType* type)
	: RValue(function, type) {}

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

	size_t numParams() const
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
