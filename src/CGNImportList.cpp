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
#include "AST.h"
#include "CGNImportList.h"
#include <iostream>

using namespace std;

void CGNImportList::run(NStatementList* list)
{
	CGNImportList runner;
	for (auto item : *list)
		runner.visit(item);
	cout << runner.buff;
}

void CGNImportList::visit(NStatement* stm)
{
	switch (stm->id()) {
	VISIT_CASE(NImportFileStm, stm)
	VISIT_CASE(NImportPkgStm, stm)
	VISIT_CASE(NPackageBlock, stm)
	VISIT_CASE(NPackageItem, stm)
	default:
		; // skip
	}
}

void CGNImportList::visitNPackageBlock(NPackageBlock* stm)
{
	for (auto item : *stm->getItems())
		visit(item);
}

void CGNImportList::visitNPackageItem(NPackageItem* stm)
{
	buff += "P:" + stm->getName()->str;
	auto val = stm->getValue();
	if (val)
		buff += "=" + val->str;
	buff += "\n";
}

void CGNImportList::visitNImportFileStm(NImportFileStm* stm)
{
	buff += "i:" + stm->getName()->str + "\n";
}

void CGNImportList::visitNImportPkgStm(NImportPkgStm* stm)
{
	buff += "I:" + stm->getName()->str + "\n";
}
