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
#include "CodeContext.h"

#define smart_stype(tclass, type, size, subtype) unique_ptr<SType>(new SType(tclass, type, size, subtype))
#define smart_sfuncTy(func, rtype, args) unique_ptr<SFunctionType>(new SFunctionType(func, rtype, args))
#define smart_strucTy(type, structure) unique_ptr<SUserType>(new SStructType(type, structure))
#define smart_unionTy(type, structure, size) unique_ptr<SUserType>(new SUnionType(type, structure, size))

SType* SType::getVoid(CodeContext& context)
{
	return context.typeManager.getVoid();
}

SType* SType::getBool(CodeContext& context)
{
	return context.typeManager.getBool();
}

SType* SType::getInt(CodeContext& context, int bitWidth, bool isUnsigned)
{
	return context.typeManager.getInt(bitWidth, isUnsigned);
}

SType* SType::getFloat(CodeContext& context, bool doubleType)
{
	return context.typeManager.getFloat(doubleType);
}

SType* SType::getArray(CodeContext& context, SType* arrType, uint64_t size)
{
	return context.typeManager.getArray(arrType, size);
}

SType* SType::getVec(CodeContext& context, SType* vecType, uint64_t size)
{
	return context.typeManager.getVec(vecType, size);
}

SType* SType::getPointer(CodeContext& context, SType* ptrType)
{
	return context.typeManager.getPointer(ptrType);
}

SFunctionType* SType::getFunction(CodeContext& context, SType* returnTy, vector<SType*> params)
{
	return context.typeManager.getFunction(returnTy, params);
}

uint64_t SType::allocSize(CodeContext& context, SType* type)
{
	return context.typeManager.allocSize(type);
}

SType* SType::numericConv(CodeContext& context, SType* ltype, SType* rtype, bool int32min)
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
			context.addError("can not cast vec types of different sizes");
			return ltype;
		}
		auto subType = numericConv(context, ltype->subType(), rtype->subType(), false);
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

SUserType* SUserType::lookup(CodeContext& context, string* name)
{
	return context.typeManager.lookupUserType(name);
}

void SUserType::createStruct(CodeContext& context, string* name, const vector<pair<string, SType*>>& structure)
{
	context.typeManager.createStruct(name, structure);
}

void SUserType::createUnion(CodeContext& context, string* name, const vector<pair<string, SType*>>& structure)
{
	context.typeManager.createUnion(name, structure);
}

TypeManager::TypeManager(Module* module)
: datalayout(module)
{
	auto &context = module->getContext();
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
	if (!item.get())
		item = smart_stype(SType::POINTER | SType::UNSIGNED, PointerType::getUnqual(*ptrType), 0, ptrType);
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

void TypeManager::createStruct(string* name, vector<pair<string, SType*>> structure)
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

void TypeManager::createUnion(string* name, vector<pair<string, SType*>> structure)
{
	SUserPtr& item = usrMap[*name];
	if (item.get() || !structure.size())
		return;
	auto type = structure[0].second;
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
	item = smart_unionTy(StructType::create(elements, *name), structure, size);
}
