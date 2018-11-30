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
#ifndef __TYPE_H__
#define __TYPE_H__

#include <map>
#include <llvm/IR/DataLayout.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Debug.h>
#include "AST.h"

// forward declaration
class CodeContext;
class SFunctionType;
class SStructType;
class SFunction;
class RValue;
class TypeManager;

using namespace std;
using namespace llvm;

class SType
{
protected:
	friend class TypeManager;
	friend class SUserType;
	friend class SAliasType;
	friend class SStructType;
	friend class SUnionType;

	int tclass;
	Type* ltype;
	uint64_t tsize;
	SType* subtype;

	SType(int typeClass, Type* type, uint64_t size = 0, SType* subtype = nullptr)
	: tclass(typeClass), ltype(type), tsize(size), subtype(subtype) {}

	virtual SType* copy()
	{
		return new SType(*this);
	}

	virtual void setConst(TypeManager* tmang);

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
		ALIAS    = 1 << 13,
		CLASS    = 1 << 14,
		OPAQUE   = 1 << 15,
		CONST    = 1 << 16,
		TEMPLATED = 1 << 17
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

	static SType* getConst(CodeContext& context, SType* type);

	static SType* getMutable(CodeContext& context, SType* type);

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

	bool isConst() const
	{
		return tclass & CONST;
	}

	bool isVoid() const
	{
		return tclass & VOID;
	}

	bool isOpaque() const
	{
		return tclass & OPAQUE;
	}

	bool isUnsized() const
	{
		return tclass & (AUTO | VOID | OPAQUE);
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

	uint64_t size() const
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

	bool isClass() const
	{
		return tclass & CLASS;
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

	bool isTemplated() const
	{
		return tclass & TEMPLATED;
	}

	SType* subType() const
	{
		return subtype;
	}

	SType* getScalar()
	{
		return isSequence()? subtype->getScalar() : this;
	}

	virtual string str(CodeContext* context = nullptr) const
	{
		string s;
		raw_string_ostream os(s);

		if (isConst())
			os << "const ";

		if (isArray()) {
			os << "[";
			auto s = size();
			if (s)
				os << s;
			os << "]" << subtype->str(context);
		} else if (isPointer()) {
			os << "@" << subtype->str(context);
		} else if (isVec()) {
			os << "vec<" << size() << "," << subtype->str(context) << ">";
		} else {
			if (isDouble()) {
				os << "double";
			} else if (isFloating()) {
				os << "float";
			} else if (isBool()) {
				os << "bool";
			} else if (isVoid()) {
				os << "void";
			} else if (isInteger()) {
				if (isUnsigned())
					os << "u";
				os << "int" << size();
			} else if (isAuto()) {
				os << "auto";
			} else {
				os << "err:" << tclass;
				ltype->print(os);
			}
			if (!context) {
				os << ":";
				ltype->print(os);
			}
		}
		return os.str();
	}

	virtual string raw()
	{
		string s;
		raw_string_ostream os(s);

		if (isConst())
			os << "c_";

		if (isArray()) {
			os << "a";
			auto s = size();
			if (s)
				os << s;
			os << "_" << subtype->raw();
		} else if (isPointer()) {
			os << "p_" << subtype->raw();
		} else if (isVec()) {
			os << "v" << size() << "_" << subtype->raw();
		} else {
			if (isDouble()) {
				os << "d";
			} else if (isFloating()) {
				os << "f";
			} else if (isBool()) {
				os << "b";
			} else if (isVoid()) {
				os << "v";
			} else if (isInteger()) {
				if (isUnsigned())
					os << "u";
				else
					os << "i";
				os << size();
			} else if (isAuto()) {
				os << "x";
			} else {
				os << "e" << tclass << "_";
				ltype->print(os);
			}
		}
		return os.str();
	}

	void dump() const;

	virtual ~SType()
	{
		// nothing to do
	}
};

class SClassType;

class SUserType : public SType
{
	friend class SStructType;
	friend class SUnionType;
	friend class SEnumType;
	friend class SAliasType;

protected:
	string name;

	SUserType(const string& name, int typeClass, Type* type, uint64_t size = 0, SType* subtype = nullptr)
	: SType(typeClass, type, size, subtype), name(name) {}

	void innerStr(CodeContext* context, stringstream& os) const;

public:
	string raw() override
	{
		return name;
	}

	static string raw(const string& name, const vector<SType*>& templateArgs)
	{
		auto ret = name;
		for (auto item : templateArgs)
			ret += "_" + item->raw();
		return ret;
	}

	static bool isDeclared(CodeContext& context, const string& name, const vector<SType*>& templateArgs);

	static SType* lookup(CodeContext& context, Token* name, vector<SType*> templateArgs, bool& hasErrors);

	static void createAlias(CodeContext& context, const string& name, SType* type);

	static void createStruct(CodeContext& context, const string& name, const vector<pair<string, SType*>>& structure, const vector<SType*>& templateArgs);

	static SClassType* createClass(CodeContext& context, const string& name, const vector<pair<string, SType*>>& structure, const vector<SType*>& templateArgs);

	static void createUnion(CodeContext& context, const string& name, const vector<pair<string, SType*>>& structure, const vector<SType*>& templateArgs);

	static void createEnum(CodeContext& context, const string& name, const vector<pair<string, int64_t>>& structure, SType* type);
};

class SAliasType : public SUserType
{
	friend class TypeManager;

	explicit SAliasType(const string& name, SType* type)
	: SUserType(name, ALIAS, type->type(), 0, type) {}

protected:
	void setConst(TypeManager* tmang) override;

public:
	string str(CodeContext* context = nullptr) const override
	{
		return subtype->str(context);
	}

	string raw() override
	{
		return subtype->raw();
	}
};

class STemplatedType : public SUserType
{
protected:
	vector<SType*> templateArgs;

	STemplatedType(const string& name, int typeClass, Type* type, uint64_t size, const vector<SType*>& templateArgs)
	: SUserType(name, typeClass | (templateArgs.size() ? TEMPLATED : 0), type, size), templateArgs(templateArgs) {}

public:
	vector<SType*> getTemplateArgs() const
	{
		return templateArgs;
	}
};

class SStructType : public STemplatedType
{
	friend class TypeManager;
	friend class SClassType;

	typedef map<string, pair<int, RValue>> container;
	typedef container::const_iterator const_iterator;

protected:
	container items;

	SStructType(const string& name, StructType* type, const vector<pair<string, SType*>>& structure, const vector<SType*>& templateArgs, int ctype = STRUCT);

	SType* copy() override
	{
		return new SStructType(*this);
	}

	void setConst(TypeManager* tmang) override;

public:
	pair<int, RValue>* getItem(const string& name);

	string str(CodeContext* context = nullptr) const override;

	string raw() override
	{
		auto raw = name;
		for (auto item : templateArgs)
			raw += "_" + item->raw();
		return raw;
	}

	const_iterator begin() const
	{
		return items.begin();
	}

	const_iterator end() const
	{
		return items.end();
	}
};

class SClassType : public SStructType
{
	friend class TypeManager;

	SClassType(const string& name, StructType* type, const vector<pair<string, SType*>>& structure, const vector<SType*>& templateArgs)
	: SStructType(name, type, structure, templateArgs, STRUCT | CLASS) {}

public:
	void addFunction(const string& name, const SFunction& func);

	SFunction getConstructor();

	SFunction getDestructor();
};

class SUnionType : public STemplatedType
{
	friend class TypeManager;

	map<string, SType*> items;

	SUnionType(const string& name, StructType* type, const vector<pair<string, SType*>>& structure, uint64_t size, const vector<SType*>& templateArgs)
	: STemplatedType(name, UNION | (structure.size()? 0 : OPAQUE), type, size, templateArgs)
	{
		for (auto var : structure)
			items[var.first] = var.second;
	}

	SType* copy() override
	{
		return new SUnionType(*this);
	}

	void setConst(TypeManager* tmang) override;

public:
	SType* getItem(const string& name)
	{
		auto iter = items.find(name);
		return iter != items.end()? iter->second : nullptr;
	}

	string str(CodeContext* context = nullptr) const override;
};

class SEnumType : public SUserType
{
	friend class TypeManager;

	map<string, APSInt> items;

	SEnumType(const string& name, SType* type, const vector<pair<string,int64_t>>& data)
	: SUserType(name, ENUM, *type, data.size(), type)
	{
		auto numbits = type->size();
		auto isUnsigned = !type->isUnsigned();
		for (auto item : data) {
			items[item.first] = APSInt(APInt(numbits, item.second), isUnsigned);
		}
	}

	SType* copy() override
	{
		return new SEnumType(*this);
	}

public:
	APSInt* getItem(const string& name)
	{
		auto iter = items.find(name);
		return iter != items.end()? &iter->second : nullptr;
	}

	string str(CodeContext* context = nullptr) const override;
};

class SFunctionType : public SType
{
	friend class TypeManager;

	vector<SType*> params;

	SFunctionType(FunctionType* type, SType* returnTy, const vector<SType*>& params)
	: SType(FUNCTION, type, 0, returnTy), params(params) {}

	SType* copy() override
	{
		return new SFunctionType(*this);
	}

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

	string str(CodeContext* context = nullptr) const override
	{
		string s;
		raw_string_ostream os(s);

		os << "(";
		for (size_t i = 0; i < params.size(); i++) {
			if (i != 0)
				os << ",";
			os << params[i]->str(context);
		}
		os << ")" << returnTy()->str(context);
		return os.str();
	}

	string raw() override
	{
		string raw = "m";
		for (auto item : params)
			raw += "_" + item->raw();
		raw += "_" + returnTy()->raw();
		return raw;
	}
};

class TypeManager
{
	using STypePtr = unique_ptr<SType>;
	using SFuncPtr = unique_ptr<SFunctionType>;
	using SUserPtr = unique_ptr<SUserType>;
	using STempPtr = unique_ptr<NTemplatedDeclaration>;

	DataLayout datalayout;

	// built-in types
	STypePtr autoTy, voidTy, boolTy, int8Ty, int16Ty, int32Ty, int64Ty, floatTy, doubleTy,
		uint8Ty, uint16Ty, uint32Ty, uint64Ty;

	// const types
	map<SType*, STypePtr> constMap;

	// mutable type
	map<SType*, SType*> mutMap;

	// array & vec types
	map<pair<SType*, uint64_t>, STypePtr> arrMap;
	map<pair<SType*, uint64_t>, STypePtr> vecMap;

	// pointer types
	map<SType*, STypePtr> ptrMap;

	// user types
	map<string, SUserPtr> usrMap;

	// function types
	map<pair<SType*, vector<SType*> >, SFuncPtr> funcMap;

	// template types
	map<string, STempPtr> templateMap;

	StructType* buildStruct(const string& name, const vector<pair<string, SType*>>& structure);

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

	SType* getConst(SType* type)
	{
		if (!type || type->isConst())
			return type;
		STypePtr &item = constMap[type];
		if (!item.get()) {
			item = unique_ptr<SType>(type->copy());
			auto ctype = item.get();
			ctype->setConst(this);
			mutMap[ctype] = type;
		}
		return item.get();
	}

	SType* getMutable(SType* type)
	{
		if (!type || !type->isConst())
			return type;
		return mutMap[type];
	}

	SType* getArray(SType* arrType, int64_t size);

	SType* getVec(SType* vecType, int64_t size);

	SType* getPointer(SType* ptrType);

	SFunctionType* getFunction(SType* returnTy, vector<SType*> args);

	SUserType* lookupUserType(const string& name)
	{
		return usrMap[name].get();
	}

	void storeTemplate(const string& name, NTemplatedDeclaration* decl)
	{
		templateMap[name] = STempPtr(decl->copy());
	}

	NTemplatedDeclaration* getTemplateType(const string& name)
	{
		return templateMap[name].get();
	}

	void createAlias(const string& name, SType* type);

	void createStruct(const string& name, const string& rawName, const vector<pair<string, SType*>>& structure, const vector<SType*>& templateArgs);

	SClassType* createClass(const string& name, const string& rawName, const vector<pair<string, SType*>>& structure, const vector<SType*>& templateArgs);

	void createUnion(const string& name, const string& rawName, const vector<pair<string, SType*>>& structure, const vector<SType*>& templateArgs);

	void createEnum(const string& name, const vector<pair<string,int64_t>>& structure, SType* type);
};

#endif
