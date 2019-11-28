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
#include <iostream>
#include "../AST.h"
#include "../parserbase.h"
#include "FormatContext.h"
#include "FMNDataType.h"
#include "FMNExpression.h"

string FMNDataType::visit(NDataType* type)
{
	switch (type->id()) {
	VISIT_CASE_RETURN(NArrayType, type)
	VISIT_CASE_RETURN(NBaseType, type)
	VISIT_CASE_RETURN(NConstType, type)
	VISIT_CASE_RETURN(NFuncPointerType, type)
	VISIT_CASE_RETURN(NPointerType, type)
	VISIT_CASE_RETURN(NThisType, type)
	VISIT_CASE_RETURN(NUserType, type)
	VISIT_CASE_RETURN(NVecType, type)
	default:
		cout << "NodeId::" << to_string(static_cast<int>(type->id())) << " unrecognized in FMNDataType" << endl;
		return "";
	}
}

string FMNDataType::visitNBaseType(NBaseType* type)
{
	switch (type->getType()) {
	case ParserBase::TT_VOID:
		return "void";
	case ParserBase::TT_BOOL:
		return "bool";
	case ParserBase::TT_INT8:
		return "int8";
	case ParserBase::TT_INT16:
		return "int16";
	case ParserBase::TT_INT:
		return "int";
	case ParserBase::TT_INT32:
		return "int32";
	case ParserBase::TT_INT64:
		return "int64";
	case ParserBase::TT_UINT8:
		return "uint8";
	case ParserBase::TT_UINT16:
		return "uint16";
	case ParserBase::TT_UINT:
		return "uint";
	case ParserBase::TT_UINT32:
		return "uint32";
	case ParserBase::TT_UINT64:
		return "uint64";
	case ParserBase::TT_FLOAT:
		return "float";
	case ParserBase::TT_DOUBLE:
		return "double";
	default:
		return "auto";
	}
}

string FMNDataType::visitNConstType(NConstType* type)
{
	return "const " + visit(type->getType());
}

string FMNDataType::visitNThisType(NThisType* type)
{
	return "this";
}

string FMNDataType::visitNArrayType(NArrayType* type)
{
	auto size = FMNExpression::run(context, type->getSize());
	return "[" + size + "]" + visit(type->getBaseType());
}

string FMNDataType::visitNVecType(NVecType* type)
{
	auto size = FMNExpression::run(context, type->getSize());
	return "vec<" + size + "," + visit(type->getBaseType()) + ">";
}

string FMNDataType::visitNUserType(NUserType* type)
{
	auto ret = type->getName()->str;
	if (type->getTemplateArgs()) {
		ret += "<";
		bool first = true;
		for (auto item : * type->getTemplateArgs()) {
			if (first)
				first = false;
			else
				ret += ",";
			ret += visit(item);
		}
		ret += ">";
	}
	return ret;
}

string FMNDataType::visitNPointerType(NPointerType* type)
{
	return "@" + visit(type->getBaseType());
}

string FMNDataType::visitNFuncPointerType(NFuncPointerType* type)
{
	string ret = "@(";
	bool last = false;
	for (auto param : *type->getParams()) {
		if (last) {
			ret += ",";
		} else {
			last = true;
		}
		ret += visit(param);
	}
	ret += ")" + visit(type->getReturnType());
	return ret;
}
