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
#ifndef __CGNIMPORT_H__
#define __CGNIMPORT_H__

class CGNImportStm
{
	typedef void (CGNImportStm::*classPtr)(NStatement*);

	static classPtr *vtable;

	CodeContext& context;

	explicit CGNImportStm(CodeContext& context)
	: context(context) {}

	void visitNImportStm(NImportStm* stm);

	void visitNVariableDeclGroup(NVariableDeclGroup* stm);

	void visitNGlobalVariableDecl(NGlobalVariableDecl* stm);

	void visitNAliasDeclaration(NAliasDeclaration* stm);

	void visitNStructDeclaration(NStructDeclaration* stm);

	void visitNEnumDeclaration(NEnumDeclaration* stm);

	void visitNFunctionDeclaration(NFunctionDeclaration* stm);

	void visitNClassStructDecl(NClassStructDecl* stm);

	void visitNClassFunctionDecl(NClassFunctionDecl* stm);

	void visitNClassConstructor(NClassConstructor* stm);

	void visitNClassDestructor(NClassDestructor* stm);

	void visitNMemberInitializer(NMemberInitializer* stm);

	void visitNClassDeclaration(NClassDeclaration* stm);

	void visit(NStatementList* list);

	void visit(NStatement* type);

	static classPtr* buildVTable();

public:

	static void run(CodeContext& context, NStatementList* list)
	{
		CGNImportStm runner(context);
		return runner.visit(list);
	}
};

#endif
