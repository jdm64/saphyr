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

#include <iostream>
#include "Value.h"
#include "CodeContext.h"

RValue RValue::getZero(CodeContext &context, SType* type)
{
	// llvm's Constant::getNullValue() supports every type
	// except function, label, and opaque type
	if (type->isFunction())
		type = SType::getNumberLike(context, type);
	return RValue(Constant::getNullValue(*type), type);
}

RValue RValue::getNullPtr(CodeContext &context, SType* type)
{
	auto ptrType = type->isPointer()? type : SType::getPointer(context, type);
	return RValue::getZero(context, ptrType);
}

RValue RValue::getNumVal(CodeContext &context, SType* type, int64_t value)
{
	auto numlike = SType::getNumberLike(context, type);
	auto basenum = numlike->getScalar();
	auto one = basenum->isFloating()?
		ConstantFP::get(*numlike, value) :
		ConstantInt::getSigned(*numlike, value);
	return RValue(one, numlike);
}

RValue RValue::getNumVal(CodeContext &context, int64_t value, int bitwidth, bool isUnsigned)
{
	auto type = SType::getInt(context, bitwidth, isUnsigned);
	return getNumVal(context, type, value);
}

RValue RValue::getAllOne(CodeContext &context, SType* type)
{
	auto numlike = SType::getNumberLike(context, type);
	return RValue(Constant::getAllOnesValue(*numlike), type);
}

RValue RValue::getValue(CodeContext &context, const APSInt& intVal)
{
	auto val = ConstantInt::get(context, intVal);
	auto type = SType::getInt(context, intVal.getBitWidth(), intVal.isUnsigned());
	return RValue(val, type);
}

RValue RValue::getUndef(SType* type)
{
	return RValue(UndefValue::get(*type), type);
}

void RValue::dump(CodeContext &context)
{
	cout << ty->str(context) << endl;
	val->print(llvm::dbgs());
	cout << endl;
}

SFunction SFunction::create(CodeContext& context, Function* function, SFunctionType* type, NAttributeList* attrs)
{
	return SFunction(function, type, context.storeAttr(attrs));
}
