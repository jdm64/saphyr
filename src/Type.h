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
class SType;
class STemplatedType;
class SStructType;
class SUnionType;
class SClassType;
class SFunctionType;
class TypeManager;
class RValue;
class SFunction;

using namespace std;
using namespace llvm;

using VecSType = vector<SType*>;

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

	static vector<Type*> convertArr(VecSType arr)
	{
		vector<Type*> vec;
		transform(arr.begin(), arr.end(), back_inserter(vec), [](auto i){ return *i; });
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

	/**
	 * returns true if the types differ only by const-ness
	 */
	static bool isConstEQ(CodeContext& context, SType* lhs, SType* rhs);

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

	static SFunctionType* getFunction(CodeContext& context, SType* returnTy, VecSType params);

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

	bool isConstructable();

	bool isDestructable();

	SType* subType() const
	{
		return subtype;
	}

	SType* getScalar()
	{
		return isSequence()? subtype->getScalar() : this;
	}

	virtual string str(const CodeContext& context) const
	{
		string s;
		raw_string_ostream os(s);

		if (isConst())
			os << "const ";

		if (isArray()) {
			os << "[";
			auto sz = size();
			if (sz)
				os << sz;
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
			auto sz = size();
			if (sz)
				os << sz;
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

protected:
	string name;

	SUserType(const string& name, int typeClass, Type* type, uint64_t size = 0, SType* subType = nullptr)
	: SType(typeClass, type, size, subType), name(name) {}

	void innerStr(stringstream& os) const;

public:
	string raw() override
	{
		return name;
	}

	static string raw(const string& name, const VecSType& templateArgs)
	{
		auto ret = name;
		for (auto item : templateArgs)
			ret += "_" + item->raw();
		return ret;
	}

	static bool isDeclared(CodeContext& context, const string& name, const VecSType& templateArgs);

	static SType* lookup(CodeContext& context, Token* name, VecSType templateArgs, bool& hasErrors);

	static void createAlias(CodeContext& context, const string& name, SType* type);

	static SStructType* createStruct(CodeContext& context, const string& name, const VecSType& templateArgs);

	static SClassType* createClass(CodeContext& context, const string& name, const VecSType& templateArgs);

	static SUnionType* createUnion(CodeContext& context, const string& name, const VecSType& templateArgs);

	static void setBody(CodeContext& context, STemplatedType* type, const vector<pair<string, SType*>>& structure);

	static void createEnum(CodeContext& context, const string& name, const vector<pair<string, int64_t>>& structure, SType* type);
};

class SAliasType : public SUserType
{
	friend class TypeManager;

	explicit SAliasType(const string& aName, SType* type)
	: SUserType(aName, ALIAS, type->type(), 0, type) {}

protected:
	void setConst(TypeManager* tmang) override;

public:
	string str(const CodeContext& context) const override
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
	VecSType templateArgs;

	STemplatedType(const string& tName, int typeClass, const VecSType& templateArgs)
	: SUserType(tName, typeClass | OPAQUE | (templateArgs.size() ? TEMPLATED : 0), nullptr, 0), templateArgs(templateArgs) {}

public:
	string raw() override
	{
		auto raw = name;
		for (auto item : templateArgs)
			raw += "_" + item->raw();
		return raw;
	}

	VecSType getTemplateArgs() const
	{
		return templateArgs;
	}
};

class SStructType : public STemplatedType
{
	friend class TypeManager;
	friend class SClassType;

	using container = map<string, vector<pair<int, RValue>>>;
	using const_iterator = container::const_iterator;

protected:
	container items;

	SStructType(const string& sName, const VecSType& args, int ctype = STRUCT)
	: STemplatedType(sName, ctype, args) {}

	SType* copy() override
	{
		return new SStructType(*this);
	}

	void setConst(TypeManager* tmang) override;

public:
	vector<pair<int, RValue>>* getItem(const string& itemName);

	bool hasItem(const string& itemName, RValue& item);

	string str(const CodeContext& context) const override;

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

	SClassType(const string& cName, const VecSType& args)
	: SStructType(cName, args, STRUCT | CLASS) {}

public:
	void addFunction(const string& fName, const SFunction& func);

	vector<SFunction> getConstructor();

	SFunction getDestructor();
};

class SUnionType : public STemplatedType
{
	friend class TypeManager;

	map<string, SType*> items;

	SUnionType(const string& uName, const VecSType& args)
	: STemplatedType(uName, UNION, args) {}

	SType* copy() override
	{
		return new SUnionType(*this);
	}

	void setConst(TypeManager* tmang) override;

public:
	SType* getItem(const string& itemName)
	{
		auto iter = items.find(itemName);
		return iter != items.end()? iter->second : nullptr;
	}

	string str(const CodeContext& context) const override;
};

class SEnumType : public SUserType
{
	friend class TypeManager;

	map<string, APSInt> items;

	SEnumType(const string& eName, SType* type, const vector<pair<string,int64_t>>& data)
	: SUserType(eName, ENUM, *type, data.size(), type)
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
	APSInt* getItem(const string& itemName)
	{
		auto iter = items.find(itemName);
		return iter != items.end()? &iter->second : nullptr;
	}

	string str(const CodeContext& context) const override;
};

class SFunctionType : public SType
{
	friend class TypeManager;

	VecSType params;

	SFunctionType(FunctionType* type, SType* returnTy, const VecSType& params)
	: SType(FUNCTION, type, 0, returnTy), params(params) {}

	SType* copy() override
	{
		return new SFunctionType(*this);
	}

public:
	using ParamIter = VecSType::iterator;

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

	size_t numParams() const
	{
		return params.size();
	}

	SType* getParam(int index) const
	{
		return params[index];
	}

	string str(const CodeContext& context) const override
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
	LLVMContext& context;

	// built-in types
	STypePtr autoTy;
	STypePtr voidTy;
	STypePtr boolTy;
	STypePtr int8Ty;
	STypePtr int16Ty;
	STypePtr int32Ty;
	STypePtr int64Ty;
	STypePtr floatTy;
	STypePtr doubleTy;
	STypePtr uint8Ty;
	STypePtr uint16Ty;
	STypePtr uint32Ty;
	STypePtr uint64Ty;

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
	map<pair<SType*, VecSType >, SFuncPtr> funcMap;

	// template types
	map<string, STempPtr> templateMap;

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

	SFunctionType* getFunction(SType* returnTy, VecSType args);

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

	void setBody(STemplatedType* type, const vector<pair<string, SType*>>& structure);

	SStructType* createStruct(const string& name, const string& rawName, const VecSType& templateArgs);

	SClassType* createClass(const string& name, const string& rawName, const VecSType& templateArgs);

	SUnionType* createUnion(const string& name, const string& rawName, const VecSType& templateArgs);

	void createEnum(const string& name, const vector<pair<string,int64_t>>& structure, SType* type);
};

#endif
