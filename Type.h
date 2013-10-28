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
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <llvm/DataLayout.h>
#include <llvm/Support/raw_ostream.h>

// forward declaration
class CodeContext;
class SFunctionType;
class SStructType;

using namespace std;
using namespace llvm;

class SType
{
protected:
	friend class TypeManager;
	friend class SUserType;

	int tclass;
	Type* ltype;
	uint64_t tsize;
	SType* subtype;

	SType(int typeClass, Type* type, uint64_t size = 0, SType* subtype = nullptr)
	: tclass(typeClass), ltype(type), tsize(size), subtype(subtype) {}

public:
	enum { VOID = 0x1, INTEGER = 0x2, UNSIGNED = 0x4, FLOATING = 0x8, DOUBLE = 0x10,
		ARRAY = 0x20, FUNCTION = 0x40, STRUCT = 0x80, VEC = 0x100, UNION = 0x200 };

	static vector<Type*> convertArr(vector<SType*> arr)
	{
		vector<Type*> vec;
		for (auto item : arr)
			vec.push_back(*item);
		return vec;
	}

	static uint64_t allocSize(CodeContext& context, SType* type);

	static SType* numericConv(CodeContext& context, SType* ltype, SType* rtype, bool int32min = true);

	static SType* getVoid(CodeContext& context);

	static SType* getBool(CodeContext& context);

	static SType* getInt(CodeContext& context, int bitWidth, bool isUnsigned = false);

	static SType* getFloat(CodeContext& context, bool doubleType = false);

	static SType* getArray(CodeContext& context, SType* arrType, uint64_t size);

	static SType* getVec(CodeContext& context, SType* vecType, uint64_t size);

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

	bool isUnsigned() const
	{
		return tclass & UNSIGNED;
	}

	int size() const
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

	bool isNumeric() const
	{
		return tclass & (INTEGER | FLOATING);
	}

	bool isArray() const
	{
		return tclass & ARRAY;
	}

	bool isVec() const
	{
		return tclass & VEC;
	}

	bool isUnion() const
	{
		return tclass & UNION;
	}

	bool isSequence() const
	{
		return tclass & (ARRAY | VEC);
	}

	bool isStruct() const
	{
		return tclass & STRUCT;
	}

	bool isComposite() const
	{
		return tclass & (ARRAY | STRUCT | VEC | UNION);
	}

	bool isFunction() const
	{
		return tclass & FUNCTION;
	}

	SType* subType() const
	{
		return subtype;
	}
};

class SUserType : public SType
{
	friend class SStructType;
	friend class SUnionType;

	SUserType(int typeClass, Type* type, uint64_t size = 0, SType* subtype = nullptr)
	: SType(typeClass, type, size, subtype) {}

public:
	static SUserType* lookup(CodeContext& context, string* name);

	static void createStruct(CodeContext& context, string* name, const vector<pair<string, SType*>>& structure);

	static void createUnion(CodeContext& context, string* name, const vector<pair<string, SType*>>& structure);
};

class SStructType : public SUserType
{
	friend class TypeManager;

	map<string, pair<int, SType*>> items;

	SStructType(StructType* type, const vector<pair<string, SType*> >& structure)
	: SUserType(STRUCT, type, structure.size())
	{
		int i = 0;
		for (auto var : structure)
			items[var.first] = make_pair(i++, var.second);
	}

public:
	pair<int, SType*>* getItem(string* name)
	{
		auto iter = items.find(*name);
		return iter != items.end()? &iter->second : nullptr;
	}
};

class SUnionType : public SUserType
{
	friend class TypeManager;

	map<string, SType*> items;

	SUnionType(StructType* type, const vector<pair<string, SType*> >& structure, uint64_t size)
	: SUserType(UNION, type, size)
	{
		for (auto var : structure)
			items[var.first] = var.second;
	}

public:
	SType* getItem(string* name)
	{
		auto iter = items.find(*name);
		return iter != items.end()? iter->second : nullptr;
	}
};

class SFunctionType : public SType
{
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
#define smart_strucTy(type, structure) unique_ptr<SUserType>(new SStructType(type, structure))
#define smart_unionTy(type, structure, size) unique_ptr<SUserType>(new SUnionType(type, structure, size))

class TypeManager
{
	using STypePtr = unique_ptr<SType>;
	using SFuncPtr = unique_ptr<SFunctionType>;
	using SUserPtr = unique_ptr<SUserType>;

	DataLayout datalayout;

	// built-in types
	STypePtr voidTy, boolTy, int8Ty, int16Ty, int32Ty, int64Ty, floatTy, doubleTy,
		uint8Ty, uint16Ty, uint32Ty, uint64Ty;

	// array & vec types
	map<pair<SType*, uint64_t>, STypePtr> arrMap;
	map<pair<SType*, uint64_t>, STypePtr> vecMap;

	// user types
	map<string, SUserPtr> usrMap;

	// function types
	map<pair<SType*, vector<SType*> >, SFuncPtr> funcMap;

public:
	TypeManager(Module* module);

	uint64_t allocSize(SType* stype)
	{
		return datalayout.getTypeAllocSize(*stype);
	}

	SType* getVoid() const
	{
		return voidTy.get();
	}

	SType* getBool() const
	{
		return boolTy.get();
	}

	SType* getInt(int bitWidth, bool isUnsigned = false) const
	{
		switch (bitWidth) {
		case 1:  return boolTy.get();
		case 8:  return isUnsigned? uint8Ty.get() : int8Ty.get();
		case 16: return isUnsigned? uint16Ty.get() : int16Ty.get();
		case 32: return isUnsigned? uint32Ty.get() : int32Ty.get();
		case 64: return isUnsigned? uint64Ty.get() : int64Ty.get();
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

	SType* getVec(SType* vecType, int64_t size)
	{
		STypePtr &item = arrMap[make_pair(vecType, size)];
		if (!item.get())
			item = smart_stype(SType::VEC, VectorType::get(*vecType, size), size, vecType);
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

	SUserType* lookupUserType(string* name)
	{
		return usrMap[*name].get();
	}

	void createStruct(string* name, vector<pair<string, SType*>> structure)
	{
		SUserPtr& item = usrMap[*name];
		if (item.get())
			return;
		vector<Type*> elements;
		for (auto item : structure)
			elements.push_back(*item.second);
		auto type = StructType::create(elements, *name);
		item = smart_strucTy(type, structure);
	}

	void createUnion(string* name, vector<pair<string, SType*>> structure)
	{
		SUserPtr& item = usrMap[*name];
		if (item.get())
			return;
		int size = 0;
		SType* type = nullptr;
		for (auto item : structure) {
			auto tsize = allocSize(item.second);
			if (tsize > size) {
				size = tsize;
				type = item.second;
			}
		}
		vector<Type*> elements;
		elements.push_back(*type);
		item = smart_unionTy(StructType::create(elements, *name), structure, size);
	}
};

#endif
