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
#include <list>
#include <iostream>
#include <numeric>
#include "../AST.h"
#include "../parserbase.h"
#include "FormatContext.h"
#include "WriterUtil.h"
#include "FMNStatement.h"
#include "FMNExpression.h"
#include "FMNDataType.h"

void FMNStatement::visit(NStatement* stm)
{
	switch (stm->id()) {
	VISIT_CASE(NAliasDeclaration, stm)
	VISIT_CASE(NClassConstructor, stm)
	VISIT_CASE(NClassDeclaration, stm)
	VISIT_CASE(NClassDestructor, stm)
	VISIT_CASE(NClassFunctionDecl, stm)
	VISIT_CASE(NClassStructDecl, stm)
	VISIT_CASE(NDeleteStatement, stm)
	VISIT_CASE(NDestructorCall, stm)
	VISIT_CASE(NEnumDeclaration, stm)
	VISIT_CASE(NExpressionStm, stm)
	VISIT_CASE(NForStatement, stm)
	VISIT_CASE(NFunctionDeclaration, stm)
	VISIT_CASE(NGlobalVariableDecl, stm)
	VISIT_CASE(NGotoStatement, stm)
	VISIT_CASE(NIfStatement, stm)
	VISIT_CASE(NImportStm, stm)
	VISIT_CASE(NLabelStatement, stm)
	VISIT_CASE(NLoopBranch, stm)
	VISIT_CASE(NLoopStatement, stm)
	VISIT_CASE(NReturnStatement, stm)
	VISIT_CASE(NStructDeclaration, stm)
	VISIT_CASE(NSwitchStatement, stm)
	VISIT_CASE(NVariableDeclGroup, stm)
	VISIT_CASE(NWhileStatement, stm)
	default:
		cout << "NodeId::" << static_cast<int>(stm->id()) << " unrecognized in FMNStatement" << endl;
	}
}

void FMNStatement::visit(NStatementList* list)
{
	for (auto item : *list)
		visit(item);
}

void FMNStatement::visitNImportStm(NImportStm* stm)
{
	context.addLine("import \"" + stm->getName()->str + "\";");
}

void FMNStatement::visitNExpressionStm(NExpressionStm* stm)
{
	context.addLine(FMNExpression::run(context, stm->getExp()) + ";");
}

void FMNStatement::visitNVariableDeclGroup(NVariableDeclGroup* stm)
{
	auto line = FMNDataType::run(context, stm->getType()) + " ";
	bool first = true;
	for (auto var : *stm->getVars()) {
		if (!first)
			line += ", ";
		first = false;
		line += var->getName()->str;

		if (var->getInitExp()) {
			line += " = " + FMNExpression::run(context, var->getInitExp());
		} else if (var->getInitList()) {
			line += "{" + FMNExpression::run(context, var->getInitList()) + "}";
		}
	}
	context.addLine(line + ";");
}

void FMNStatement::visitNGlobalVariableDecl(NGlobalVariableDecl* stm)
{
	auto line = FMNDataType::run(context, stm->getType()) + " " + stm->getName()->str;
	if (stm->getInitExp()) {
		line += " = " + FMNExpression::run(context, stm->getInitExp());
	} else if (stm->getInitList()) {
		line += "{" + FMNExpression::run(context, stm->getInitList()) + "}";
	}
	context.addLine(line);
}

void FMNStatement::visitNAliasDeclaration(NAliasDeclaration* stm)
{
	auto type = FMNDataType::run(context, stm->getType());
	context.addLine("alias " + stm->getName()->str + " = " + type +";");
}

void FMNStatement::visitNStructDeclaration(NStructDeclaration* stm)
{
	context.addLine("");
	WriterUtil::writeAttr(context, stm->getAttrs());
	string type;
	switch (stm->getType()) {
	case NStructDeclaration::CreateType::CLASS:
		type = "class";
		break;
	case NStructDeclaration::CreateType::STRUCT:
		type = "struct";
		break;
	case NStructDeclaration::CreateType::UNION:
		type = "union";
		break;
	}
	context.addLine(type + " " + stm->getName()->str);
	if (!stm->getVars()) {
		context.add(";");
		return;
	}
	context.addLine("{");
	context.indent();
	for (auto item : *stm->getVars())
		visit(item);
	context.undent();
	context.addLine("}");
}

void FMNStatement::visitNEnumDeclaration(NEnumDeclaration* stm)
{
	context.addLine("");
	context.addLine("enum " + stm->getName()->str);
	if (stm->getBaseType()) {
		context.add("<" + FMNDataType::run(context, stm->getBaseType()) + ">");
	}
	context.addLine("{");
	context.indent();
	string line;
	bool first = true;
	for (auto item : *stm->getVarList()) {
		if (!first)
			line += ", ";
		first = false;
		line += item->getName()->str;
		if (item->getInitExp()) {
			line += " = " + FMNExpression::run(context, item->getInitExp());
		} else if (item->getInitList()) {
			line += "{" + FMNExpression::run(context, item->getInitList()) + "}";
		}
	}
	context.addLine(line);
	context.undent();
	context.addLine("}");
}

void FMNStatement::visitNFunctionDeclaration(NFunctionDeclaration* stm)
{
	WriterUtil::writeFunctionDecl(stm->getName()->str, stm->getAttrs(), stm->getRType(), stm->getParams(), nullptr, stm->getBody(), context);
}

void FMNStatement::visitNClassStructDecl(NClassStructDecl* stm)
{
	context.addLine("struct this");
	context.addLine("{");
	context.indent();
	for (auto item : *stm->getVarList()) {
		visit(item);
	}
	context.undent();
	context.addLine("}");
}

void FMNStatement::visitNClassFunctionDecl(NClassFunctionDecl* stm)
{
	WriterUtil::writeFunctionDecl(stm->getName()->str, stm->getAttrs(), stm->getRType(), stm->getParams(), nullptr, stm->getBody(), context);
}

void FMNStatement::visitNClassConstructor(NClassConstructor* stm)
{
	WriterUtil::writeFunctionDecl("this", stm->getAttrs(), stm->getRType(), stm->getParams(), stm->getInitList(), stm->getBody(), context);
}

void FMNStatement::visitNClassDestructor(NClassDestructor* stm)
{
	WriterUtil::writeFunctionDecl("~this", stm->getAttrs(), stm->getRType(), stm->getParams(), nullptr, stm->getBody(), context);
}

void FMNStatement::visitNClassDeclaration(NClassDeclaration* stm)
{
	context.addLine("");;
	WriterUtil::writeAttr(context, stm->getAttrs());
	context.addLine("class " + stm->getName()->str);
	if (!stm->getMembers()) {
		context.add(";");
		return;
	}
	context.addLine("{");
	context.indent();
	for (auto item : *stm->getMembers()) {
		visit(item);
	}
	context.undent();
	context.addLine("}");
}

void FMNStatement::visitNReturnStatement(NReturnStatement* stm)
{
	auto val = stm->getValue() ? FMNExpression::run(context, stm->getValue()) : "";
	context.addLine("return " + val + ";");
}

void FMNStatement::visitNLoopStatement(NLoopStatement* stm)
{
	auto expr = FMNExpression::run(context, stm->getCond());
	context.addLine("loop");
	WriterUtil::writeBlockStmt(context, stm->getBody());
}

void FMNStatement::visitNWhileStatement(NWhileStatement* stm)
{
	auto expr = FMNExpression::run(context, stm->getCond());
	string type = stm->until() ? "until" : "while";

	if (stm->doWhile()) {
		context.addLine("do");
	} else {
		context.addLine(type + " (" + expr + ")");
	}

	WriterUtil::writeBlockStmt(context, stm->getBody());

	if (stm->doWhile()) {
		if (WriterUtil::isBlockStmt(stm->getBody())) {
			context.add(" " + type + " (" + expr + ");");
		} else {
			context.addLine(type + " (" + expr + ");");
		}
	}
}

void FMNStatement::visitNSwitchStatement(NSwitchStatement* stm)
{
	context.addLine("switch (" + FMNExpression::run(context, stm->getValue()) + ") {");
	for (auto s : *stm->getCases()) {
		if (s->isValueCase()) {
			context.addLine("case " + FMNExpression::run(context, s->getValue()) + ":");
		} else {
			context.addLine("default:");
		}
		context.indent();
		visit(s->getBody());
		context.undent();
	}
	context.addLine("}");
}

void FMNStatement::visitNForStatement(NForStatement* stm)
{
	vector<string> lines;
	context.setBuffer(&lines);
	visit(stm->getPreStm());
	context.setBuffer(nullptr);

	context.addLine("for (");
	context.add(accumulate(lines.begin(), lines.end(), string(), [](string& a, string& b) {
		b.erase(b.begin(), find_if(b.begin(), b.end(), [](int ch) {
			return !isspace(ch);
		}));
		b.pop_back(); // remove ;
		return a + (a.length() > 0 ? ", " : "") + b;
	}));
	context.add("; " + FMNExpression::run(context, stm->getCond())  + "; ");
	context.add(FMNExpression::run(context, stm->getPostExp()));
	context.add(")");

	WriterUtil::writeBlockStmt(context, stm->getBody());
}

void FMNStatement::visitNIfStatement(NIfStatement* stm)
{
	vector<NIfStatement*> ifSmts;
	auto curr = stm;
	NStatementList* elseBody = nullptr;

	ifSmts.push_back(stm);
	while (curr) {
		elseBody = curr->getElseBody();
		if (elseBody && elseBody->size() == 1 && elseBody->at(0)->id() == NodeId::NIfStatement) {
			curr = static_cast<NIfStatement*>(elseBody->at(0));
			ifSmts.push_back(curr);
		} else {
			curr = nullptr;
		}
	}

	bool useBlock = elseBody && WriterUtil::isBlockStmt(elseBody);
	if (!useBlock) {
		for (auto ifStmt : ifSmts) {
			useBlock |= WriterUtil::isBlockStmt(ifStmt->getBody());
			if (useBlock)
				break;
		}
	}

	bool first = true;
	for (auto ifStmt : ifSmts) {
		auto cond = "if (" +FMNExpression::run(context, ifStmt->getCond()) + ")";
		if (first) {
			first = false;
			context.addLine(cond);
		} else if (useBlock) {
			context.add(" else " + cond);
		} else {
			context.addLine("else " + cond);
		}
		WriterUtil::writeBlockStmt(context, ifStmt->getBody(), useBlock);
	}

	if (elseBody) {
		if (useBlock)
			context.add(" else");
		else
			context.addLine("else");
		WriterUtil::writeBlockStmt(context, elseBody, useBlock);
	}
}

void FMNStatement::visitNLabelStatement(NLabelStatement* stm)
{
	int indent = context.getIndent();
	context.setIndent(0);
	context.addLine(stm->getName()->str + ":");
	context.setIndent(indent);
}

void FMNStatement::visitNGotoStatement(NGotoStatement* stm)
{
	context.addLine("goto " + stm->getName()->str + ";");
}

void FMNStatement::visitNLoopBranch(NLoopBranch* stm)
{
	string line;
	switch (stm->getType()) {
	case ParserBase::TT_CONTINUE:
		line = "continue";
		break;
	case ParserBase::TT_REDO:
		line = "redo";
		break;
	case ParserBase::TT_BREAK:
		line = "break";
		break;
	default:
		return;
	}
	if (stm->getLevel())
		line += " " + stm->getLevel()->getStrVal();
	context.addLine(line + ";");
}

void FMNStatement::visitNDeleteStatement(NDeleteStatement* stm)
{
	context.addLine("delete " + FMNExpression::run(context, stm->getVar()) + ";");
}

void FMNStatement::visitNDestructorCall(NDestructorCall* stm)
{
	string line;
	Token* var = *stm->getVar();
	if (var->str != "this")
		line += FMNExpression::run(context, stm->getVar()) + ".";
	context.addLine(line + "~this();");
}
