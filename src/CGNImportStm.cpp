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
#include "CodeContext.h"
#include "CGNImportStm.h"
#include "Builder.h"

#define TABLE_ADD(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartStatement)] = reinterpret_cast<classPtr>(&CGNImportStm::visit##ID)

CGNImportStm::classPtr* CGNImportStm::buildVTable()
{
	auto table = new CGNImportStm::classPtr[NODEID_DIFF(NodeId::EndStatement, NodeId::StartStatement)];
	TABLE_ADD(NAliasDeclaration);
	TABLE_ADD(NClassConstructor);
	TABLE_ADD(NClassDeclaration);
	TABLE_ADD(NClassDestructor);
	TABLE_ADD(NClassFunctionDecl);
	TABLE_ADD(NClassStructDecl);
	TABLE_ADD(NEnumDeclaration);
	TABLE_ADD(NFunctionDeclaration);
	TABLE_ADD(NGlobalVariableDecl);
	TABLE_ADD(NImportStm);
	TABLE_ADD(NStructDeclaration);
	TABLE_ADD(NVariableDeclGroup);
	return table;
}

CGNImportStm::classPtr* CGNImportStm::vtable = buildVTable();

void CGNImportStm::visit(NStatement* stm)
{
	(this->*vtable[NODEID_DIFF(stm->id(), NodeId::StartStatement)])(stm);
}

void CGNImportStm::visit(NStatementList* list)
{
	for (auto item : *list)
		visit(item);
}

void CGNImportStm::visitNImportStm(NImportStm* stm)
{
	auto idx = filepath.rfind('/');
	string prefix = idx != string::npos? filepath.substr(0, idx + 1) : "";
	string filename = prefix + stm->getName()->str;

	Builder::LoadImport(context, filename);
}

void CGNImportStm::visitNVariableDeclGroup(NVariableDeclGroup* stm)
{
	for (auto variable : *stm->getVars()) {
		variable->setDataType(stm->getType());
		visit(variable);
	}
}

void CGNImportStm::visitNGlobalVariableDecl(NGlobalVariableDecl* stm)
{
	Builder::CreateGlobalVar(context, stm, true);
}

void CGNImportStm::visitNAliasDeclaration(NAliasDeclaration* stm)
{
	Builder::CreateAlias(context, stm);
}

void CGNImportStm::visitNStructDeclaration(NStructDeclaration* stm)
{
	Builder::CreateStruct(context, stm->getType(), stm->getName(), stm->getVars());
}

void CGNImportStm::visitNEnumDeclaration(NEnumDeclaration* stm)
{
	Builder::CreateEnum(context, stm);
}

void CGNImportStm::visitNFunctionDeclaration(NFunctionDeclaration* stm)
{
	Builder::CreateFunction(context, stm->getName(), stm->getRType(), stm->getParams(), nullptr);
}

void CGNImportStm::visitNClassStructDecl(NClassStructDecl* stm)
{
	auto stToken = stm->getClass()->getName();
	auto stType = NStructDeclaration::CreateType::CLASS;
	Builder::CreateStruct(context, stType, stToken, stm->getVarList());
}

void CGNImportStm::visitNClassFunctionDecl(NClassFunctionDecl* stm)
{
	Builder::CreateClassFunction(context, stm, true);
}

void CGNImportStm::visitNClassConstructor(NClassConstructor* stm)
{
	Builder::CreateClassConstructor(context, stm, true);
}

void CGNImportStm::visitNClassDestructor(NClassDestructor* stm)
{
	Builder::CreateClassDestructor(context, stm, true);
}

void CGNImportStm::visitNClassDeclaration(NClassDeclaration* stm)
{
	Builder::CreateClass(context, stm, [=](int structIdx){
		visit(stm->getList()->at(structIdx));
		context.setClass(static_cast<SClassType*>(SUserType::lookup(context, stm->getName()->str)));

		for (int i = 0; i < stm->getList()->size(); i++) {
			if (i == structIdx)
				continue;
			visit(stm->getList()->at(i));
		}
		context.setClass(nullptr);
	});
}
