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
#include <llvm/Function.h>
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

class NStatement : public Node
{
public:
	virtual Value* genCode(CodeContext& context) = 0;
};
typedef NodeList<NStatement> NStatementList;

extern NStatementList* programBlock;

class NExpression : public NStatement {};
typedef NodeList<NExpression> NExpressionList;

class NDeclaration : public NStatement
{
protected:
	string* name;

public:
	NDeclaration(string* name)
	: name(name) {}

	string* getName()
	{
		return name;
	}

	~NDeclaration()
	{
		delete name;
	}
};

class NDataType : public Node
{
protected:
	BaseDataType type;

public:
	NDataType(BaseDataType type)
	: type(type) {}

	virtual Type* getType(CodeContext& context) = 0;
};

class NBaseType : public NDataType
{
public:
	NBaseType(BaseDataType type = BaseDataType::AUTO)
	: NDataType(type) {}

	Type* getType(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::BaseType;
	}
};

class NVariableDecl : public NDeclaration
{
protected:
	NExpression* initExp;
	NDataType* type;

public:
	NVariableDecl(string* name, NExpression* initExp = nullptr)
	: NDeclaration(name), initExp(initExp), type(nullptr) {}

	// NOTE: must be called before genCode()
	void setDataType(NDataType* qtype)
	{
		type = qtype;
	}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::VarDecl;
	}

	~NVariableDecl()
	{
		delete initExp;
		delete type;
	}
};
typedef NodeList<NVariableDecl> NVariableDeclList;

class NGlobalVariableDecl : public NVariableDecl
{
public:
	NGlobalVariableDecl(string* name, NExpression* initExp = nullptr)
	: NVariableDecl(name, initExp) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::GlobalVarDecl;
	}
};

class NVariable : public NExpression
{
	string* name;

public:
	NVariable(string* name)
	: name(name) {}

	Value* genCode(CodeContext& context);

	string* getName()
	{
		return name;
	}

	NodeType getNodeType()
	{
		return NodeType::Variable;
	}

	~NVariable()
	{
		delete name;
	}
};

class NParameter : public NDeclaration
{
	NDataType* type;
	Value* arg; // NOTE: not owned by NParameter

public:
	NParameter(NDataType* type, string* name)
	: NDeclaration(name), type(type), arg(nullptr) {}

	// NOTE: this must be called before genCode()
	void setArgument(Value* argument)
	{
		arg = argument;
	}

	Type* getType(CodeContext& context)
	{
		return type->getType(context);
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
	NDataType* type;
	NVariableDeclList* variables;

public:
	NVariableDeclGroup(NDataType* type, NVariableDeclList* variables)
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

class NFunctionPrototype : public NDeclaration
{
	NDataType* rtype;
	NParameterList* params;

public:
	NFunctionPrototype(string* name, NDataType* rtype, NParameterList* params)
	: NDeclaration(name), rtype(rtype), params(params) {}

	Value* genCode(CodeContext& context);

	void genCodeParams(Function* function, CodeContext& context);

	FunctionType* getFunctionType(CodeContext& context)
	{
		vector<Type*> args;
		for (auto item : *params)
			args.push_back(item->getType(context));

		auto returnType = rtype->getType(context);
		return FunctionType::get(returnType, args, false);
	}

	NodeType getNodeType()
	{
		return NodeType::FunctionProto;
	}

	~NFunctionPrototype()
	{
		delete rtype;
		delete params;
	}
};

class NFunctionDeclaration : public NDeclaration
{
	NFunctionPrototype* prototype;
	NStatementList* body;

public:
	NFunctionDeclaration(NFunctionPrototype* prototype, NStatementList* body)
	: NDeclaration(prototype->getName()), prototype(prototype), body(body) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::FunctionDec;
	}

	~NFunctionDeclaration()
	{
		delete prototype;
		delete body;
	}
};

class NConditionStmt : public NStatement
{
protected:
	NExpression* condition;
	NStatementList* body;

public:
	NConditionStmt(NExpression* condition, NStatementList* body)
	: condition(condition), body(body) {}

	~NConditionStmt()
	{
		delete condition;
		delete body;
	}
};

class NWhileStatement : public NConditionStmt
{
	bool isDoWhile;
	bool isUntil;

public:
	NWhileStatement(NExpression* condition, NStatementList* body, bool isDoWhile = false, bool isUntil = false)
	: NConditionStmt(condition, body), isDoWhile(isDoWhile), isUntil(isUntil) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::WhileStm;
	}
};

class NForStatement : public NConditionStmt
{
	NStatementList* preStm;
	NExpressionList* postExp;

public:
	NForStatement(NStatementList* preStm, NExpression* condition, NExpressionList* postExp, NStatementList* body)
	: NConditionStmt(condition, body), preStm(preStm), postExp(postExp) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::ForStm;
	}

	~NForStatement()
	{
		delete preStm;
		delete postExp;
	}
};

class NIfStatement : public NConditionStmt
{
	NStatementList* elseBody;

public:
	NIfStatement(NExpression* condition, NStatementList* ifBody, NStatementList* elseBody)
	: NConditionStmt(condition, ifBody), elseBody(elseBody) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::IfStm;
	}

	~NIfStatement()
	{
		delete elseBody;
	}
};

class NLabelStatement : public NStatement
{
	string* name;

public:
	NLabelStatement(string* name)
	: name(name) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::Label;
	}

	~NLabelStatement()
	{
		delete name;
	}
};

class NJumpStatement : public NStatement
{
	// For clean type hierarchy
};

class NReturnStatement : public NJumpStatement
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

class NGotoStatement : public NJumpStatement
{
	string* name;

public:
	NGotoStatement(string* name)
	: name(name) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::Goto;
	}

	~NGotoStatement()
	{
		delete name;
	}
};

class NLoopBranch : public NJumpStatement
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
	NVariable* lhs;
	NExpression* rhs;

public:
	NAssignment(int oper, NVariable* lhs, NExpression* rhs)
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

class NNullCoalescing : public NBinaryOperator
{
public:
	NNullCoalescing(NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(0, lhs, rhs) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::NullCoalescing;
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

class NFunctionCall : public NExpression
{
	string* name;
	NExpressionList* arguments;

public:
	NFunctionCall(string* name, NExpressionList* arguments)
	: name(name), arguments(arguments) {}

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
	BaseDataType type;
	string* value;

public:
	NConstant(BaseDataType type, string* value)
	: type(type), value(value) {}

	~NConstant()
	{
		delete value;
	}
};

class NIntConst : public NConstant
{
public:
	NIntConst(string* value, BaseDataType type = BaseDataType::INT)
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
	NFloatConst(string* value, BaseDataType type = BaseDataType::FLOAT)
	: NConstant(type, value) {}

	Value* genCode(CodeContext& context);

	NodeType getNodeType()
	{
		return NodeType::FloatConst;
	}
};

#endif
