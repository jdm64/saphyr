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
#include "CGNInt.h"
#include "CodeContext.h"

#define TABLE_ADD(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartExpression)] = reinterpret_cast<classPtr>(&CGNInt::visit##ID)

CGNInt::classPtr* CGNInt::buildVTable()
{
	auto table = new CGNInt::classPtr[NODEID_DIFF(NodeId::EndExpression, NodeId::StartExpression)];
	TABLE_ADD(NBoolConst);
	TABLE_ADD(NCharConst);
	TABLE_ADD(NIntConst);
	return table;
}

CGNInt::classPtr* CGNInt::vtable = CGNInt::buildVTable();

APSInt CGNInt::visit(NIntLikeConst* intConst)
{
	return (this->*vtable[NODEID_DIFF(intConst->id(), NodeId::StartExpression)])(intConst);
}

APSInt CGNInt::visitNBoolConst(NBoolConst* boolConst)
{
	return APSInt(APInt(1, boolConst->getValue()));
}

APSInt CGNInt::visitNIntConst(NIntConst* incConst)
{
	static const map<string, SType*> suffix = {
		{"i8", SType::getInt(context, 8)},
		{"u8", SType::getInt(context, 8, true)},
		{"i16", SType::getInt(context, 16)},
		{"u16", SType::getInt(context, 16, true)},
		{"i32", SType::getInt(context, 32)},
		{"u32", SType::getInt(context, 32, true)},
		{"i64", SType::getInt(context, 64)},
		{"u64", SType::getInt(context, 64, true)} };
	auto type = SType::getInt(context, 32); // default is int32

	auto data = NConstant::getValueAndSuffix(incConst->getStrVal());
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid integer suffix: " + data[1], incConst->getToken());
		else
			type = suf->second;
	}
	auto base = incConst->getBase();
	string intVal(data[0], base == 10? 0:2);
	return APSInt(APInt(type->size(), intVal, base), type->isUnsigned());
}

APSInt CGNInt::visitNCharConst(NCharConst* intConst)
{
	auto strVal = intConst->getStrVal();
	char cVal = strVal.at(0);
	if (cVal == '\\' && strVal.length() > 1) {
		switch (strVal.at(1)) {
		case '0': cVal = '\0'; break;
		case 'a': cVal = '\a'; break;
		case 'b': cVal = '\b'; break;
		case 'e': cVal =   27; break;
		case 'f': cVal = '\f'; break;
		case 'n': cVal = '\n'; break;
		case 'r': cVal = '\r'; break;
		case 't': cVal = '\t'; break;
		case 'v': cVal = '\v'; break;
		default: cVal = strVal.at(1);
		}
	}
	return APSInt(APInt(8, cVal, true));
}
