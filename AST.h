/*      Saphyr, a C++ style compiler using LLVM
        Copyright (C) 2012, Justin Madru (justin.jdm64@gmail.com)

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __AST__
#define __AST__

#include <vector>
#include <llvm/Value.h>
#include <llvm/Type.h>
#include <llvm/Instructions.h>
#include "Constants.h"

using namespace std;
using namespace llvm;

// forward declaration
class CodeContext;

class Node
{
public:
	virtual ~Node() {};

	virtual NodeType getNodeType() = 0;

	virtual Value* genCode(CodeContext& context) = 0;
};

template<typename NType>
class NodeList : public Node
{
	typedef typename vector<NType*>::iterator NTypeIter;

protected:
	vector<NType*> list;

public:
	Value* genCode(CodeContext& context)
	{
		for (auto item : list)
			item->genCode(context);
		return nullptr;
	}

	NodeType getNodeType()
	{
		return NodeType::BaseNodeList;
	}

	int size()
	{
		return list.size();
	}

	NTypeIter begin()
	{
		return list.begin();
	}

	NTypeIter end()
	{
		return list.end();
	}

	void addItem(NType* item)
	{
		list.push_back(item);
	}

	NType* getItem(int i)
	{
		return list[i];
	}

	NType* getFirst()
	{
		return list[0];
	}

	NType* getLast()
	{
		return list[list.size() - 1];
	}
};

class NStatement : public Node {};
typedef NodeList<NStatement> NStatementList;

extern NStatementList* programBlock;

class NExpression : public NStatement {};
typedef NodeList<NExpression> NExpressionList;

class NIdentifier : public NExpression
{
protected:
	string* name;

public:
	NIdentifier(string* name)
	: name(name) {}

	string* getName()
	{
		return name;
	}

	~NIdentifier()
	{
		delete name;
	}
};

class NQualifier : public NIdentifier
{
	QualifierType type;

public:
	NQualifier(QualifierType type)
	: NIdentifier(nullptr), type(type) {}

	Type* getVarType(CodeContext& context);

	Value* genCode(CodeContext& context)
	{
		return nullptr;
	}

	NodeType getNodeType()
	{
		return NodeType::Qualifier;
	}
};

class NVariableDecl : public NIdentifier
{
public:
	NVariableDecl(string* name)
	: NIdentifier(name) {}

	Value* genCode(CodeContext& context) {}

	NodeType getNodeType()
	{
		return NodeType::VarDecl;
	}
};
typedef NodeList<NVariableDecl> NVariableDeclList;

class NVariable : public NIdentifier
{
public:
	NVariable(string* name)
	: NIdentifier(name) {}

	Value* genCode(CodeContext& context);

	Type* getVarType(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::Variable;
	}
};

class NParameter : public NIdentifier
{
	NQualifier* type;
	Value* arg; // NOTE: not owned by NParameter

public:
	NParameter(NQualifier* type, string* name)
	: NIdentifier(name), type(type), arg(nullptr) {}

	// NOTE: this must be called before genCode()
	void setArgument(Value* argument)
	{
		arg = argument;
	}

	Type* getVarType(CodeContext& context)
	{
		return type->getVarType(context);
	}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::Parameter;
	}

	~NParameter()
	{
		delete type;
	}
};
typedef NodeList<NParameter> NParameterList;

class NVariableDeclGroup : public NStatement
{
	NQualifier* type;
	NVariableDeclList* variables;

public:
	NVariableDeclGroup(NQualifier* type, NVariableDeclList* variables)
	: type(type), variables(variables) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::VariableDecGroup;
	}

	~NVariableDeclGroup()
	{
		delete variables;
		delete type;
	}
};

class NFunctionDeclaration : public NStatement
{
	NQualifier* rtype;
	string* name;
	NParameterList* params;
	NStatementList* body;

public:
	NFunctionDeclaration(NQualifier* rtype, string* name, NParameterList* params, NStatementList* body)
	: rtype(rtype), name(name), params(params), body(body) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::FunctionDec;
	}

	~NFunctionDeclaration()
	{
		delete rtype;
		delete name;
		delete params;
		delete body;
	}
};

class NReturnStatement : public NStatement
{
	NExpression* value;

public:
	NReturnStatement(NExpression* value = nullptr)
	: value(value) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::ReturnStm;
	}

	~NReturnStatement()
	{
		delete value;
	}
};

class NWhileStatement : public NStatement
{
	NExpression* condition;
	NStatementList* body;
	bool isDoWhile;
	bool isUntil;

public:
	NWhileStatement(NExpression* condition, NStatementList* body, bool isDoWhile = false, bool isUntil = false)
	: condition(condition), body(body), isDoWhile(isDoWhile), isUntil(isUntil) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::WhileStm;
	}

	~NWhileStatement()
	{
		delete condition;
		delete body;
	}
};

class NForStatement : public NStatement
{
	NStatementList* preStm;
	NExpression* condition;
	NExpressionList* postExp;
	NStatementList* body;

public:
	NForStatement(NStatementList* preStm, NExpression* condition, NExpressionList* postExp, NStatementList* body)
	: preStm(preStm), condition(condition), postExp(postExp), body(body) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::ForStm;
	}

	~NForStatement()
	{
		delete preStm;
		delete condition;
		delete postExp;
		delete body;
	}
};

class NLoopBranch : public NStatement
{
	int type;

public:
	NLoopBranch(int type)
	: type(type) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::LoopBranch;
	}
};

class NAssignment : public NExpression
{
	int oper;
	NIdentifier* lhs;
	NExpression* rhs;

public:
	NAssignment(int oper, NIdentifier* lhs, NExpression* rhs)
	: oper(oper), lhs(lhs), rhs(rhs) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::Assignment;
	}

	~NAssignment()
	{
		delete lhs;
		delete rhs;
	}
};

class NTernaryOperator : public NExpression
{
	NExpression* condition;
	NExpression* trueVal;
	NExpression* falseVal;

public:
	NTernaryOperator(NExpression* condition, NExpression* trueVal, NExpression* falseVal)
	: condition(condition), trueVal(trueVal), falseVal(falseVal) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::TernaryOp;
	}

	~NTernaryOperator()
	{
		delete condition;
		delete trueVal;
		delete falseVal;
	}
};

class NBinaryOperator : public NExpression
{
protected:
	int oper;
	NExpression* lhs;
	NExpression* rhs;

public:
	NBinaryOperator(int oper, NExpression* lhs, NExpression* rhs)
	: oper(oper), lhs(lhs), rhs(rhs) {}

	~NBinaryOperator()
	{
		delete lhs;
		delete rhs;
	}
};

class NLogicalOperator : public NBinaryOperator
{
public:
	NLogicalOperator(int oper, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, lhs, rhs) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::LogicalOp;
	}
};

class NCompareOperator : public NBinaryOperator
{
public:
	NCompareOperator(int oper, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, lhs, rhs) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::CompareOp;
	}
};

class NBinaryMathOperator : public NBinaryOperator
{
public:
	NBinaryMathOperator(int oper, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, lhs, rhs) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::BinaryMathOp;
	}
};

class NUnaryOperator : public NExpression
{
protected:
	int oper;
	NExpression* unary;

public:
	NUnaryOperator(int oper, NExpression* unary)
	: oper(oper), unary(unary) {}

	~NUnaryOperator()
	{
		delete unary;
	}
};

class NUnaryMathOperator : public NUnaryOperator
{
public:
	NUnaryMathOperator(int oper, NExpression* unaryExp)
	: NUnaryOperator(oper, unaryExp) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::UnaryMath;
	}
};

class NFunctionCall : public NIdentifier
{
	NExpressionList* arguments;

public:
	NFunctionCall(string* name, NExpressionList* arguments)
	: NIdentifier(name), arguments(arguments) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::FunctionCall;
	}

	~NFunctionCall()
	{
		delete arguments;
	}
};

class NIncrement : public NExpression
{
	NVariable* variable;
	bool isIncrement;
	bool isPostfix;

public:
	NIncrement(NVariable* variable, bool isIncrement, bool isPostfix)
	: variable(variable), isIncrement(isIncrement), isPostfix(isPostfix) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::Increment;
	}

	~NIncrement()
	{
		delete variable;
	}
};

class NConstant : public NExpression
{
protected:
	QualifierType type;
	string* value;

public:
	NConstant(QualifierType type, string* value)
	: type(type), value(value) {}

	~NConstant()
	{
		delete value;
	}
};

class NIntConst : public NConstant
{
public:
	NIntConst(string* value, QualifierType type = QualifierType::INT)
	: NConstant(type, value) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::IntConst;
	}
};

class NFloatConst : public NConstant
{
public:
	NFloatConst(string* value, QualifierType type = QualifierType::FLOAT)
	: NConstant(type, value) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::FloatConst;
	}
};

#endif
