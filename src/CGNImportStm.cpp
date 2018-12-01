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
#include "CodeContext.h"
#include "CGNImportStm.h"
#include "Builder.h"

void CGNImportStm::visit(NStatement* stm)
{
	switch (stm->id()) {
	VISIT_CASE(NAliasDeclaration, stm)
	VISIT_CASE(NClassConstructor, stm)
	VISIT_CASE(NClassDeclaration, stm)
	VISIT_CASE(NClassDestructor, stm)
	VISIT_CASE(NClassFunctionDecl, stm)
	VISIT_CASE(NClassStructDecl, stm)
	VISIT_CASE(NEnumDeclaration, stm)
	VISIT_CASE(NFunctionDeclaration, stm)
	VISIT_CASE(NGlobalVariableDecl, stm)
	VISIT_CASE(NImportStm, stm)
	VISIT_CASE(NStructDeclaration, stm)
	VISIT_CASE(NVariableDeclGroup, stm)
	default:
		context.addError("NodeId::" + to_string(static_cast<int>(stm->id())) + " unrecognized in CGNImportStm", nullptr);
	}
}

void CGNImportStm::visit(NStatementList* list)
{
	for (auto item : *list)
		visit(item);
}

void CGNImportStm::visitNImportStm(NImportStm* stm)
{
	Builder::LoadImport(context, stm);
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
	NVariableDeclGroupList empty;
	auto vars = NAttributeList::find(stm->getAttrs(), "opaque")? &empty : stm->getVars();

	Builder::CreateStruct(context, stm->getType(), stm->getName(), vars);
}

void CGNImportStm::visitNEnumDeclaration(NEnumDeclaration* stm)
{
	Builder::CreateEnum(context, stm);
}

void CGNImportStm::visitNFunctionDeclaration(NFunctionDeclaration* stm)
{
	Builder::CreateFunction(context, stm->getName(), stm->getRType(), stm->getParams(), nullptr, stm->getAttrs());
}

void CGNImportStm::visitNClassStructDecl(NClassStructDecl* stm)
{
	auto cl = stm->getClass();
	auto stToken = cl->getName();
	auto stType = NStructDeclaration::CreateType::CLASS;
	NVariableDeclGroupList empty;
	auto vars = NAttributeList::find(cl->getAttrs(), "opaque")? &empty : stm->getVarList();

	Builder::CreateStruct(context, stType, stToken, vars);
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
	if (!context.inTemplate()) {
		auto name = stm->getName()->str;
		if (SUserType::isDeclared(context, name, {}) || context.getTemplate(name)) {
			context.addError("type with name " + name + " already declared", stm->getName());
			return;
		} else if (stm->getTemplateParams()) {
			context.storeTemplate(name, stm);
			return;
		}
	}
	Builder::CreateClass(context, stm, [=](size_t structIdx) {
		visit(stm->getMembers()->at(structIdx));
		if (!context.getClass())
			return;

		for (size_t i = 0; i < stm->getMembers()->size(); i++) {
			if (i == structIdx)
				continue;
			visit(stm->getMembers()->at(i));
		}
	});
}
