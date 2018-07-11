/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2018, Justin Madru (justin.jdm64@gmail.com)
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
#include "FormatContext.h"
#include "WriterUtil.h"
#include "FMNStatement.h"
#include "FMNExpression.h"
#include "FMNDataType.h"

void WriterUtil::writeAttr(FormatContext& context, NAttributeList* attrs)
{
	if (!attrs)
		return;

	string data = "#[";
	bool lastAttr = false;
	for (auto attr : *attrs) {
		if (lastAttr) {
			data += ", ";
		} else {
			lastAttr = true;
		}
		data += attr->getName()->str;
		if (attr->getValues()) {
			data += "(";
			bool lastVal = false;
			for (auto val : *attr->getValues()) {
				if (lastVal) {
					data += ", ";
				} else {
					lastVal = true;
				}
				data += "\"" + val->str() + "\"";
			}
			data += ")";
		}
	}
	data += "]";

	context.addLine(data);
}

bool WriterUtil::isBlockStmt(NStatementList* stmts)
{
	if (!stmts)
		return false;
	auto size = stmts->size();
	return size == 0 || size > 1 || stmts->at(0)->isBlockStmt();
}

void WriterUtil::writeBlockStmt(FormatContext& context, NStatementList* stmts, bool forceBlock)
{
	bool writeBlock = forceBlock || isBlockStmt(stmts);

	if (writeBlock)
		context.add(" {");
	context.indent();
	FMNStatement::run(context, stmts);
	context.undent();
	if (writeBlock)
		context.addLine("}");
}

void WriterUtil::writeFunctionDecl(const string& name, NAttributeList* attrs, NDataType* rtype, NParameterList* params, NInitializerList* initList, NStatementList* body, FormatContext& context)
{
	context.addLine("");
	writeAttr(context, attrs);
	auto rty = rtype ? FMNDataType::run(context, rtype) + " " : "";
	string paramStr = "";
	bool first = true;

	for (auto param : *params) {
		if (!first)
			paramStr += ", ";
		first = false;
		paramStr += FMNDataType::run(context, param->getType());
		paramStr += " " + param->getName()->str;
	}
	context.addLine(rty + name + "(" + paramStr + ")");
	if (initList) {
		string initStr;
		bool last = false;
		for (auto init : *initList) {
			if (last) {
				initStr += ", ";
			} else {
				last = true;
			}
			initStr += init->getName()->str + "{";
			initStr += FMNExpression::run(context, init->getExp());
			initStr += "}";
		}
		context.addLine(initStr);
	}

	if (body) {
		context.addLine("{");
		context.indent();
		FMNStatement::run(context, body);
		context.undent();
		context.addLine("}");
	} else {
		context.add(";");
	}
}
