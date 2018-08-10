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
#include "CodeContext.h"

#define smart_stype(tclass, type, size, subtype) unique_ptr<SType>(new SType(tclass, type, size, subtype))
#define smart_sfuncTy(func, rtype, args) unique_ptr<SFunctionType>(new SFunctionType(func, rtype, args))
#define smart_aliasTy(name, type) unique_ptr<SAliasType>(new SAliasType(name, type))
#define smart_strucTy(name, type, structure) unique_ptr<SUserType>(new SStructType(name, type, structure))
#define smart_classTy(name, type, structure) unique_ptr<SUserType>(new SClassType(name, type, structure))
#define smart_unionTy(name, type, structure, size) unique_ptr<SUserType>(new SUnionType(name, type, structure, size))
#define smart_enumTy(name, type, structure) unique_ptr<SUserType>(new SEnumType(name, type, structure))
#define smart_opaqueTy(type) unique_ptr<SOpaqueType>(new SOpaqueType(type))

void SType::dump() const
{
	dbgs() << str(nullptr) << '\n';
}

void SType::setConst(TypeManager* tmang)
{
	tclass |= CONST;
	if (isSequence())
		subtype = tmang->getConst(subtype);
}

void SAliasType::setConst(TypeManager* tmang)
{
	tclass |= CONST;
	subtype = tmang->getConst(subtype);
}

void SStructType::setConst(TypeManager* tmang)
{
	tclass |= CONST;
	for (auto& item : items) {
		auto rval = item.second.second;
		auto type = tmang->getConst(rval.stype());
		item.second.second = RValue(rval.value(), type);
	}
}

void SUnionType::setConst(TypeManager* tmang)
{
	tclass |= CONST;
	for (auto& item : items)
		item.second = tmang->getConst(item.second);
}

SType* SType::getAuto(CodeContext& context)
{
	return context.getTypeManager().getAuto();
}

SType* SType::getConst(CodeContext& context, SType* type)
{
	return context.getTypeManager().getConst(type);
}

SType* SType::getMutable(CodeContext& context, SType* type)
{
	return context.getTypeManager().getMutable(type);
}

SType* SType::getVoid(CodeContext& context)
{
	return context.getTypeManager().getVoid();
}

SType* SType::getBool(CodeContext& context)
{
	return context.getTypeManager().getBool();
}

SType* SType::getInt(CodeContext& context, int bitWidth, bool isUnsigned)
{
	return context.getTypeManager().getInt(bitWidth, isUnsigned);
}

SType* SType::getFloat(CodeContext& context, bool doubleType)
{
	return context.getTypeManager().getFloat(doubleType);
}

SType* SType::getArray(CodeContext& context, SType* arrType, uint64_t size)
{
	return context.getTypeManager().getArray(arrType, size);
}

SType* SType::getVec(CodeContext& context, SType* vecType, uint64_t size)
{
	return context.getTypeManager().getVec(vecType, size);
}

SType* SType::getPointer(CodeContext& context, SType* ptrType)
{
	return context.getTypeManager().getPointer(ptrType);
}

SFunctionType* SType::getFunction(CodeContext& context, SType* returnTy, vector<SType*> params)
{
	return context.getTypeManager().getFunction(returnTy, params);
}

uint64_t SType::allocSize(CodeContext& context, SType* type)
{
	return context.getTypeManager().allocSize(type);
}

SType* SType::numericConv(CodeContext& context, Token* optToken, SType* ltype, SType* rtype, bool int32min)
{
	switch (ltype->isVec() | (rtype->isVec() << 1)) {
	default:
		// should never happen
		return ltype;
	case 0:
		goto novec;
	case 1:
		return ltype;
	case 2:
		return rtype;
	case 3: // both vector
		if (ltype->size() != rtype->size()) {
			context.addError("can not cast vec types of different sizes", optToken);
			return ltype;
		}
		auto subType = numericConv(context, optToken, ltype->subType(), rtype->subType(), false);
		return SType::getVec(context, subType, ltype->size());
	}
novec:
	auto btype = ltype->tclass | rtype->tclass;
	if (btype & FLOATING)
		return SType::getFloat(context, btype & DOUBLE);

	auto lbits = ltype->size() - !ltype->isUnsigned();
	auto rbits = rtype->size() - !rtype->isUnsigned();
	if (lbits > rbits)
		return (int32min && lbits < 31)? SType::getInt(context, 32) : ltype;
	else
		return (int32min && rbits < 31)? SType::getInt(context, 32) : rtype;
}

bool SType::validate(CodeContext& context, Token* token, SType* type)
{
	if (!type->isArray())
		return true;

	if (!type->size()) {
		context.addError("can't create a non-pointer to a zero size array", token);
		return false;
	}
	return validate(context, token, type->subType());
}

void SUserType::innerStr(CodeContext* context, stringstream& os) const
{
	if (isConst())
		os << "const ";
	os << name;
}

SStructType::SStructType(const string& name, StructType* type, const vector<pair<string, SType*>>& structure, int ctype)
: SUserType(name, ctype | (structure.size()? 0 : OPAQUE), type, structure.size())
{
	int i = 0;
	for (auto var : structure)
		items[var.first] = make_pair(i++, RValue(nullptr, var.second));
}

pair<int, RValue>* SStructType::getItem(const string& name)
{
	auto iter = items.find(name);
	return iter != items.end()? &iter->second : nullptr;
}

string SStructType::str(CodeContext* context) const
{
	stringstream os;

	if (context) {
		innerStr(context, os);
	} else {
		os << "S:{|";
		for (auto i : items) {
			os << i.first << "=" << i.second.first << ":"
				<< i.second.second.stype()->str(context) << "|";
		}
		os << "}";
	}
	return os.str();
}

void SClassType::addFunction(const string& name, const SFunction& func)
{
	items[name] = make_pair(0, func);
}

string SUnionType::str(CodeContext* context) const
{
	stringstream os;

	if (context) {
		innerStr(context, os);
	} else {
		os << "U:{|";
		for (auto i : items) {
			os << i.first << "=" << i.second->str(context) << "|";
		}
		os << "}";
	}
	return os.str();
}

string SEnumType::str(CodeContext* context) const
{
	stringstream os;

	if (context) {
		innerStr(context, os);
	} else {
		os << "E:{|";
		for (auto i : items) {
			os << i.first << "=" << i.second.getSExtValue() << "|";
		}
		os << "}";
	}
	return os.str();
}

SUserType* SUserType::lookup(CodeContext& context, const string& name)
{
	return context.getTypeManager().lookupUserType(name);
}

void SUserType::createAlias(CodeContext& context, const string& name, SType* type)
{
	context.getTypeManager().createAlias(name, type);
}

void SUserType::createStruct(CodeContext& context, const string& name, const vector<pair<string, SType*>>& structure)
{
	context.getTypeManager().createStruct(name, structure);
}

SClassType* SUserType::createClass(CodeContext& context, const string& name, const vector<pair<string, SType*>>& structure)
{
	return context.getTypeManager().createClass(name, structure);
}

void SUserType::createUnion(CodeContext& context, const string& name, const vector<pair<string, SType*>>& structure)
{
	context.getTypeManager().createUnion(name, structure);
}

void SUserType::createEnum(CodeContext& context, const string& name, const vector<pair<string, int64_t>>& structure, SType* type)
{
	context.getTypeManager().createEnum(name, structure, type);
}

TypeManager::TypeManager(Module* module)
: datalayout(module)
{
	auto &context = module->getContext();
	autoTy = smart_stype(SType::AUTO, Type::getInt32Ty(context), 0, nullptr);
	voidTy = smart_stype(SType::VOID, Type::getVoidTy(context), 0, nullptr);
	boolTy = smart_stype(SType::INTEGER | SType::UNSIGNED, Type::getInt1Ty(context), 1, nullptr);
	int8Ty = smart_stype(SType::INTEGER, Type::getInt8Ty(context), 8, nullptr);
	int16Ty = smart_stype(SType::INTEGER, Type::getInt16Ty(context), 16, nullptr);
	int32Ty = smart_stype(SType::INTEGER, Type::getInt32Ty(context), 32, nullptr);
	int64Ty = smart_stype(SType::INTEGER, Type::getInt64Ty(context), 64, nullptr);
	uint8Ty = smart_stype(SType::INTEGER | SType::UNSIGNED, Type::getInt8Ty(context), 8, nullptr);
	uint16Ty = smart_stype(SType::INTEGER | SType::UNSIGNED, Type::getInt16Ty(context), 16, nullptr);
	uint32Ty = smart_stype(SType::INTEGER | SType::UNSIGNED, Type::getInt32Ty(context), 32, nullptr);
	uint64Ty = smart_stype(SType::INTEGER | SType::UNSIGNED, Type::getInt64Ty(context), 64, nullptr);
	floatTy = smart_stype(SType::FLOATING, Type::getFloatTy(context), 0, nullptr);
	doubleTy = smart_stype(SType::FLOATING | SType::DOUBLE, Type::getDoubleTy(context), 0, nullptr);
}

SType* TypeManager::getArray(SType* arrType, int64_t size)
{
	STypePtr &item = arrMap[make_pair(arrType, size)];
	if (!item.get())
		item = smart_stype(SType::ARRAY, ArrayType::get(*arrType, size), size, arrType);
	return item.get();
}

SType* TypeManager::getVec(SType* vecType, int64_t size)
{
	STypePtr &item = arrMap[make_pair(vecType, size)];
	if (!item.get())
		item = smart_stype(SType::VEC, VectorType::get(*vecType, size), size, vecType);
	return item.get();
}

SType* TypeManager::getPointer(SType* ptrType)
{
	STypePtr &item = ptrMap[ptrType];
	if (!item.get()) {
		// pointer to void must be i8*
		auto llptr = PointerType::getUnqual(*(ptrType->isVoid()? int8Ty.get() : ptrType));
		item = smart_stype(SType::POINTER | SType::UNSIGNED, llptr, 0, ptrType);
	}
	return item.get();
}

SFunctionType* TypeManager::getFunction(SType* returnTy, vector<SType*> args)
{
	SFuncPtr &item = funcMap[make_pair(returnTy, args)];
	if (!item.get()) {
		auto func = FunctionType::get(*returnTy, SType::convertArr(args), false);
		item = smart_sfuncTy(func, returnTy, args);
	}
	return item.get();
}

void TypeManager::createAlias(const string& name, SType* type)
{
	SUserPtr& item = usrMap[name];
	if (item.get())
		return;
	item = smart_aliasTy(name, type);
}

StructType* TypeManager::buildStruct(const string& name, const vector<pair<string, SType*>>& structure)
{
	vector<Type*> elements;
	if (structure.empty())
		elements.push_back(*int8Ty.get());
	for (auto item : structure)
		elements.push_back(*item.second);
	return StructType::create(elements, name);
}

void TypeManager::createStruct(const string& name, const vector<pair<string, SType*>>& structure)
{
	SUserPtr& item = usrMap[name];
	if (item.get())
		return;
	item = smart_strucTy(name, buildStruct(name, structure), structure);
}

SClassType* TypeManager::createClass(const string& name, const vector<pair<string, SType*>>& structure)
{
	SUserPtr& item = usrMap[name];
	if (!item.get())
		item = smart_classTy(name, buildStruct(name, structure), structure);
	return static_cast<SClassType*>(item.get());
}

void TypeManager::createUnion(const string& name, const vector<pair<string, SType*>>& structure)
{
	SUserPtr& item = usrMap[name];
	if (item.get())
		return;
	auto type = structure.size()? structure[0].second : int8Ty.get();
	auto size = allocSize(type);
	for (auto item : structure) {
		auto tsize = allocSize(item.second);
		if (tsize > size) {
			size = tsize;
			type = item.second;
		}
	}
	vector<Type*> elements;
	elements.push_back(*type);
	item = smart_unionTy(name, StructType::create(elements, name), structure, size);
}

void TypeManager::createEnum(const string& name, const vector<pair<string, int64_t>>& structure, SType* type)
{
	SUserPtr& item = usrMap[name];
	if (item.get() || !structure.size())
		return;
	item = smart_enumTy(name, type, structure);
}
