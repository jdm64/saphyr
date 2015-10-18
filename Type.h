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
#ifndef __TYPE_H__
#define __TYPE_H__

#include <map>
#include <llvm/IR/DataLayout.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Debug.h>
#include "Token.h"

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
	enum {
		INTEGER  = 1 << 0,
		UNSIGNED = 1 << 1,
		FLOATING = 1 << 2,
		DOUBLE   = 1 << 3,
		POINTER  = 1 << 4,
		VEC      = 1 << 5,
		ARRAY    = 1 << 6,
		ENUM     = 1 << 7,
		STRUCT   = 1 << 8,
		UNION    = 1 << 9,
		FUNCTION = 1 << 10,
		VOID     = 1 << 11,
		AUTO     = 1 << 12,
		ALIAS    = 1 << 13
	};

	static vector<Type*> convertArr(vector<SType*> arr)
	{
		vector<Type*> vec;
		for (auto item : arr)
			vec.push_back(*item);
		return vec;
	}

	static SType* getNumberLike(CodeContext& context, SType* type)
	{
		if (type->isNumeric()) {
			return type;
		} else if (type->tclass & (ARRAY | FUNCTION | POINTER)) {
			return getNumberLike(context, type->subtype);
		} else if (type->isVec()) {
			auto subT = type->subtype;
			return subT->isNumeric()? type : getNumberLike(context, subT);
		}
		// default to int32 for void, struct, union
		return getInt(context, 32);
	}

	/**
	 * used to the validity of types that can't be checked as its build.
	 * checks include:
	 *
	 * zero-size arrays on the stack
	 */
	static bool validate(CodeContext& context, Token* token, SType* type);

	static uint64_t allocSize(CodeContext& context, SType* type);

	static SType* numericConv(CodeContext& context, Token* optToken, SType* ltype, SType* rtype, bool int32min = true);

	static SType* getAuto(CodeContext& context);

	static SType* getVoid(CodeContext& context);

	static SType* getBool(CodeContext& context);

	static SType* getInt(CodeContext& context, int bitWidth, bool isUnsigned = false);

	static SType* getFloat(CodeContext& context, bool doubleType = false);

	static SType* getArray(CodeContext& context, SType* arrType, uint64_t size);

	static SType* getVec(CodeContext& context, SType* vecType, uint64_t size);

	static SType* getPointer(CodeContext& context, SType* ptrType);

	static SFunctionType* getFunction(CodeContext& context, SType* returnTy, vector<SType*> params);

	operator Type*() const
	{
		return ltype;
	}

	Type* type() const
	{
		return ltype;
	}

	bool isAlias() const
	{
		return tclass & ALIAS;
	}

	bool isAuto() const
	{
		return tclass & AUTO;
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

	/**
	 * @return true if the type is not a simple type
	 * @see http://llvm.org/docs/LangRef.html#single-value-types
	 */
	bool isComplex() const
	{
		return tclass & (ARRAY | STRUCT | UNION | FUNCTION);
	}

	bool isPointer() const
	{
		return tclass & POINTER;
	}

	bool isEnum() const
	{
		return tclass & ENUM;
	}

	bool isFunction() const
	{
		return tclass & FUNCTION;
	}

	SType* subType() const
	{
		return subtype;
	}

	SType* getScalar()
	{
		return isSequence()? subtype->getScalar() : this;
	}

	virtual void print(raw_ostream &os) const
	{
		if (isArray()) {
			os << "[" << size() << "]";
			subtype->print(os);
		} else if (isPointer()) {
			os << "@";
			subtype->print(os);
		} else if (isVec()) {
			os << "vec<" << size() << ",";
			subtype->print(os);
			os << ">";
		} else if (isDouble()) {
			os << "double:";
			ltype->print(os);
		} else if (isFloating()) {
			os << "float:";
			ltype->print(os);
		} else if (isBool()) {
			os << "bool:";
			ltype->print(os);
		} else if (isVoid()) {
			os << "void:";
			ltype->print(os);
		} else if (isInteger()) {
			if (isUnsigned())
				os << "u";
			os << "int" << size() << ":";
			ltype->print(os);
		} else if (isAuto()) {
			os << "auto";
		} else {
			os << "err:" << tclass << ":";
			ltype->print(os);
		}
	}

	void dump() const;

	virtual ~SType()
	{
		// nothing to do
	}
};

class SUserType : public SType
{
	friend class SStructType;
	friend class SUnionType;
	friend class SEnumType;
	friend class SAliasType;

	SUserType(int typeClass, Type* type, uint64_t size = 0, SType* subtype = nullptr)
	: SType(typeClass, type, size, subtype) {}

public:
	static SUserType* lookup(CodeContext& context, const string& name);

	static void createAlias(CodeContext& context, const string& name, SType* type);

	static void createStruct(CodeContext& context, const string& name, const vector<pair<string, SType*>>& structure);

	static void createUnion(CodeContext& context, const string& name, const vector<pair<string, SType*>>& structure);

	static void createEnum(CodeContext& context, const string& name, const vector<pair<string, int64_t>>& structure);
};

class SAliasType : public SUserType
{
	friend class TypeManager;

	explicit SAliasType(SType* type)
	: SUserType(ALIAS, type->type(), 0, type) {}
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
	pair<int, SType*>* getItem(const string& name)
	{
		auto iter = items.find(name);
		return iter != items.end()? &iter->second : nullptr;
	}

	void print(raw_ostream &os) const
	{
		os << "S:{|";
		for (auto i : items) {
			os << i.first << "=" << i.second.first << ":";
			i.second.second->print(os);
			os << "|";
		}
		os << "}";
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
	SType* getItem(const string& name)
	{
		auto iter = items.find(name);
		return iter != items.end()? iter->second : nullptr;
	}

	void print(raw_ostream &os) const
	{
		os << "U:{|";
		for (auto i : items) {
			os << i.first << "=";
			i.second->print(os);
			os << "|";
		}
		os << "}";
	}
};

class SEnumType : public SUserType
{
	friend class TypeManager;

	map<string, APSInt> items;

	SEnumType(SType* type, const vector<pair<string,int64_t>>& data)
	: SUserType(ENUM, *type, data.size(), type)
	{
		auto numbits = type->size();
		auto isUnsigned = !type->isUnsigned();
		for (auto item : data) {
			items[item.first] = APSInt(APInt(numbits, item.second), isUnsigned);
		}
	}

public:
	APSInt* getItem(const string& name)
	{
		auto iter = items.find(name);
		return iter != items.end()? &iter->second : nullptr;
	}

	void print(raw_ostream &os) const
	{
		os << "E:{|";
		for (auto i : items) {
			os << i.first << "=" << i.second << "|";
		}
		os << "}";
	}
};

class SFunctionType : public SType
{
	friend class TypeManager;

	vector<SType*> params;

	SFunctionType(FunctionType* type, SType* returnTy, vector<SType*> params)
	: SType(FUNCTION, type, 0, returnTy), params(std::move(params)) {}

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

	void print(raw_ostream &os) const
	{
		os << "(|";
		for (auto i : params) {
			i->print(os);
			os << "|";
		}
		os << ")";
		returnTy()->print(os);
	}
};

class TypeManager
{
	using STypePtr = unique_ptr<SType>;
	using SFuncPtr = unique_ptr<SFunctionType>;
	using SUserPtr = unique_ptr<SUserType>;

	DataLayout datalayout;

	// built-in types
	STypePtr autoTy, voidTy, boolTy, int8Ty, int16Ty, int32Ty, int64Ty, floatTy, doubleTy,
		uint8Ty, uint16Ty, uint32Ty, uint64Ty;

	// array & vec types
	map<pair<SType*, uint64_t>, STypePtr> arrMap;
	map<pair<SType*, uint64_t>, STypePtr> vecMap;

	// pointer types
	map<SType*, STypePtr> ptrMap;

	// user types
	map<string, SUserPtr> usrMap;

	// function types
	map<pair<SType*, vector<SType*> >, SFuncPtr> funcMap;

public:
	explicit TypeManager(Module* module);

	uint64_t allocSize(SType* stype)
	{
		return datalayout.getTypeAllocSize(*stype);
	}

	SType* getAuto() const
	{
		return autoTy.get();
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

	SType* getArray(SType* arrType, int64_t size);

	SType* getVec(SType* vecType, int64_t size);

	SType* getPointer(SType* ptrType);

	SFunctionType* getFunction(SType* returnTy, vector<SType*> args);

	SUserType* lookupUserType(const string& name)
	{
		return usrMap[name].get();
	}

	void createAlias(const string& name, SType* type);

	void createStruct(const string& name, vector<pair<string, SType*>> structure);

	void createUnion(const string& name, vector<pair<string, SType*>> structure);

	void createEnum(const string& name, vector<pair<string,int64_t>> structure);
};

#endif
