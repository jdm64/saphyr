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
#include <regex>
#include "CodeContext.h"
#include "CGNStatement.h"

#define uPtrSType(tclass, type, size, subtype) uPtr<SType>(new SType((tclass), (type), (size), (subtype)))
#define uPtrSFunc(func, rtype, args) uPtr<SFunctionType>(new SFunctionType((func), (rtype), (args)))
#define uPtrSAlias(name, type) uPtr<SAliasType>(new SAliasType((name), (type)))
#define uPtrSStruct(name, args) uPtr<SUserType>(new SStructType((name), (args)))
#define uPtrSClass(name, args) uPtr<SUserType>(new SClassType((name), (args)))
#define uPtrSUnion(name, args) uPtr<SUserType>(new SUnionType((name), (args)))
#define uPtrSEnum(name, type, structure) uPtr<SUserType>(new SEnumType((name), (type), (structure)))

bool SType::isConstructable()
{
	SClassType* clType = nullptr;
	if (isClass())
		clType = static_cast<SClassType*>(this);
	else if (isArray() && subType()->isClass())
		clType = static_cast<SClassType*>(subType());

	return clType ? !clType->getConstructor().empty() : false;
}

bool SType::isDestructable()
{
	SClassType* clType = nullptr;
	if (isClass())
		clType = static_cast<SClassType*>(this);
	else if (isArray() && subType()->isClass())
		clType = static_cast<SClassType*>(subType());

	return clType ? clType->getDestructor() : false;
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
		for (auto& pair : item.second) {
			auto rval = pair.second;
			auto type = tmang->getConst(rval.stype());
			pair.second = RValue(rval.value(), type);
		}

	}
}

void SUnionType::setConst(TypeManager* tmang)
{
	tclass |= CONST;
	for (auto& item : items)
		item.second = tmang->getConst(item.second);
}

SType* SType::getSuffixType(CodeContext& context, const string& name)
{
	return context.getTypeManager().getSuffixType(name);
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

SType * SType::getReference(CodeContext& context, SType* type)
{
	return context.getTypeManager().getReference(type);
}

SType * SType::getCopyRef(CodeContext& context, SType* type)
{
	return context.getTypeManager().getCopyRef(type);
}

SFunctionType* SType::getFunction(CodeContext& context, SType* returnTy, VecSType params)
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
	case 0:
		goto novec;
	case 2:
		return rtype;
	default:
		// case 1
		return ltype;
	case 3:
		// both vector
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

bool SType::isConstEQ(CodeContext& context, SType* lhs, SType* rhs)
{
	auto mask = POINTER | ARRAY | VEC;
	if (mask & lhs->tclass & rhs->tclass) {
		return lhs->size() == rhs->size() && isConstEQ(context, lhs->subType(), rhs->subType());
	}
	return getMutable(context, lhs) == getMutable(context, rhs);
}

void SUserType::innerStr(stringstream& os) const
{
	if (isConst())
		os << "const ";
	os << name;
}

vector<pair<int, RValue>>* SStructType::getItem(const string& itemName)
{
	auto iter = items.find(itemName);
	return iter != items.end()? &iter->second : nullptr;
}

bool SStructType::hasItem(const string& itemName, RValue& item)
{
	auto list = getItem(itemName);
	if (!list)
		return false;
	return any_of(list->begin(), list->end(), [&](auto i){ return item.value() == i.second.value() && item.type() == i.second.type(); });
}

string SStructType::str(const CodeContext& context) const
{
	stringstream os;

	innerStr(os);
	if (templateArgs.size()) {
		os << "<";
		bool first = true;
		for (auto item : templateArgs) {
			if (first)
				first = false;
			else
				os << ",";
			os << item->str(context);
		}
		os << ">";
	}
	return os.str();
}

void SClassType::addFunction(const string& fName, const SFunction& func)
{
	items[fName].push_back(make_pair(0, func));
}

VecSFunc SClassType::getConstructor()
{
	auto item = getItem("this");
	if (!item)
		return {};
	VecSFunc ret;
	transform(item->begin(), item->end(), back_inserter(ret), [](auto p){ return static_cast<SFunction&>(p.second); });
	return ret;
}

SFunction SClassType::getDestructor()
{
	auto item = getItem("null");
	if (!item)
		return {};
	return static_cast<SFunction&>((*item)[0].second);
}

string SUnionType::str(const CodeContext& context) const
{
	stringstream os;
	innerStr(os);
	return os.str();
}

string SEnumType::str(const CodeContext& context) const
{
	stringstream os;
	innerStr(os);
	return os.str();
}

bool SUserType::isDeclared(CodeContext& context, const string& name, const VecSType& templateArgs)
{
	auto inTemplate = context.inTemplate();
	auto hasArgs = !templateArgs.empty();
	if (inTemplate && !hasArgs) {
		auto type = context.getTemplateArg(name);
		if (type)
			return type;
	}

	auto rawName = SUserType::raw(name, templateArgs);
	return context.getTypeManager().lookupUserType(rawName);
}

SType* SUserType::lookup(CodeContext& context, Token* name, VecSType templateArgs, bool& hasErrors)
{
	auto nameStr = name->str;
	bool inTemplate = context.inTemplate();
	auto noArgs = templateArgs.empty();
	if (inTemplate && noArgs) {
		auto type = context.getTemplateArg(nameStr);
		if (type)
			return type;
	}

	auto& tm = context.getTypeManager();
	auto type = tm.lookupUserType(nameStr);
	if (type) {
		if (!type->isTemplated() && !noArgs) {
			hasErrors = true;
			context.addError(nameStr + " type is not a template", name);
			return nullptr;
		}
		return type;
	} else if (noArgs) {
		if (tm.getTemplateType(nameStr)) {
			hasErrors = true;
			context.addError(nameStr + " type requires template arguments", name);
		}
		return nullptr;
	}

	auto rawName = SUserType::raw(nameStr, templateArgs);
	type = tm.lookupUserType(rawName);
	if (type)
		return type;

	auto templateType = tm.getTemplateType(nameStr);
	if (!templateType)
		return nullptr;

	auto params = templateType->getTemplateParams();
	if (params->size() != templateArgs.size()) {
		hasErrors = true;
		context.addError("number of template args doesn't match for " + nameStr, name);
		return nullptr;
	}

	vector<pair<string, SType*>> templateMappings;
	for (size_t i = 0; i < templateArgs.size(); i++)
		templateMappings.push_back({params->at(i)->str, templateArgs[i]});

	auto errorCount = context.errorCount();
	auto templateCtx = CodeContext::newForTemplate(context, templateMappings);
	auto templatePtr = uPtr<NTemplatedDeclaration>(templateType->copy());
	CGNStatement::run(templateCtx, templatePtr.get());

	type = context.getTypeManager().lookupUserType(rawName);
	if (context.errorCount() > errorCount) {
		context.addError("errors when creating type: " + type->str(context), name);
	}
	return type;
}

void SUserType::createAlias(CodeContext& context, const string& name, SType* type)
{
	context.getTypeManager().createAlias(name, type);
}

SStructType* SUserType::createStruct(CodeContext& context, const string& name, const VecSType& templateArgs)
{
	auto rawName = SUserType::raw(name, templateArgs);
	return context.getTypeManager().createStruct(name, rawName, templateArgs);
}

SClassType* SUserType::createClass(CodeContext& context, const string& name, const VecSType& templateArgs)
{
	auto rawName = SUserType::raw(name, templateArgs);
	return context.getTypeManager().createClass(name, rawName, templateArgs);
}

SUnionType* SUserType::createUnion(CodeContext& context, const string& name, const VecSType& templateArgs)
{
	auto rawName = SUserType::raw(name, templateArgs);
	return context.getTypeManager().createUnion(name, rawName, templateArgs);
}

void SUserType::setBody(CodeContext& context, STemplatedType* type, const vector<pair<string, SType*>>& structure)
{
	context.getTypeManager().setBody(type, structure);
}

void SUserType::createEnum(CodeContext& context, const string& name, const vector<pair<string, int64_t>>& structure, SType* type)
{
	context.getTypeManager().createEnum(name, structure, type);
}

TypeManager::TypeManager(Module* module)
: datalayout(module), context(module->getContext())
{
	autoTy = uPtrSType(SType::AUTO, Type::getInt32Ty(context), 0, nullptr);
	voidTy = uPtrSType(SType::VOID, Type::getVoidTy(context), 0, nullptr);
	boolTy = uPtrSType(SType::INTEGER | SType::UNSIGNED, Type::getInt1Ty(context), 1, nullptr);
	int8Ty = uPtrSType(SType::INTEGER, Type::getInt8Ty(context), 8, nullptr);
	int16Ty = uPtrSType(SType::INTEGER, Type::getInt16Ty(context), 16, nullptr);
	int32Ty = uPtrSType(SType::INTEGER, Type::getInt32Ty(context), 32, nullptr);
	int64Ty = uPtrSType(SType::INTEGER, Type::getInt64Ty(context), 64, nullptr);
	uint8Ty = uPtrSType(SType::INTEGER | SType::UNSIGNED, Type::getInt8Ty(context), 8, nullptr);
	uint16Ty = uPtrSType(SType::INTEGER | SType::UNSIGNED, Type::getInt16Ty(context), 16, nullptr);
	uint32Ty = uPtrSType(SType::INTEGER | SType::UNSIGNED, Type::getInt32Ty(context), 32, nullptr);
	uint64Ty = uPtrSType(SType::INTEGER | SType::UNSIGNED, Type::getInt64Ty(context), 64, nullptr);
	floatTy = uPtrSType(SType::FLOATING, Type::getFloatTy(context), 0, nullptr);
	doubleTy = uPtrSType(SType::FLOATING | SType::DOUBLE, Type::getDoubleTy(context), 0, nullptr);

	suffix = {
		{"f", floatTy.get()},
		{"d", doubleTy.get()},
		{"i8", int8Ty.get()},
		{"u8", uint8Ty.get()},
		{"i16", int16Ty.get()},
		{"u16", uint16Ty.get()},
		{"i32", int32Ty.get()},
		{"u32", uint32Ty.get()},
		{"i64", int64Ty.get()},
		{"u64", uint64Ty.get()}
	};
}

SType* TypeManager::getArray(SType* arrType, int64_t size)
{
	STypePtr &item = arrMap[make_pair(arrType, size)];
	if (!item.get())
		item = uPtrSType(SType::ARRAY, ArrayType::get(*arrType, size), size, arrType);
	return item.get();
}

SType* TypeManager::getVec(SType* vecType, int64_t size)
{
	STypePtr &item = arrMap[make_pair(vecType, size)];
	if (!item.get())
#if LLVM_VERSION_MAJOR >= 11
		item = uPtrSType(SType::VEC, FixedVectorType::get(*vecType, size), size, vecType);
#else
		item = uPtrSType(SType::VEC, VectorType::get(*vecType, size), size, vecType);
#endif
	return item.get();
}

SType* TypeManager::getPointer(SType* ptrType)
{
	STypePtr &item = ptrMap[ptrType];
	if (!item.get()) {
		// pointer to void must be i8*
		auto llptr = PointerType::getUnqual(*(ptrType->isVoid()? int8Ty.get() : ptrType));
		item = uPtrSType(SType::POINTER | SType::UNSIGNED, llptr, 0, ptrType);
	}
	return item.get();
}

SType* TypeManager::getReference(SType* type)
{
	STypePtr &item = refMap[type];
	if (!item.get()) {
		auto llptr = PointerType::getUnqual(*type);
		item = uPtrSType(SType::REFERENCE | SType::UNSIGNED, llptr, 0, type);
	}
	return item.get();
}

SType* TypeManager::getCopyRef(SType* type)
{
	STypePtr &item = cpyRefMap[type];
	if (!item.get()) {
		auto llptr = PointerType::getUnqual(*type);
		item = uPtrSType(SType::REFERENCE | SType::COPY_REF | SType::UNSIGNED, llptr, 0, type);
	}
	return item.get();
}

SFunctionType* TypeManager::getFunction(SType* returnTy, VecSType args)
{
	SFuncPtr &item = funcMap[make_pair(returnTy, args)];
	if (!item.get()) {
		auto func = FunctionType::get(*returnTy, SType::convertArr(args), false);
		item = uPtrSFunc(func, returnTy, args);
	}
	return item.get();
}

void TypeManager::createAlias(const string& name, SType* type)
{
	SUserPtr& item = usrMap[name];
	if (item.get())
		return;
	item = uPtrSAlias(name, type);
}

void TypeManager::setBody(STemplatedType* type, const vector<pair<string,SType*>>& structure)
{
	if (structure.size())
		type->tclass &= ~SType::OPAQUE;
	else
		return;

	vector<Type*> elements;
	if (type->isStruct()) {
		auto sTy = static_cast<SStructType*>(type);
		int i = 0;
		for (auto item : structure) {
			sTy->items[item.first].push_back(make_pair(i++, RValue(nullptr, item.second)));
			elements.push_back(*item.second);
		}
	} else if (type->isUnion()) {
		auto uTy = static_cast<SUnionType*>(type);
		const SType* rawType = int8Ty.get();
		uint64_t size = 0;
		for (auto item : structure) {
			auto tsize = allocSize(item.second);
			if (tsize > size) {
				size = tsize;
				rawType = item.second;
			}
			uTy->items[item.first] = item.second;
		}
		elements.push_back(*rawType);
	}

	static_cast<StructType*>(type->ltype)->setBody(elements);
}

SStructType* TypeManager::createStruct(const string& name, const string& rawName, const VecSType& templateArgs)
{
	SUserPtr& item = usrMap[rawName];
	if (!item.get()) {
		item = uPtrSStruct(name, templateArgs);
		item.get()->ltype = StructType::create(context, rawName);
	}
	return static_cast<SStructType*>(item.get());
}

SClassType* TypeManager::createClass(const string& name, const string& rawName, const VecSType& templateArgs)
{
	SUserPtr& item = usrMap[rawName];
	if (!item.get()) {
		item = uPtrSClass(name, templateArgs);
		item.get()->ltype = StructType::create(context, rawName);
	}
	return static_cast<SClassType*>(item.get());
}

SUnionType* TypeManager::createUnion(const string& name, const string& rawName, const VecSType& templateArgs)
{
	SUserPtr& item = usrMap[rawName];
	if (!item.get()) {
		item = uPtrSUnion(name, templateArgs);
		item.get()->ltype = StructType::create(context, rawName);
	}
	return static_cast<SUnionType*>(item.get());
}

void TypeManager::createEnum(const string& name, const vector<pair<string, int64_t>>& structure, SType* type)
{
	SUserPtr& item = usrMap[name];
	if (item.get() || !structure.size())
		return;
	item = uPtrSEnum(name, type, structure);
}
