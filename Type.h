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
#ifndef __TYPE_H__
#define __TYPE_H__

#include <iostream>
#include <map>
#include <memory>
#include <llvm/LLVMContext.h>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>

// forward declaration
class CodeContext;

using namespace std;
using namespace llvm;

class SType
{
protected:
	friend class TypeManager;

	int tclass;
	Type* ltype;
	uint64_t tsize;
	SType* subtype;

	SType(int typeClass, Type* type, uint64_t size = 0, SType* subtype = nullptr)
	: tclass(typeClass), ltype(type), tsize(size), subtype(subtype) {}

public:
	enum { VOID = 0x1, INTEGER = 0x2, FLOATING = 0x4, DOUBLE = 0x8, ARRAY = 0x10 };

	static SType* get(CodeContext& context, Type* type);

	static SType* getVoid(CodeContext& context);

	static SType* getBool(CodeContext& context);

	static SType* getInt(CodeContext& context, int bitWidth);

	static SType* getFloat(CodeContext& context, bool doubleType = false);

	static SType* getArray(CodeContext& context, SType* arrType, uint64_t size);

	operator Type*() const
	{
		return ltype;
	}

	Type* type() const
	{
		return ltype;
	}

	bool matches(SType* valueType) const
	{
		return ltype == valueType->ltype;
	}

	bool isVoid() const
	{
		return tclass & VOID;
	}

	bool isBool() const
	{
		return isInteger() && tsize == 1;
	}

	bool isInteger() const
	{
		return tclass & INTEGER;
	}

	int intSize() const
	{
		return tsize;
	}

	bool isFloating() const
	{
		return tclass & FLOATING;
	}

	bool isDouble() const
	{
		return tclass & DOUBLE;
	}

	bool isArray() const
	{
		return tclass & ARRAY;
	}

	SType* arrType() const
	{
		return subtype;
	}
};

#define smart_stype(tclass, type, size, subtype) unique_ptr<SType>(new SType(tclass, type, size, subtype))

class TypeManager
{
	using STypePtr = unique_ptr<SType>;

private:
	LLVMContext& context;

	// built-in types
	STypePtr voidTy, boolTy, int8Ty, int16Ty, int32Ty, int64Ty, floatTy, doubleTy;

	// array types
	map<pair<SType*, uint64_t>, STypePtr> arrMap;

public:
	TypeManager(LLVMContext& ctx);

	SType* get(Type* type)
	{
		if (type->isVoidTy())
			return getVoid();
		else if (type->isIntegerTy())
			return getInt(type->getIntegerBitWidth());
		else if (type->isFloatingPointTy())
			return getFloat(type->isDoubleTy());
		else if (type->isArrayTy())
			return getArray(get(type->getArrayElementType()), type->getArrayNumElements());

		// BUG - type not handled
		string str;
		raw_string_ostream stream(str);
		type->print(stream);
		cout << "BUG: can't wrap llvm::Type:\n" << endl;
		cout << stream.str() << endl;

		return nullptr;
	}

	SType* getVoid() { return voidTy.get(); }

	SType* getBool() { return boolTy.get(); }

	SType* getInt(int bitWidth)
	{
		switch (bitWidth) {
		case 1:  return boolTy.get();
		case 8:  return int8Ty.get();
		case 16: return int16Ty.get();
		case 32: return int32Ty.get();
		case 64: return int64Ty.get();
		default: return nullptr;
		}
	}

	SType* getFloat(bool doubleType = false)
	{
		return doubleType? doubleTy.get() : floatTy.get();
	}

	SType* getArray(SType* arrType, int64_t size)
	{
		STypePtr &item = arrMap[make_pair(arrType, size)];
		if (!item.get())
			item = smart_stype(SType::ARRAY, ArrayType::get(*arrType, size), size, arrType);
		return item.get();
	}
};

#endif
