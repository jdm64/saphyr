/*      Saphyr, a C++ style compiler using LLVM
        Copyright (C) 2012, Justin Madru (justin.jdm64@gmail.com)

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <llvm/Support/raw_ostream.h>
#include "CodeContext.h"

SType* SType::get(CodeContext& context, Type* type)
{
	if (type->isVoidTy())
		return SType::getVoid(context);
	else if (type->isIntegerTy())
		return SType::getInt(context, type->getIntegerBitWidth());
	else if (type->isFloatingPointTy())
		return SType::getFloat(context, type->isDoubleTy());
	else if (type->isArrayTy())
		return SType::getArray(context, SType::get(context, type->getArrayElementType()), type->getArrayNumElements());

	string str;
	raw_string_ostream stream(str);
	type->print(stream);
	cout << "BUG: can't wrap llvm::Type:\n" << endl;
	cout << stream.str() << endl;

	return nullptr;
}

SType* SType::getVoid(CodeContext& context)
{
	return new SType(SType::VOID, Type::getVoidTy(context));
}

SType* SType::getBool(CodeContext& context)
{
	return new SType(SType::INTEGER, Type::getIntNTy(context, 1), 1);
}

SType* SType::getInt(CodeContext& context, int bitWidth)
{
	auto type = Type::getIntNTy(context, bitWidth);
	return new SType(SType::INTEGER, type, bitWidth);
}

SType* SType::getFloat(CodeContext& context, bool doubleType)
{
	auto type = doubleType? Type::getDoubleTy(context) : Type::getFloatTy(context);
	return new SType(SType::FLOATING | (doubleType? SType::DOUBLE : 0), type);
}

SType* SType::getArray(CodeContext& context, SType* arrType, uint64_t size)
{
	auto type = ArrayType::get(arrType->type(), size);
	return new SType(SType::ARRAY, type, size, arrType);
}
