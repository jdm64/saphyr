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
#include "../AST.h"
#include "../parserbase.h"
#include "FormatContext.h"
#include "FMNExpression.h"
#include "FMNDataType.h"

string FMNExpression::visit(NExpression* exp)
{
	if (!exp)
		return string();
	switch (exp->id()) {
	VISIT_CASE_RETURN(NArrayVariable, exp)
	VISIT_CASE_RETURN(NArrowOperator, exp)
	VISIT_CASE_RETURN(NBaseVariable, exp)
	VISIT_CASE_RETURN(NDereference, exp)
	VISIT_CASE_RETURN(NMemberVariable, exp)
	VISIT_CASE_RETURN(NAddressOf, exp)
	VISIT_CASE_RETURN(NAssignment, exp)
	VISIT_CASE_RETURN(NBinaryMathOperator, exp)
	VISIT_CASE_RETURN(NBoolConst, exp)
	VISIT_CASE_RETURN(NCharConst, exp)
	VISIT_CASE_RETURN(NCompareOperator, exp)
	VISIT_CASE_RETURN(NExprVariable, exp)
	VISIT_CASE_RETURN(NFloatConst, exp)
	VISIT_CASE_RETURN(NFunctionCall, exp)
	VISIT_CASE_RETURN(NIncrement, exp)
	VISIT_CASE_RETURN(NIntConst, exp)
	VISIT_CASE_RETURN(NLogicalOperator, exp)
	VISIT_CASE_RETURN(NMemberFunctionCall, exp)
	VISIT_CASE_RETURN(NNewExpression, exp)
	VISIT_CASE_RETURN(NNullCoalescing, exp)
	VISIT_CASE_RETURN(NNullPointer, exp)
	VISIT_CASE_RETURN(NStringLiteral, exp)
	VISIT_CASE_RETURN(NTernaryOperator, exp)
	VISIT_CASE_RETURN(NUnaryMathOperator, exp)
	default:
		cout << "NodeId::" << to_string(static_cast<int>(exp->id())) << " unrecognized in FMNExpression" << endl;
		return string();
	}
}

string FMNExpression::visit(NExpressionList* list)
{
	string str = "";
	bool last = false;
	for (auto item : *list) {
		if (last) {
			str += ", ";
		} else {
			last = true;
		}
		str += visit(item);
	}
	return str;
}

string FMNExpression::visitNBaseVariable(NBaseVariable* exp)
{
	return exp->getName()->str;
}

string FMNExpression::visitNArrayVariable(NArrayVariable* exp)
{
	return visit(exp->getArrayVar()) + "[" + visit(exp->getIndex()) + "]";
}

string FMNExpression::visitNArrowOperator(NArrowOperator* exp)
{
	string line;
	NDataType* type;

	switch (exp->getType()) {
	case NArrowOperator::DATA:
		type = exp->getDataType();
		if (!type)
			return line;
		line += FMNDataType::run(context, type);
		break;
	case NArrowOperator::EXP:
		auto ex = exp->getExp();
		if (!ex)
			return line;
		line += visit(ex);
		break;
	}
	line += "->" + exp->getName()->str;
	auto arg = exp->getArg();
	if (arg) {
		line += "(" + FMNDataType::run(context, exp->getArg()) + ")";
	}
	return line;
}

string FMNExpression::visitNDereference(NDereference* exp)
{
	return visit(exp->getVar()) + "@";
}

string FMNExpression::visitNMemberVariable(NMemberVariable* exp)
{
	return visit(exp->getBaseVar()) + "." + exp->getMemberName()->str;
}

string FMNExpression::visitNAddressOf(NAddressOf* nVar)
{
	return visit(nVar->getVar()) + "$";
}

string FMNExpression::visitNExprVariable(NExprVariable* exp)
{
	return "(" + visit(exp->getExp()) + ")";
}

string FMNExpression::visitNAssignment(NAssignment* exp)
{
	string line = visit(exp->getLhs()) + " ";
	switch (exp->getOp()) {
	case '=':
		line += "=";
		break;
	case '+':
	case '&':
	case '/':
	case '%':
	case '*':
	case '|':
	case '-':
	case '^':
		line += static_cast<char>(exp->getOp());
		line += "=";
		break;
	case ParserBase::TT_DQ_MARK:
		line += "\?\?=";
		break;
	case ParserBase::TT_LSHIFT:
		line += "<<=";
		break;
	case ParserBase::TT_RSHIFT:
		line += ">>=";
		break;
	}
	return line + " " + visit(exp->getRhs());
}

string FMNExpression::visitNTernaryOperator(NTernaryOperator* exp)
{
	return visit(exp->getCondition()) + " ? " + visit(exp->getTrueVal()) + " : " + visit(exp->getFalseVal());
}

string FMNExpression::visitNNewExpression(NNewExpression* exp)
{
	string line = "new " + FMNDataType::run(context, exp->getType());
	auto args = exp->getArgs();
	if (args) {
		line += "{" + FMNExpression::run(context, args) + "}";
	}
	return line;
}

string FMNExpression::visitNLogicalOperator(NLogicalOperator* exp)
{
	string line = visit(exp->getLhs());
	switch (exp->getOp()) {
	case ParserBase::TT_LOG_OR:
		line += " || ";
		break;
	case ParserBase::TT_LOG_AND:
		line += " && ";
		break;
	}
	return line + visit(exp->getRhs());
}

string FMNExpression::visitNCompareOperator(NCompareOperator* exp)
{
	string line = visit(exp->getLhs());
	switch (exp->getOp()) {
	case '<':
		line += " < ";
		break;
	case '>':
		line += " > ";
		break;
	case ParserBase::TT_LEQ:
		line += " <= ";
		break;
	case ParserBase::TT_NEQ:
		line += " != ";
		break;
	case ParserBase::TT_EQ:
		line += " == ";
		break;
	case ParserBase::TT_GEQ:
		line += " >= ";
		break;
	}
	return line + visit(exp->getRhs());
}

string FMNExpression::visitNBinaryMathOperator(NBinaryMathOperator* exp)
{
	string line = visit(exp->getLhs()) + " ";
	switch (exp->getOp()) {
	case ParserBase::TT_LSHIFT:
		line += "<<";
		break;
	case ParserBase::TT_RSHIFT:
		line += ">>";
		break;
	default:
		line += static_cast<char>(exp->getOp());
	}
	return line + " " + visit(exp->getRhs());
}

string FMNExpression::visitNNullCoalescing(NNullCoalescing* exp)
{
	return visit(exp->getLhs()) + " ?? " + visit(exp->getRhs());
}

string FMNExpression::visitNUnaryMathOperator(NUnaryMathOperator* exp)
{
	return static_cast<char>(exp->getOp()) + visit(exp->getExp());
}

string FMNExpression::visitNFunctionCall(NFunctionCall* exp)
{
	string ret = exp->getName()->str;
	ret += "(";
	bool last = false;
	for (auto arg : *exp->getArguments()) {
		if (last) {
			ret += ", ";
		} else {
			last = true;
		}
		ret += visit(arg);
	}
	ret += ")";
	return ret;
}

string FMNExpression::visitNMemberFunctionCall(NMemberFunctionCall* exp)
{
	string ret = visit(exp->getBaseVar()) + ".";
	ret += exp->getName()->str;
	ret += "(";
	bool last = false;
	for (auto arg : *exp->getArguments()) {
		if (last) {
			ret += ", ";
		} else {
			last = true;
		}
		ret += visit(arg);
	}
	ret += ")";
	return ret;
}

string FMNExpression::visitNIncrement(NIncrement* exp)
{
	string op = exp->getOp() == ParserBase::TT_INC ? "++" : "--";
	string var = visit(exp->getVar());
	return exp->postfix() ? var + op : op + var;
}

string FMNExpression::visitNBoolConst(NBoolConst* exp)
{
	return exp->getStrVal();
}

string FMNExpression::visitNNullPointer(NNullPointer* exp)
{
	return exp->getStrVal();
}

string FMNExpression::visitNStringLiteral(NStringLiteral* exp)
{
	return "\"" + exp->getStrVal() + "\"";
}

string FMNExpression::visitNIntConst(NIntConst* exp)
{
	return exp->getStrVal();
}

string FMNExpression::visitNFloatConst(NFloatConst* exp)
{
	return exp->getStrVal();
}

string FMNExpression::visitNCharConst(NCharConst* exp)
{
	return "'" + exp->getStrVal() + "'";
}
