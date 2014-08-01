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

#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
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

	static RValue getZero(CodeContext &context, SType* type)
	{
		return RValue(Constant::getNullValue(*type), type);
	}

	static RValue getOne(CodeContext &context, SType* type)
	{
		auto one = type->isFloating()?
			ConstantFP::get(type->type(), "1.0") :
			ConstantInt::getSigned(type->type(), 1);
		return RValue(one, type);
	}

	static RValue getAllOne(CodeContext &context, SType* type)
	{
		return RValue(Constant::getAllOnesValue(*type), type);
	}

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
};

class LValue : public RValue
{
public:
	LValue()
	: RValue() {}

	LValue(Value* value, SType* type)
	: RValue(value, type) {}
};

#endif
