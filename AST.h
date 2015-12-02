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
#include "Token.h"
#include "Value.h"
#include "Function.h"

// forward declaration
class CodeContext;

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
	explicit NodeList(bool doDelete = true)
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

	void addItemFront(NType* item)
	{
		list.insert(list.begin(), item);
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
	explicit NConstant(Token* token)
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
	explicit NNullPointer(Token* token)
	: NConstant(token) {}

	RValue genValue(CodeContext& context);
};

class NStringLiteral : public NConstant
{
public:
	explicit NStringLiteral(Token* str)
	: NConstant(str)
	{
		value->str = unescape(value->str.substr(1, value->str.size() - 2));
	}

	RValue genValue(CodeContext& context);
};

class NIntLikeConst : public NConstant
{
public:
	explicit NIntLikeConst(Token* token)
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
	explicit NCharConst(Token* charStr)
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
	explicit NFloatConst(Token* value)
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
	explicit NDeclaration(Token* name)
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

class NNamedType : public NDataType
{
protected:
	Token* token;

public:
	explicit NNamedType(Token* token)
	: token(token) {}

	Token* getToken() const
	{
		return token;
	}

	const string& getName() const
	{
		return token->str;
	}

	~NNamedType()
	{
		delete token;
	}
};

class NBaseType : public NNamedType
{
	int type;

public:
	NBaseType(Token* token, int type)
	: NNamedType(token), type(type) {}

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
	NDataType* baseType;
	NIntConst* size;
	Token* vecToken;

public:
	NVecType(Token* vecToken, NIntConst* size, NDataType* baseType)
	: baseType(baseType), size(size), vecToken(vecToken) {}

	SType* getType(CodeContext& context);

	~NVecType()
	{
		delete baseType;
		delete size;
	}
};

class NUserType : public NNamedType
{
public:
	explicit NUserType(Token* name)
	: NNamedType(name) {}

	SType* getType(CodeContext& context);
};

class NPointerType : public NDataType
{
	NDataType* baseType;

public:
	explicit NPointerType(NDataType* baseType)
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
	Token* atTok;

public:
	NFuncPointerType(Token* atTok, NDataType* returnType, NDataTypeList* params)
	: returnType(returnType), params(params), atTok(atTok) {}

	SType* getType(CodeContext& context)
	{
		auto ptr = getType(context, atTok, returnType, params);
		return ptr? SType::getPointer(context, ptr) : nullptr;
	}

	static SFunctionType* getType(CodeContext& context, Token* atToken, NDataType* retType, NDataTypeList* params);

	~NFuncPointerType()
	{
		delete returnType;
		delete params;
		delete atTok;
	}
};

class NVariableDecl : public NDeclaration
{
protected:
	NExpression* initExp;
	NDataType* type;
	Token* eqToken;

public:
	NVariableDecl(Token* name, Token* eqToken = nullptr, NExpression* initExp = nullptr)
	: NDeclaration(name), initExp(initExp), type(nullptr), eqToken(eqToken) {}

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

	Token* getEqToken() const
	{
		return eqToken;
	}

	void genCode(CodeContext& context);

	~NVariableDecl()
	{
		delete initExp;
		delete eqToken;
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
	NGlobalVariableDecl(Token* name, Token* eqToken = nullptr, NExpression* initExp = nullptr)
	: NVariableDecl(name, eqToken, initExp) {}

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
	explicit NBaseVariable(Token* name)
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
	Token* brackTok;

public:
	NArrayVariable(NVariable* arrVar, Token* brackTok, NExpression* index)
	: arrVar(arrVar), index(index), brackTok(brackTok) {}

	RValue loadVar(CodeContext& context);

	const string& getName() const
	{
		return arrVar->getName();
	}

	~NArrayVariable()
	{
		delete arrVar;
		delete index;
		delete brackTok;
	}
};

class NMemberVariable : public NVariable
{
	NVariable* baseVar;
	Token* memberName;
	Token* dotToken;

public:
	NMemberVariable(NVariable* baseVar, Token* memberName, Token* dotToken)
	: baseVar(baseVar), memberName(memberName), dotToken(dotToken) {}

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
		delete dotToken;
	}
};

class NExprVariable : public NVariable
{
	const static string STR_TMP_EXP;

	NExpression* expr;

public:
	explicit NExprVariable(NExpression* expr)
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
	Token* atTok;

public:
	NDereference(NVariable* derefVar, Token* atTok)
	: derefVar(derefVar), atTok(atTok) {}

	RValue loadVar(CodeContext& context);

	const string& getName() const
	{
		return derefVar->getName();
	}

	~NDereference()
	{
		delete derefVar;
		delete atTok;
	}
};

class NAddressOf : public NVariable
{
	NVariable* addVar;

public:
	explicit NAddressOf(NVariable* addVar)
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
public:
	enum class CreateType {STRUCT, UNION, CLASS};

private:
	NVariableDeclGroupList* list;
	CreateType ctype;

public:
	NStructDeclaration(Token* name, NVariableDeclGroupList* list, CreateType ctype = CreateType::STRUCT)
	: NDeclaration(name), list(list), ctype(ctype) {}

	void genCode(CodeContext& context);

	~NStructDeclaration()
	{
		delete list;
	}
};

class NEnumDeclaration : public NDeclaration
{
	NVariableDeclList* variables;
	Token* lBrac;
	NDataType* baseType;

public:
	NEnumDeclaration(Token* name, NVariableDeclList* variables, Token* lBrac = nullptr, NDataType* baseType = nullptr)
	: NDeclaration(name), variables(variables), lBrac(lBrac), baseType(baseType) {}

	void genCode(CodeContext& context);

	~NEnumDeclaration()
	{
		delete variables;
		delete lBrac;
		delete baseType;
	}
};

class NFunctionDeclaration : public NDeclaration
{
	NDataType* rtype;
	NParameterList* params;
	NStatementList* body;

public:
	NFunctionDeclaration(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body)
	: NDeclaration(name), rtype(rtype), params(params), body(body) {}

	void genCode(CodeContext& context) final;

	SFunction genFunction(CodeContext& context);

	void genCodeParams(SFunction function, CodeContext& context) const;

	SFunctionType* getFunctionType(CodeContext& context, Token* token)
	{
		NDataTypeList typeList(false);
		for (auto item : *params) {
			typeList.addItem(item->getTypeNode());
		}
		return NFuncPointerType::getType(context, token, rtype, &typeList);
	}

	~NFunctionDeclaration()
	{
		delete rtype;
		delete params;
		delete body;
	}
};

// forward declaration
class NClassDeclaration;

class NClassMember : public NDeclaration
{
protected:
	NClassDeclaration* theClass;

public:
	explicit NClassMember(Token* name)
	: NDeclaration(name), theClass(nullptr) {}

	void setClass(NClassDeclaration* cl)
	{
		theClass = cl;
	}

	virtual bool isStruct() const = 0;
};
typedef NodeList<NClassMember> NClassMemberList;

class NClassDeclaration : public NDeclaration
{
	NClassMemberList* list;

public:
	NClassDeclaration(Token* name, NClassMemberList* list)
	: NDeclaration(name), list(list)
	{
		for (auto i : *list)
			i->setClass(this);
	}

	void genCode(CodeContext& context);

	~NClassDeclaration()
	{
		delete list;
	}
};

class NClassStructDecl : public NClassMember
{
	NVariableDeclGroupList* list;

public:
	NClassStructDecl(Token* name, NVariableDeclGroupList* list)
	: NClassMember(name), list(list) {}

	void genCode(CodeContext& context);

	bool isStruct() const
	{
		return true;
	}

	~NClassStructDecl()
	{
		delete list;
	}
};

class NClassFunctionDecl : public NClassMember
{
	NDataType* rtype;
	NParameterList* params;
	NStatementList* body;

public:
	NClassFunctionDecl(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body)
	: NClassMember(name), rtype(rtype), params(params), body(body) {}

	void genCode(CodeContext& context);

	bool isStruct() const
	{
		return false;
	}

	~NClassFunctionDecl()
	{
		delete rtype;
		delete params;
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
	explicit NLoopStatement(NStatementList* body)
	: NConditionStmt(nullptr, body) {}

	void genCode(CodeContext& context);
};

class NWhileStatement : public NConditionStmt
{
	Token* lparen;
	bool isDoWhile;
	bool isUntil;

public:
	NWhileStatement(Token* lparen, NExpression* condition, NStatementList* body, bool isDoWhile = false, bool isUntil = false)
	: NConditionStmt(condition, body), lparen(lparen), isDoWhile(isDoWhile), isUntil(isUntil) {}

	void genCode(CodeContext& context);
};

class NSwitchCase : public NStatement
{
	NIntConst* value;
	NStatementList* body;
	Token* token;

public:
	NSwitchCase(Token* token, NStatementList* body, NIntConst* value = nullptr)
	: value(value), body(body), token(token) {}

	void genCode(CodeContext& context)
	{
		body->genCode(context);
	}

	Token* getToken() const
	{
		return token;
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
		delete token;
	}
};
typedef NodeList<NSwitchCase> NSwitchCaseList;

class NSwitchStatement : public NStatement
{
	NExpression* value;
	NSwitchCaseList* cases;
	Token* lparen;

public:
	NSwitchStatement(Token* lparen, NExpression* value, NSwitchCaseList* cases)
	: value(value), cases(cases), lparen(lparen) {}

	void genCode(CodeContext& context);

	~NSwitchStatement()
	{
		delete value;
		delete cases;
		delete lparen;
	}
};

class NForStatement : public NConditionStmt
{
	NStatementList* preStm;
	NExpressionList* postExp;
	Token* semiCol2;

public:
	NForStatement(NStatementList* preStm, NExpression* condition, Token* semiCol2, NExpressionList* postExp, NStatementList* body)
	: NConditionStmt(condition, body), preStm(preStm), postExp(postExp), semiCol2(semiCol2) {}

	void genCode(CodeContext& context);

	~NForStatement()
	{
		delete preStm;
		delete postExp;
		delete semiCol2;
	}
};

class NIfStatement : public NConditionStmt
{
	NStatementList* elseBody;
	Token* lparen;

public:
	NIfStatement(Token* lparen, NExpression* condition, NStatementList* ifBody, NStatementList* elseBody)
	: NConditionStmt(condition, ifBody), elseBody(elseBody), lparen(lparen) {}

	void genCode(CodeContext& context);

	~NIfStatement()
	{
		delete elseBody;
		delete lparen;
	}
};

class NLabelStatement : public NDeclaration
{
public:
	explicit NLabelStatement(Token* name)
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
	Token* retToken;

public:
	NReturnStatement(Token* retToken = nullptr, NExpression* value = nullptr)
	: value(value), retToken(retToken) {}

	void genCode(CodeContext& context);

	~NReturnStatement()
	{
		delete value;
		delete retToken;
	}
};

class NGotoStatement : public NJumpStatement
{
	Token* name;

public:
	explicit NGotoStatement(Token* name)
	: name(name) {}

	void genCode(CodeContext& context);

	Token* getNameToken() const
	{
		return name;
	}

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
	Token* token;
	int type;
	NIntConst* level;

public:
	NLoopBranch(Token* token, int type, NIntConst* level = nullptr)
	: token(token), type(type), level(level) {}

	void genCode(CodeContext& context);

	~NLoopBranch()
	{
		delete token;
		delete level;
	}
};

class NDeleteStatement : public NStatement
{
	NVariable *variable;
	Token* token;

public:
	NDeleteStatement(Token* token, NVariable* variable)
	: variable(variable), token(token) {}

	void genCode(CodeContext& context);

	~NDeleteStatement()
	{
		delete variable;
		delete token;
	}
};

class NOperatorExpr : public NExpression
{
protected:
	int oper;
	Token* opTok;

public:
	NOperatorExpr(int oper, Token* opTok)
	: oper(oper), opTok(opTok) {}

	~NOperatorExpr()
	{
		delete opTok;
	}
};

class NAssignment : public NOperatorExpr
{
	NVariable* lhs;
	NExpression* rhs;

public:
	NAssignment(int oper, Token* opToken, NVariable* lhs, NExpression* rhs)
	: NOperatorExpr(oper, opToken), lhs(lhs), rhs(rhs) {}

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
	Token* colTok;

public:
	NTernaryOperator(NExpression* condition, NExpression* trueVal, Token *colTok, NExpression* falseVal)
	: condition(condition), trueVal(trueVal), falseVal(falseVal), colTok(colTok) {}

	RValue genValue(CodeContext& context);

	~NTernaryOperator()
	{
		delete condition;
		delete trueVal;
		delete falseVal;
		delete colTok;
	}
};

class NNewExpression : public NExpression
{
	NDataType* type;
	Token* token;

public:
	NNewExpression(Token* token, NDataType* type)
	: type(type), token(token) {}

	RValue genValue(CodeContext& context);

	~NNewExpression()
	{
		delete type;
		delete token;
	}
};

class NBinaryOperator : public NOperatorExpr
{
protected:
	NExpression* lhs;
	NExpression* rhs;

public:
	NBinaryOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NOperatorExpr(oper, opToken), lhs(lhs), rhs(rhs) {}

	~NBinaryOperator()
	{
		delete lhs;
		delete rhs;
	}
};

class NLogicalOperator : public NBinaryOperator
{
public:
	NLogicalOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NCompareOperator : public NBinaryOperator
{
public:
	NCompareOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NBinaryMathOperator : public NBinaryOperator
{
public:
	NBinaryMathOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NNullCoalescing : public NBinaryOperator
{
public:
	NNullCoalescing(Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(0, opToken, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NSizeOfOperator : public NExpression
{
	enum OfType { DATA, EXP, NAME };

	OfType type;
	NDataType* dtype;
	NExpression* exp;
	Token* name;
	Token* sizeTok;

public:
	NSizeOfOperator(Token* sizeTok, NDataType* dtype)
	: type(DATA), dtype(dtype), exp(nullptr), name(nullptr), sizeTok(sizeTok) {}

	NSizeOfOperator(Token* sizeTok, NExpression* exp)
	: type(EXP), dtype(nullptr), exp(exp), name(nullptr), sizeTok(sizeTok) {}

	NSizeOfOperator(Token* sizeTok, Token* name)
	: type(NAME), dtype(nullptr), exp(nullptr), name(name), sizeTok(sizeTok) {}

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
		delete sizeTok;
	}
};

class NUnaryOperator : public NOperatorExpr
{
protected:
	NExpression* unary;

public:
	NUnaryOperator(int oper, Token* opToken, NExpression* unary)
	: NOperatorExpr(oper, opToken), unary(unary) {}

	~NUnaryOperator()
	{
		delete unary;
	}
};

class NUnaryMathOperator : public NUnaryOperator
{
public:
	NUnaryMathOperator(int oper, Token* opToken, NExpression* unaryExp)
	: NUnaryOperator(oper, opToken, unaryExp) {}

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

class NIncrement : public NOperatorExpr
{
	NVariable* variable;
	bool isPostfix;

public:
	NIncrement(int oper, Token* opToken, NVariable* variable, bool isPostfix)
	: NOperatorExpr(oper, opToken), variable(variable), isPostfix(isPostfix) {}

	RValue genValue(CodeContext& context);

	~NIncrement()
	{
		delete variable;
	}
};

#endif
