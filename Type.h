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
class SFunctionType;

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
	enum { VOID = 0x1, INTEGER = 0x2, FLOATING = 0x4, DOUBLE = 0x8, ARRAY = 0x10, FUNCTION = 0x20 };

	static vector<Type*> convertArr(vector<SType*> arr)
	{
		vector<Type*> vec;
		for (auto item : arr)
			vec.push_back(*item);
		return vec;
	}

	static SType* opType(CodeContext& context, SType* ltype, SType* rtype, bool int32min = true);

	static SType* getVoid(CodeContext& context);

	static SType* getBool(CodeContext& context);

	static SType* getInt(CodeContext& context, int bitWidth);

	static SType* getFloat(CodeContext& context, bool doubleType = false);

	static SType* getArray(CodeContext& context, SType* arrType, uint64_t size);

	static SFunctionType* getFunction(CodeContext& context, SType* returnTy, vector<SType*> params);

	operator Type*() const
	{
		return ltype;
	}

	Type* type() const
	{
		return ltype;
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

	bool isFunction() const
	{
		return tclass & FUNCTION;
	}

	SType* arrType() const
	{
		return subtype;
	}
};

class SFunctionType : public SType
{
private:
	friend class TypeManager;

	vector<SType*> params;

	SFunctionType(FunctionType* type, SType* returnTy, vector<SType*> params)
	: SType(FUNCTION, type, 0, returnTy), params(params) {}

public:
	using ParamIter = vector<SType*>::iterator;

	operator FunctionType*() const
	{
		return (FunctionType*) type();
	}

	FunctionType* funcType() const
	{
		return (FunctionType*) type();
	}

	SType* returnTy() const
	{
		return subtype;
	}

	int numParams() const
	{
		return params.size();
	}

	SType* getParam(int index) const
	{
		return params[index];
	}
};

#define smart_stype(tclass, type, size, subtype) unique_ptr<SType>(new SType(tclass, type, size, subtype))
#define smart_sfuncTy(func, rtype, args) unique_ptr<SFunctionType>(new SFunctionType(func, rtype, args))

class TypeManager
{
	using STypePtr = unique_ptr<SType>;
	using SFuncPtr = unique_ptr<SFunctionType>;

private:
	LLVMContext& context;

	// built-in types
	STypePtr voidTy, boolTy, int8Ty, int16Ty, int32Ty, int64Ty, floatTy, doubleTy;

	// array types
	map<pair<SType*, uint64_t>, STypePtr> arrMap;

	// function types
	map<pair<SType*, vector<SType*> >, SFuncPtr> funcMap;

public:
	TypeManager(LLVMContext& ctx);

	SType* getVoid() const
	{
		return voidTy.get();
	}

	SType* getBool() const
	{
		return boolTy.get();
	}

	SType* getInt(int bitWidth) const
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

	SType* getFloat(bool doubleType = false) const
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

	SFunctionType* getFunction(SType* returnTy, vector<SType*> args)
	{
		SFuncPtr &item = funcMap[make_pair(returnTy, args)];
		if (!item.get()) {
			auto func = FunctionType::get(*returnTy, SType::convertArr(args), false);
			item = smart_sfuncTy(func, returnTy, args);
		}
		return item.get();
	}
};

#endif
