/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2016, Justin Madru (justin.jdm64@gmail.com)
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
#include "Value.h"
#include "AST.h"
#include "CGNDataType.h"
#include "CodeContext.h"
#include "parserbase.h"
#include "Builder.h"
#include "CGNInt.h"

#define TABLE_ADD(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartDataType)] = reinterpret_cast<classPtr>(&CGNDataType::visit##ID)

CGNDataType::classPtr* CGNDataType::buildVTable()
{
	auto table = new CGNDataType::classPtr[NODEID_DIFF(NodeId::EndDataType, NodeId::StartDataType)];
	TABLE_ADD(NArrayType);
	TABLE_ADD(NBaseType);
	TABLE_ADD(NFuncPointerType);
	TABLE_ADD(NPointerType);
	TABLE_ADD(NUserType);
	TABLE_ADD(NVecType);
	return table;
}

CGNDataType::classPtr* CGNDataType::vtable = buildVTable();

SType* CGNDataType::visit(NDataType* type)
{
	return (this->*vtable[NODEID_DIFF(type->id(), NodeId::StartDataType)])(type);
}

SType* CGNDataType::visitNBaseType(NBaseType* type)
{
	switch (type->getType()) {
	case ParserBase::TT_VOID:
		return SType::getVoid(context);
	case ParserBase::TT_BOOL:
		return SType::getBool(context);
	case ParserBase::TT_INT8:
		return SType::getInt(context, 8);
	case ParserBase::TT_INT16:
		return SType::getInt(context, 16);
	case ParserBase::TT_INT:
	case ParserBase::TT_INT32:
		return SType::getInt(context, 32);
	case ParserBase::TT_INT64:
		return SType::getInt(context, 64);
	case ParserBase::TT_UINT8:
		return SType::getInt(context, 8, true);
	case ParserBase::TT_UINT16:
		return SType::getInt(context, 16, true);
	case ParserBase::TT_UINT:
	case ParserBase::TT_UINT32:
		return SType::getInt(context, 32, true);
	case ParserBase::TT_UINT64:
		return SType::getInt(context, 64, true);
	case ParserBase::TT_FLOAT:
		return SType::getFloat(context);
	case ParserBase::TT_DOUBLE:
		return SType::getFloat(context, true);
	case ParserBase::TT_AUTO:
	default:
		return SType::getAuto(context);
	}
}

SType* CGNDataType::visitNArrayType(NArrayType* type)
{
	auto baseType = type->getBaseType();
	auto btype = visit(baseType);
	if (!btype) {
		return nullptr;
	} else if (btype->isAuto()) {
		auto token = static_cast<NNamedType*>(baseType)->getToken();
		context.addError("can't create array of auto types", token);
		return nullptr;
	}
	auto size = type->getSize();
	if (size) {
		auto arrSize = CGNInt::run(context, size).getSExtValue();
		if (arrSize <= 0) {
			context.addError("Array size must be positive", size->getToken());
			return nullptr;
		}
		return SType::getArray(context, btype, arrSize);
	} else {
		return SType::getArray(context, btype, 0);
	}
}

SType* CGNDataType::visitNVecType(NVecType* type)
{
	auto size = type->getSize();
	auto arrSize = CGNInt::run(context, size).getSExtValue();
	if (arrSize <= 0) {
		context.addError("vec size must be greater than 0", size->getToken());
		return nullptr;
	}
	auto btype = visit(type->getBaseType());
	if (!btype) {
		return nullptr;
	} else if (!btype->isNumeric() && !btype->isPointer()) {
		context.addError("vec type only supports numeric and pointer types", type->getToken());
		return nullptr;
	}
	return SType::getVec(context, btype, arrSize);
}


SType* CGNDataType::visitNUserType(NUserType* type)
{
	auto typeName = type->getName();
	auto ty = SUserType::lookup(context, typeName);
	if (!ty) {
		context.addError(typeName + " type not declared", type->getToken());
		return nullptr;
	}
	return ty->isAlias()? ty->subType() : ty;
}

SType* CGNDataType::visitNPointerType(NPointerType* type)
{
	auto baseType = type->getBaseType();
	auto btype = visit(baseType);
	if (!btype) {
		return nullptr;
	} else if (btype->isAuto()) {
		auto token = static_cast<NNamedType*>(baseType)->getToken();
		context.addError("can't create pointer to auto type", token);
		return nullptr;
	}
	return SType::getPointer(context, btype);
}

SType* CGNDataType::visitNFuncPointerType(NFuncPointerType* type)
{
	auto ptr = Builder::getFuncType(context, type->getToken(), type->getReturnType(), type->getParams());
	return ptr? SType::getPointer(context, ptr) : nullptr;
}
