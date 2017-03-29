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
#include "Value.h"
#include "AST.h"
#include "CGNDataType.h"
#include "CodeContext.h"
#include "parserbase.h"
#include "Builder.h"
#include "CGNInt.h"
#include "Instructions.h"
#include "CGNExpression.h"

#define TABLE_ADD(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartDataType)] = reinterpret_cast<classPtr>(&CGNDataType::visit##ID)

CGNDataType::classPtr* CGNDataType::buildVTable()
{
	auto table = new CGNDataType::classPtr[NODEID_DIFF(NodeId::EndDataType, NodeId::StartDataType)];
	TABLE_ADD(NArrayType);
	TABLE_ADD(NBaseType);
	TABLE_ADD(NConstType);
	TABLE_ADD(NFuncPointerType);
	TABLE_ADD(NPointerType);
	TABLE_ADD(NThisType);
	TABLE_ADD(NUserType);
	TABLE_ADD(NVecType);
	return table;
}

CGNDataType::classPtr* CGNDataType::vtable = CGNDataType::buildVTable();

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

SType* CGNDataType::visitNConstType(NConstType* type)
{
	return SType::getConst(context, visit(type->getType()));
}

SType* CGNDataType::visitNThisType(NThisType* type)
{
	auto cl = context.getClass();
	if (!cl) {
		context.addError("use of 'this' is only valid inside of a class", *type);
		return nullptr;
	}
	return cl;
}

SType* CGNDataType::getArrayType(NArrayType* type)
{
	auto baseType = type->getBaseType();
	auto btype = visit(baseType);
	if (!btype) {
		return nullptr;
	} else if (btype->isUnsized()) {
		context.addError("can't create array of " + btype->str(&context) + " types", *baseType);
		return nullptr;
	}
	return btype;
}

SType* CGNDataType::visitNArrayType(NArrayType* type)
{
	auto btype = getArrayType(type);
	if (!btype)
		return nullptr;
	auto size = type->getSize();
	if (size) {
		if (size->isConstant() && static_cast<NConstant*>(size)->isIntConst()) {
			auto arrSize = CGNInt::run(context, static_cast<NIntLikeConst*>(size)).getSExtValue();
			if (arrSize <= 0) {
				context.addError("Array size must be positive", *size);
				return nullptr;
			}
			return SType::getArray(context, btype, arrSize);
		}
		context.addError("Array size must be a constant integer", *size);
		return nullptr;
	} else {
		return SType::getArray(context, btype, 0);
	}
}

SType* CGNDataType::visitNVecType(NVecType* type)
{
	auto size = type->getSize();
	auto arrSize = CGNInt::run(context, size).getSExtValue();
	if (arrSize <= 0) {
		context.addError("vec size must be greater than 0", *size);
		return nullptr;
	}
	auto btype = visit(type->getBaseType());
	if (!btype) {
		return nullptr;
	} else if (!btype->isNumeric() && !btype->isPointer()) {
		context.addError("vec type only supports numeric and pointer types", *type->getBaseType());
		return nullptr;
	}
	return SType::getVec(context, btype, arrSize);
}


SType* CGNDataType::visitNUserType(NUserType* type)
{
	auto typeName = type->getName()->str;
	auto ty = SUserType::lookup(context, typeName);
	if (!ty) {
		context.addError(typeName + " type not declared", *type);
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
		context.addError("can't create pointer to auto type", *baseType);
		return nullptr;
	}
	return SType::getPointer(context, btype);
}

SType* CGNDataType::visitNFuncPointerType(NFuncPointerType* type)
{
	auto ptr = Builder::getFuncType(context, type->getReturnType(), type->getParams());
	return ptr? SType::getPointer(context, ptr) : nullptr;
}

#define TABLE_ADD2(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartDataType)] = reinterpret_cast<classPtr>(&CGNDataTypeNew::visit##ID)

CGNDataType::classPtr* CGNDataTypeNew::buildVTable()
{
	auto table = new CGNDataType::classPtr[NODEID_DIFF(NodeId::EndDataType, NodeId::StartDataType)];
	TABLE_ADD2(NArrayType);
	TABLE_ADD2(NBaseType);
	TABLE_ADD2(NFuncPointerType);
	TABLE_ADD2(NPointerType);
	TABLE_ADD2(NThisType);
	TABLE_ADD2(NUserType);
	TABLE_ADD2(NVecType);
	return table;
}

CGNDataType::classPtr* CGNDataTypeNew::vtable = CGNDataTypeNew::buildVTable();

SType* CGNDataTypeNew::visit(NDataType* type)
{
	return (this->*vtable[NODEID_DIFF(type->id(), NodeId::StartDataType)])(type);
}

SType* CGNDataTypeNew::run(CodeContext& context, NDataType* type, RValue& size)
{
	CGNDataTypeNew runner(context);
	auto ty = runner.visit(type);
	size = runner.sizeVal;
	return ty;
}

void CGNDataTypeNew::setSize(SType* type)
{
	if (type->isUnsized())
		sizeVal = RValue();
	else
		sizeVal = RValue::getNumVal(context, SType::getInt(context, 64), SType::allocSize(context, type));
}

void CGNDataTypeNew::setSize(uint64_t size)
{
	setSize(RValue::getNumVal(context, SType::getInt(context, 64), size));
}

void CGNDataTypeNew::setSize(const RValue& size)
{
	sizeVal = Inst::BinaryOp('*', nullptr, sizeVal, size, context);
}

SType* CGNDataTypeNew::visitNBaseType(NBaseType* type)
{
	auto ty = CGNDataType::visitNBaseType(type);
	if (ty)
		setSize(ty);
	return ty;
}

SType* CGNDataTypeNew::visitNConstType(NConstType* type)
{
	return SType::getConst(context, visit(type->getType()));
}

SType* CGNDataTypeNew::visitNThisType(NThisType* type)
{
	auto ty = CGNDataType::visitNThisType(type);
	if (ty)
		setSize(ty);
	return ty;
}

SType* CGNDataTypeNew::visitNArrayType(NArrayType* type)
{
	auto btype = getArrayType(type);
	if (!btype)
		return nullptr;
	auto size = type->getSize();
	if (size) {
		if (size->isConstant() && static_cast<NConstant*>(size)->isIntConst()) {
			auto arrSize = CGNInt::run(context, static_cast<NIntLikeConst*>(size)).getSExtValue();
			if (arrSize <= 0) {
				context.addError("Array size must be positive", *size);
				return nullptr;
			}
			setSize(arrSize);
			return SType::getArray(context, btype, arrSize);
		}
		setSize(CGNExpression::run(context, size));
	}
	return SType::getArray(context, btype, 0);
}

SType* CGNDataTypeNew::visitNVecType(NVecType* type)
{
	auto ty = CGNDataType::visitNVecType(type);
	if (ty)
		setSize(ty);
	return ty;
}

SType* CGNDataTypeNew::visitNUserType(NUserType* type)
{
	auto ty = CGNDataType::visitNUserType(type);
	if (ty)
		setSize(ty);
	return ty;
}

SType* CGNDataTypeNew::visitNPointerType(NPointerType* type)
{
	auto ty = CGNDataType::visitNPointerType(type);
	if (ty)
		setSize(ty);
	return ty;
}

SType* CGNDataTypeNew::visitNFuncPointerType(NFuncPointerType* type)
{
	auto ty = CGNDataType::visitNFuncPointerType(type);
	if (ty)
		setSize(ty);
	return ty;
}
