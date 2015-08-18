/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2014, Justin Madru (justin.jdm64@gmail.com)
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
#ifndef __AST_H__
#define __AST_H__

#include <vector>
#include <set>
#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Instructions.h>
#include "Value.h"
#include "Function.h"

// forward declaration
class CodeContext;

class Token
{
public:
	Token(string token, int lineNum)
	: str(std::move(token)), line(lineNum) {}

	string str;
	int line;
};

class Node
{
public:
	virtual ~Node() {};
};

template<typename NType>
class NodeList : public Node
{
	typedef typename vector<NType*>::iterator NTypeIter;
	bool doDelete;

protected:
	vector<NType*> list;

public:
	NodeList(bool doDelete = true)
	: doDelete(doDelete) {}

	template<typename OtherList>
	OtherList* move()
	{
		auto other = new OtherList;
		for (const auto item : *this)
			other->addItem(item);

		doDelete = false;
		delete this;

		return other;
	}

	bool empty() const
	{
		return list.empty();
	}

	int size() const
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

	NType* at(int i)
	{
		return list.at(i);
	}

	NType* front()
	{
		return list.empty()? nullptr : list.front();
	}

	NType* back()
	{
		return list.empty()? nullptr : list.back();
	}

	~NodeList()
	{
		if (doDelete) {
			for (auto i : list)
				delete i;
		}
	}
};

class NStatement : public Node
{
public:
	virtual void genCode(CodeContext& context) = 0;

	virtual bool isTerminator() const
	{
		return false;
	}
};

class NStatementList : public NodeList<NStatement>
{
public:
	void genCode(CodeContext& context) const
	{
		for (auto item : list)
			item->genCode(context);
	}
};

class NExpression : public NStatement
{
public:
	void genCode(CodeContext& context) final
	{
		genValue(context);
	};

	virtual bool isConstant() const
	{
		return false;
	}

	virtual bool isComplex() const
	{
		return true;
	}

	virtual RValue genValue(CodeContext& context) = 0;
};

class NExpressionList : public NodeList<NExpression>
{
public:
	void genCode(CodeContext& context) const
	{
		for (auto item : list)
			item->genValue(context);
	}
};

class NConstant : public NExpression
{
protected:
	Token* value;

public:
	NConstant(Token* token)
	: value(token) {}

	Token* getToken() const
	{
		return value;
	}

	bool isConstant() const
	{
		return true;
	}

	bool isComplex() const
	{
		return false;
	}

	virtual bool isIntConst() const
	{
		return false;
	}

	const string& getStrVal() const
	{
		return value->str;
	}

	static vector<string> getValueAndSuffix(const string& value)
	{
		auto pos = value.find('_');
		if (pos == string::npos)
			return {value};
		else
			return {value.substr(0, pos), value.substr(pos + 1)};
	}

	static string unescape(const string &val)
	{
		string str;
		str.reserve(val.size());

		for (int i = 0;;) {
			auto idx = val.find('\\', i);
			if (idx == string::npos) {
				str.append(val, i, string::npos);
				break;
			} else {
				char c = val.at(++idx);
				switch (c) {
				case '0': c = '\0'; break;
				case 'a': c = '\a'; break;
				case 'b': c = '\b'; break;
				case 'e': c =   27; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case 'v': c = '\v'; break;
				default:
					break;
				}
				str.append(val, i, idx - i - 1);
				str += c;
				i = idx + 1;
			}
		}
		return str;
	}

	static void remove(string& val, char c = '\'')
	{
		val.erase(std::remove(val.begin(), val.end(), c), val.end());
	}

	~NConstant()
	{
		delete value;
	}
};

class NNullPointer : public NConstant
{
public:
	NNullPointer(Token* token)
	: NConstant(token) {}

	RValue genValue(CodeContext& context);
};

class NStringLiteral : public NConstant
{
public:
	NStringLiteral(Token* str)
	: NConstant(str)
	{
		value->str = unescape(value->str.substr(1, value->str.size() - 2));
	}

	RValue genValue(CodeContext& context);
};

class NIntLikeConst : public NConstant
{
public:
	NIntLikeConst(Token* token)
	: NConstant(token) {}

	RValue genValue(CodeContext& context) final;

	virtual APSInt getIntVal(CodeContext& context) = 0;

	bool isIntConst() const
	{
		return true;
	}
};

class NBoolConst : public NIntLikeConst
{
	bool bvalue;

public:
	NBoolConst(Token* token, bool value)
	: NIntLikeConst(token), bvalue(value) {}

	APSInt getIntVal(CodeContext& context);
};

class NCharConst : public NIntLikeConst
{
public:
	NCharConst(Token* charStr)
	: NIntLikeConst(charStr)
	{
		value->str = value->str.substr(1, value->str.length() - 2);
	}

	APSInt getIntVal(CodeContext& context);
};

class NIntConst : public NIntLikeConst
{
	int base;

public:
	NIntConst(Token* value, int base = 10)
	: NIntLikeConst(value), base(base)
	{
		remove(value->str);
	}

	APSInt getIntVal(CodeContext& context);
};

class NFloatConst : public NConstant
{
public:
	NFloatConst(Token* value)
	: NConstant(value)
	{
		remove(value->str);
	}

	RValue genValue(CodeContext& context);
};

class NDeclaration : public NStatement
{
protected:
	Token* name;

public:
	NDeclaration(Token* name = nullptr)
	: name(name) {}

	Token* getNameToken() const
	{
		return name;
	}

	virtual const string& getName() const
	{
		return name->str;
	}

	~NDeclaration()
	{
		delete name;
	}
};

class NDataType : public Node
{
public:
	virtual SType* getType(CodeContext& context) = 0;
};
typedef NodeList<NDataType> NDataTypeList;

class NBaseType : public NDataType
{
	int type;

public:
	NBaseType(int type)
	: type(type) {}

	SType* getType(CodeContext& context);
};

class NArrayType : public NDataType
{
	NDataType* baseType;
	NIntConst* size;

public:
	NArrayType(NDataType* baseType, NIntConst* size = nullptr)
	: baseType(baseType), size(size) {}

	SType* getType(CodeContext& context);

	~NArrayType()
	{
		delete baseType;
		delete size;
	}
};

class NVecType : public NDataType
{
	NBaseType* baseType;
	NIntConst* size;

public:
	NVecType(NIntConst* size, NBaseType* baseType)
	: baseType(baseType), size(size) {}

	SType* getType(CodeContext& context);

	~NVecType()
	{
		delete baseType;
		delete size;
	}
};

class NUserType : public NDataType
{
	Token* name;

public:
	NUserType(Token* name)
	: name(name) {}

	SType* getType(CodeContext& context);

	const string& getName() const
	{
		return name->str;
	}

	~NUserType()
	{
		delete name;
	}
};

class NPointerType : public NDataType
{
	NDataType* baseType;

public:
	NPointerType(NDataType* baseType)
	: baseType(baseType) {}

	SType* getType(CodeContext& context);

	~NPointerType()
	{
		delete baseType;
	}
};

class NFuncPointerType : public NDataType
{
	NDataType* returnType;
	NDataTypeList* params;

public:
	NFuncPointerType(NDataType* returnType, NDataTypeList* params)
	: returnType(returnType), params(params) {}

	SType* getType(CodeContext& context)
	{
		auto ptr = getType(context, returnType, params);
		return ptr? SType::getPointer(context, ptr) : nullptr;
	}

	static SFunctionType* getType(CodeContext& context, NDataType* retType, NDataTypeList* params);

	~NFuncPointerType()
	{
		delete returnType;
		delete params;
	}
};

class NVariableDecl : public NDeclaration
{
protected:
	NExpression* initExp;
	NDataType* type;

public:
	NVariableDecl(Token* name, NExpression* initExp = nullptr)
	: NDeclaration(name), initExp(initExp), type(nullptr) {}

	// NOTE: must be called before genCode()
	void setDataType(NDataType* qtype)
	{
		type = qtype;
	}

	bool hasInit() const
	{
		return initExp;
	}

	NExpression* getInitExp() const
	{
		return initExp;
	}

	void genCode(CodeContext& context);

	~NVariableDecl()
	{
		delete initExp;
	}
};

class NVariableDeclList : public NodeList<NVariableDecl>
{
public:
	void genCode(NDataType* type, CodeContext& context) const
	{
		for (auto variable : list) {
			variable->setDataType(type);
			variable->genCode(context);
		}
	}
};

class NGlobalVariableDecl : public NVariableDecl
{
public:
	NGlobalVariableDecl(Token* name, NExpression* initExp = nullptr)
	: NVariableDecl(name, initExp) {}

	void genCode(CodeContext& context);
};

class NVariable : public NExpression
{
public:
	RValue genValue(CodeContext& context)
	{
		return genValue(context, loadVar(context));
	}

	virtual RValue genValue(CodeContext& context, RValue var);

	virtual RValue loadVar(CodeContext& context) = 0;

	virtual const string& getName() const = 0;
};

class NBaseVariable : public NVariable
{
	Token* name;

public:
	NBaseVariable(Token* name)
	: name(name) {}

	RValue loadVar(CodeContext& context);

	const string& getName() const
	{
		return name->str;
	}

	bool isComplex() const
	{
		return false;
	}

	~NBaseVariable()
	{
		delete name;
	}
};

class NArrayVariable : public NVariable
{
	NVariable* arrVar;
	NExpression* index;

public:
	NArrayVariable(NVariable* arrVar, NExpression* index)
	: arrVar(arrVar), index(index) {}

	RValue loadVar(CodeContext& context);

	const string& getName() const
	{
		return arrVar->getName();
	}

	~NArrayVariable()
	{
		delete arrVar;
		delete index;
	}
};

class NMemberVariable : public NVariable
{
	NVariable* baseVar;
	Token* memberName;

public:
	NMemberVariable(NVariable* baseVar, Token* memberName)
	: baseVar(baseVar), memberName(memberName) {}

	RValue loadVar(CodeContext& context);

	RValue loadStruct(CodeContext& context, RValue& baseValue, SStructType* structType) const;

	RValue loadUnion(CodeContext& context, RValue& baseValue, SUnionType* unionType) const;

	RValue loadEnum(CodeContext& context, SEnumType* enumType) const;

	const string& getName() const
	{
		return baseVar->getName();
	}

	const string& getMemberName() const
	{
		return memberName->str;
	}

	~NMemberVariable()
	{
		delete baseVar;
		delete memberName;
	}
};

class NExprVariable : public NVariable
{
	const static string STR_TMP_EXP;

	NExpression* expr;

public:
	NExprVariable(NExpression* expr)
	: expr(expr) {}

	RValue loadVar(CodeContext& context)
	{
		return expr->genValue(context);
	}

	const string& getName() const
	{
		return STR_TMP_EXP;
	}

	~NExprVariable()
	{
		delete expr;
	}
};

class NDereference : public NVariable
{
	NVariable* derefVar;

public:
	NDereference(NVariable* derefVar)
	: derefVar(derefVar) {}

	RValue loadVar(CodeContext& context);

	const string& getName() const
	{
		return derefVar->getName();
	}

	~NDereference()
	{
		delete derefVar;
	}
};

class NAddressOf : public NVariable
{
	NVariable* addVar;

public:
	NAddressOf(NVariable* addVar)
	: addVar(addVar) {}

	RValue genValue(CodeContext& context, RValue var)
	{
		return var? RValue(var, SType::getPointer(context, var.stype())) : var;
	}

	RValue loadVar(CodeContext& context);

	const string& getName() const
	{
		return addVar->getName();
	}

	~NAddressOf()
	{
		delete addVar;
	}
};

class NParameter : public NDeclaration
{
	NDataType* type;
	RValue arg; // NOTE: not owned by NParameter

public:
	NParameter(NDataType* type, Token* name)
	: NDeclaration(name), type(type) {}

	// NOTE: this must be called before genCode()
	void setArgument(RValue argument)
	{
		arg = argument;
	}

	SType* getType(CodeContext& context)
	{
		return type->getType(context);
	}

	NDataType* getTypeNode() const
	{
		return type;
	}

	void genCode(CodeContext& context);

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

	void genCode(CodeContext& context)
	{
		variables->genCode(type, context);
	}

	bool addMembers(vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context);

	~NVariableDeclGroup()
	{
		delete variables;
		delete type;
	}
};
typedef NodeList<NVariableDeclGroup> NVariableDeclGroupList;

class NAliasDeclaration : public NDeclaration
{
	NDataType* type;

public:
	NAliasDeclaration(Token* name, NDataType* type)
	: NDeclaration(name), type(type) {}

	void genCode(CodeContext& context);

	~NAliasDeclaration()
	{
		delete type;
	}
};

class NStructDeclaration : public NDeclaration
{
	NVariableDeclGroupList* list;

protected:
	virtual void createUserType(vector<pair<string, SType*> > structVars, CodeContext& context)
	{
		SUserType::createStruct(context, getName(), structVars);
	}

public:
	NStructDeclaration(Token* name, NVariableDeclGroupList* list)
	: NDeclaration(name), list(list) {}

	void genCode(CodeContext& context);

	~NStructDeclaration()
	{
		delete list;
	}
};

class NUnionDeclaration : public NStructDeclaration
{
protected:
	void createUserType(vector<pair<string, SType*> > structVars, CodeContext& context)
	{
		SUserType::createUnion(context, getName(), structVars);
	}

public:
	NUnionDeclaration(Token* name, NVariableDeclGroupList* list)
	: NStructDeclaration(name, list) {}
};

class NEnumDeclaration : public NDeclaration
{
	NVariableDeclList* variables;

public:
	NEnumDeclaration(Token* name, NVariableDeclList* variables)
	: NDeclaration(name), variables(variables) {}

	void genCode(CodeContext& context);

	~NEnumDeclaration()
	{
		delete variables;
	}
};

class NFunctionPrototype : public NDeclaration
{
	NDataType* rtype;
	NParameterList* params;

public:
	NFunctionPrototype(Token* name, NDataType* rtype, NParameterList* params)
	: NDeclaration(name), rtype(rtype), params(params) {}

	void genCode(CodeContext& context) final;

	SFunction genFunction(CodeContext& context);

	void genCodeParams(SFunction function, CodeContext& context) const;

	SFunctionType* getFunctionType(CodeContext& context)
	{
		NDataTypeList typeList(false);
		for (auto item : *params) {
			typeList.addItem(item->getTypeNode());
		}
		return NFuncPointerType::getType(context, rtype, &typeList);
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
	: prototype(prototype), body(body) {}

	void genCode(CodeContext& context);

	const string& getName() const
	{
		return prototype->getName();
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

class NLoopStatement : public NConditionStmt
{
public:
	NLoopStatement(NStatementList* body)
	: NConditionStmt(nullptr, body) {}

	void genCode(CodeContext& context);
};

class NWhileStatement : public NConditionStmt
{
	bool isDoWhile;
	bool isUntil;

public:
	NWhileStatement(NExpression* condition, NStatementList* body, bool isDoWhile = false, bool isUntil = false)
	: NConditionStmt(condition, body), isDoWhile(isDoWhile), isUntil(isUntil) {}

	void genCode(CodeContext& context);
};

class NSwitchCase : public NStatement
{
	NIntConst* value;
	NStatementList* body;

public:
	// used for default case
	NSwitchCase(NStatementList* body)
	: value(nullptr), body(body) {}

	NSwitchCase(NIntConst* value, NStatementList* body)
	: value(value), body(body) {}

	void genCode(CodeContext& context)
	{
		body->genCode(context);
	}

	ConstantInt* getValue(CodeContext& context);

	bool isValueCase() const
	{
		return value != nullptr;
	}

	bool isLastStmBranch() const
	{
		auto last = body->back();
		if (!last)
			return false;
		return last->isTerminator();
	}

	~NSwitchCase()
	{
		delete value;
		delete body;
	}
};
typedef NodeList<NSwitchCase> NSwitchCaseList;

class NSwitchStatement : public NStatement
{
	NExpression* value;
	NSwitchCaseList* cases;

public:
	NSwitchStatement(NExpression* value, NSwitchCaseList* cases)
	: value(value), cases(cases) {}

	void genCode(CodeContext& context);

	~NSwitchStatement()
	{
		delete value;
		delete cases;
	}
};

class NForStatement : public NConditionStmt
{
	NStatementList* preStm;
	NExpressionList* postExp;

public:
	NForStatement(NStatementList* preStm, NExpression* condition, NExpressionList* postExp, NStatementList* body)
	: NConditionStmt(condition, body), preStm(preStm), postExp(postExp) {}

	void genCode(CodeContext& context);

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

	void genCode(CodeContext& context);

	~NIfStatement()
	{
		delete elseBody;
	}
};

class NLabelStatement : public NDeclaration
{
public:
	NLabelStatement(Token* name)
	: NDeclaration(name) {}

	void genCode(CodeContext& context);
};

class NJumpStatement : public NStatement
{
public:
	bool isTerminator() const
	{
		return true;
	}
};

class NReturnStatement : public NJumpStatement
{
	NExpression* value;

public:
	NReturnStatement(NExpression* value = nullptr)
	: value(value) {}

	void genCode(CodeContext& context);

	~NReturnStatement()
	{
		delete value;
	}
};

class NGotoStatement : public NJumpStatement
{
	Token* name;

public:
	NGotoStatement(Token* name)
	: name(name) {}

	void genCode(CodeContext& context);

	const string& getName() const
	{
		return name->str;
	}

	~NGotoStatement()
	{
		delete name;
	}
};

class NLoopBranch : public NJumpStatement
{
	int type;
	NIntConst* level;

public:
	NLoopBranch(int type, NIntConst* level = nullptr)
	: type(type), level(level) {}

	void genCode(CodeContext& context);

	~NLoopBranch()
	{
		delete level;
	}
};

class NDeleteStatement : public NStatement
{
	NVariable *variable;

public:
	NDeleteStatement(NVariable* variable)
	: variable(variable) {}

	void genCode(CodeContext& context);

	~NDeleteStatement()
	{
		delete variable;
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

	RValue genValue(CodeContext& context);

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

	RValue genValue(CodeContext& context);

	~NTernaryOperator()
	{
		delete condition;
		delete trueVal;
		delete falseVal;
	}
};

class NNewExpression : public NExpression
{
	NDataType* type;

public:
	NNewExpression(NDataType* type)
	: type(type) {}

	RValue genValue(CodeContext& context);

	~NNewExpression()
	{
		delete type;
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

	RValue genValue(CodeContext& context);
};

class NCompareOperator : public NBinaryOperator
{
public:
	NCompareOperator(int oper, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NBinaryMathOperator : public NBinaryOperator
{
public:
	NBinaryMathOperator(int oper, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NNullCoalescing : public NBinaryOperator
{
public:
	NNullCoalescing(NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(0, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NSizeOfOperator : public NExpression
{
	enum OfType { DATA, EXP, NAME };

	OfType type;
	NDataType* dtype;
	NExpression* exp;
	Token* name;

public:
	NSizeOfOperator(NDataType* dtype)
	: type(DATA), dtype(dtype), exp(nullptr), name(nullptr) {}

	NSizeOfOperator(NExpression* exp)
	: type(EXP), dtype(nullptr), exp(exp), name(nullptr) {}

	NSizeOfOperator(Token* name)
	: type(NAME), dtype(nullptr), exp(nullptr), name(name) {}

	RValue genValue(CodeContext& context);

	bool isConstant() const
	{
		return type == EXP? exp->isConstant() : true;
	}

	~NSizeOfOperator()
	{
		delete dtype;
		delete exp;
		delete name;
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

	RValue genValue(CodeContext& context);
};

class NFunctionCall : public NVariable
{
	Token* name;
	NExpressionList* arguments;

public:
	NFunctionCall(Token* name, NExpressionList* arguments)
	: name(name), arguments(arguments) {}

	RValue genValue(CodeContext& context);

	RValue loadVar(CodeContext& context);

	const string& getName() const
	{
		return name->str;
	}

	~NFunctionCall()
	{
		delete name;
		delete arguments;
	}
};

class NIncrement : public NExpression
{
	NVariable* variable;
	int type;
	bool isPostfix;

public:
	NIncrement(NVariable* variable, int type, bool isPostfix)
	: variable(variable), type(type), isPostfix(isPostfix) {}

	RValue genValue(CodeContext& context);

	~NIncrement()
	{
		delete variable;
	}
};

#endif
