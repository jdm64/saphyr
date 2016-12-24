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
#include "AST.h"
#include "CGNImportList.h"
#include <iostream>

#define TABLE_ADD(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartStatement)] = reinterpret_cast<classPtr>(&CGNImportList::visit##ID)

CGNImportList::classPtr* CGNImportList::buildVTable()
{
	auto size = NODEID_DIFF(NodeId::EndStatement, NodeId::StartStatement);
	auto table = new CGNImportList::classPtr[size];
	for (auto i = 0; i < size; i++) {
		table[i] = nullptr;
	}
	TABLE_ADD(NImportStm);
	return table;
}

CGNImportList::classPtr* CGNImportList::vtable = CGNImportList::buildVTable();

void CGNImportList::visit(NStatement* stm)
{
	auto ptr = vtable[NODEID_DIFF(stm->id(), NodeId::StartStatement)];
	if (ptr)
		(this->*ptr)(stm);
}

void CGNImportList::visitNImportStm(NImportStm* stm)
{
	cout << stm->getName()->str << endl;
}
