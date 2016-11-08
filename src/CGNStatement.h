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
#ifndef __CGNSTATEMENT_H__
#define __CGNSTATEMENT_H__

class CGNStatement
{
	typedef void (CGNStatement::*classPtr)(NStatement*);

	static classPtr *vtable;

	CodeContext& context;
	RValue storedValue;

	void visitNImportStm(NImportStm* stm);

	void visitNExpressionStm(NExpressionStm* stm);

	void visitNParameter(NParameter* stm);

	void visitNVariableDecl(NVariableDecl* stm);

	void visitNVariableDeclGroup(NVariableDeclGroup* stm);

	void visitNGlobalVariableDecl(NGlobalVariableDecl* stm);

	void visitNAliasDeclaration(NAliasDeclaration* stm);

	void visitNOpaqueDecl(NOpaqueDecl* stm);

	void visitNStructDeclaration(NStructDeclaration* stm);

	void visitNEnumDeclaration(NEnumDeclaration* stm);

	void visitNFunctionDeclaration(NFunctionDeclaration* stm);

	void visitNClassStructDecl(NClassStructDecl* stm);

	void visitNClassFunctionDecl(NClassFunctionDecl* stm);

	void visitNClassConstructor(NClassConstructor* stm);

	void visitNClassDestructor(NClassDestructor* stm);

	void visitNMemberInitializer(NMemberInitializer* stm);

	void visitNClassDeclaration(NClassDeclaration* stm);

	void visitNReturnStatement(NReturnStatement* stm);

	void visitNLoopStatement(NLoopStatement* stm);

	void visitNWhileStatement(NWhileStatement* stm);

	void visitNSwitchStatement(NSwitchStatement* stm);

	void visitNForStatement(NForStatement* stm);

	void visitNIfStatement(NIfStatement* stm);

	void visitNLabelStatement(NLabelStatement* stm);

	void visitNGotoStatement(NGotoStatement* stm);

	void visitNLoopBranch(NLoopBranch* stm);

	void visitNDeleteStatement(NDeleteStatement* stm);

	void visitNDestructorCall(NDestructorCall* stm);

	void visit(NStatementList* list);

	static classPtr* buildVTable();

public:
	explicit CGNStatement(CodeContext& context)
	: context(context) {}

	void visit(NStatement* type);

	void storeValue(RValue value)
	{
		storedValue = value;
	}

	static void run(CodeContext& context, NStatement* exp)
	{
		CGNStatement runner(context);
		return runner.visit(exp);
	}

	static void run(CodeContext& context, NStatementList* list)
	{
		CGNStatement runner(context);
		return runner.visit(list);
	}
};

#endif
