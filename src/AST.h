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
#ifndef __AST_H__
#define __AST_H__

#include <vector>
#include <set>
#include <algorithm>
#include "BaseNodes.h"

using namespace std;

class NStatement : public Node
{
public:
	virtual bool isTerminator() const
	{
		return false;
	}
};

typedef NodeList<NStatement> NStatementList;

class NExpression : public Node
{
public:
	virtual operator Token*() const = 0;

	virtual bool isConstant() const
	{
		return false;
	}

	virtual bool isComplex() const
	{
		return true;
	}
};

typedef NodeList<NExpression> NExpressionList;

class NExpressionStm : public NStatement
{
	NExpression* exp;

public:
	explicit NExpressionStm(NExpression* exp)
	: exp(exp) {}

	static NStatementList* convert(NExpressionList* other)
	{
		auto ret = new NStatementList;
		for (auto item : *other)
			ret->add(new NExpressionStm(item));
		other->setDelete(false);
		delete other;
		return ret;
	}

	NExpression* getExp() const
	{
		return exp;
	}

	~NExpressionStm()
	{
		delete exp;
	}

	ADD_ID(NExpressionStm)
};

class NConstant : public NExpression
{
protected:
	Token* value;

public:
	explicit NConstant(Token* token)
	: value(token) {}

	operator Token*() const
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

	ADD_ID(NNullPointer)
};

class NStringLiteral : public NConstant
{
public:
	explicit NStringLiteral(Token* str)
	: NConstant(str)
	{
		Token::unescape(str->str);
	}

	ADD_ID(NStringLiteral)
};

class NIntLikeConst : public NConstant
{
public:
	explicit NIntLikeConst(Token* token)
	: NConstant(token) {}

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

	bool getValue() const
	{
		return bvalue;
	}

	ADD_ID(NBoolConst)
};

class NCharConst : public NIntLikeConst
{
public:
	explicit NCharConst(Token* charStr)
	: NIntLikeConst(charStr)
	{
		value->str = value->str.substr(1, value->str.length() - 2);
	}

	ADD_ID(NCharConst)
};

class NIntConst : public NIntLikeConst
{
	int base;

public:
	NIntConst(Token* value, int base = 10)
	: NIntLikeConst(value), base(base)
	{
		Token::remove(value->str);
	}

	int getBase() const
	{
		return base;
	}

	ADD_ID(NIntConst)
};

class NFloatConst : public NConstant
{
public:
	explicit NFloatConst(Token* value)
	: NConstant(value)
	{
		Token::remove(value->str);
	}

	ADD_ID(NFloatConst)
};

class NImportStm : public NStatement
{
	Token* filename;

public:
	explicit NImportStm(Token* filename)
	: filename(filename)
	{
		Token::unescape(filename->str);
	}

	operator Token*() const
	{
		return getName();
	}

	Token* getName() const
	{
		return filename;
	}

	~NImportStm()
	{
		delete filename;
	}

	ADD_ID(NImportStm)
};

class NDeclaration : public NStatement
{
protected:
	Token* name;

public:
	explicit NDeclaration(Token* name)
	: name(name) {}

	Token* getName() const
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
public:
	virtual operator Token*() const = 0;
};
typedef NodeList<NDataType> NDataTypeList;

class NNamedType : public NDataType
{
protected:
	Token* token;

public:
	explicit NNamedType(Token* token)
	: token(token) {}

	operator Token*() const
	{
		return getName();
	}

	Token* getName() const
	{
		return token;
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

	int getType() const
	{
		return type;
	}

	ADD_ID(NBaseType)
};

class NConstType : public NDataType
{
	Token* constTok;
	NDataType* type;

public:
	NConstType(Token* constTok, NDataType* type)
	: constTok(constTok), type(type) {}

	operator Token*() const
	{
		return constTok;
	}

	NDataType* getType() const
	{
		return type;
	}

	~NConstType()
	{
		delete constTok;
		delete type;
	}

	ADD_ID(NConstType);
};

class NThisType : public NNamedType
{
public:
	explicit NThisType(Token* token)
	: NNamedType(token) {}

	ADD_ID(NThisType);
};

class NArrayType : public NDataType
{
	Token* lBrac;
	NDataType* baseType;
	NExpression* size;

public:
	NArrayType(Token* lBrac, NDataType* baseType, NExpression* size = nullptr)
	: lBrac(lBrac), baseType(baseType), size(size) {}

	operator Token*() const
	{
		return lBrac;
	}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	NExpression* getSize() const
	{
		return size;
	}

	~NArrayType()
	{
		delete baseType;
		delete size;
		delete lBrac;
	}

	ADD_ID(NArrayType)
};

class NVecType : public NDataType
{
	NDataType* baseType;
	NIntConst* size;
	Token* vecToken;

public:
	NVecType(Token* vecToken, NIntConst* size, NDataType* baseType)
	: baseType(baseType), size(size), vecToken(vecToken) {}

	NIntConst* getSize() const
	{
		return size;
	}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	operator Token*() const
	{
		return vecToken;
	}

	~NVecType()
	{
		delete baseType;
		delete size;
	}

	ADD_ID(NVecType)
};

class NUserType : public NNamedType
{
public:
	explicit NUserType(Token* name)
	: NNamedType(name) {}

	ADD_ID(NUserType)
};

class NPointerType : public NDataType
{
	NDataType* baseType;
	Token* atTok;

public:
	explicit NPointerType(NDataType* baseType, Token* atTok = nullptr)
	: baseType(baseType), atTok(atTok) {}

	operator Token*() const
	{
		return atTok;
	}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	~NPointerType()
	{
		delete baseType;
		delete atTok;
	}

	ADD_ID(NPointerType)
};

class NFuncPointerType : public NDataType
{
	NDataType* returnType;
	NDataTypeList* params;
	Token* atTok;

public:
	NFuncPointerType(Token* atTok, NDataType* returnType, NDataTypeList* params)
	: returnType(returnType), params(params), atTok(atTok) {}

	operator Token*() const
	{
		return atTok;
	}

	NDataType* getReturnType() const
	{
		return returnType;
	}

	NDataTypeList* getParams() const
	{
		return params;
	}

	~NFuncPointerType()
	{
		delete returnType;
		delete params;
		delete atTok;
	}

	ADD_ID(NFuncPointerType)
};

class NVariableDecl : public NDeclaration
{
protected:
	NExpression* initExp;
	NExpressionList* initList;
	NDataType* type;

public:
	NVariableDecl(Token* name, NExpression* initExp = nullptr)
	: NDeclaration(name), initExp(initExp), initList(nullptr), type(nullptr) {}

	NVariableDecl(Token* name, NExpressionList* initList)
	: NDeclaration(name), initExp(nullptr), initList(initList), type(nullptr) {}

	NDataType* getType() const
	{
		return type;
	}

	// NOTE: must be called before genCode()
	void setDataType(NDataType* qtype)
	{
		type = qtype;
	}

	bool hasInit() const
	{
		return initExp || initList;
	}

	NExpression* getInitExp() const
	{
		return initExp;
	}

	NExpressionList* getInitList() const
	{
		return initList;
	}

	~NVariableDecl()
	{
		delete initExp;
		delete initList;
	}

	ADD_ID(NVariableDecl)
};

typedef NodeList<NVariableDecl> NVariableDeclList;

class NGlobalVariableDecl : public NVariableDecl
{
public:
	NGlobalVariableDecl(Token* name, NExpression* initExp = nullptr)
	: NVariableDecl(name, initExp) {}

	ADD_ID(NGlobalVariableDecl)
};

class NVariable : public NExpression {};

class NBaseVariable : public NVariable
{
	Token* name;

public:
	explicit NBaseVariable(Token* name)
	: name(name) {}

	operator Token*() const
	{
		return getName();
	}

	Token* getName() const
	{
		return name;
	}

	bool isComplex() const
	{
		return false;
	}

	~NBaseVariable()
	{
		delete name;
	}

	ADD_ID(NBaseVariable)
};

class NArrayVariable : public NVariable
{
	NVariable* arrVar;
	NExpression* index;
	Token* brackTok;

public:
	NArrayVariable(NVariable* arrVar, Token* brackTok, NExpression* index)
	: arrVar(arrVar), index(index), brackTok(brackTok) {}

	NExpression* getIndex() const
	{
		return index;
	}

	operator Token*() const
	{
		return brackTok;
	}

	NVariable* getArrayVar() const
	{
		return arrVar;
	}

	~NArrayVariable()
	{
		delete arrVar;
		delete index;
		delete brackTok;
	}

	ADD_ID(NArrayVariable)
};

class NMemberVariable : public NVariable
{
	NVariable* baseVar;
	Token* memberName;

public:
	NMemberVariable(NVariable* baseVar, Token* memberName)
	: baseVar(baseVar), memberName(memberName) {}

	NVariable* getBaseVar() const
	{
		return baseVar;
	}

	operator Token*() const
	{
		return getMemberName();
	}

	Token* getMemberName() const
	{
		return memberName;
	}

	~NMemberVariable()
	{
		delete baseVar;
		delete memberName;
	}

	ADD_ID(NMemberVariable)
};

class NExprVariable : public NVariable
{
	NExpression* expr;

public:
	explicit NExprVariable(NExpression* expr)
	: expr(expr) {}

	operator Token*() const
	{
		return *expr;
	}

	NExpression* getExp() const
	{
		return expr;
	}

	~NExprVariable()
	{
		delete expr;
	}

	ADD_ID(NExprVariable)
};

class NDereference : public NVariable
{
	NVariable* derefVar;
	Token* atTok;

public:
	NDereference(NVariable* derefVar, Token* atTok)
	: derefVar(derefVar), atTok(atTok) {}

	NVariable* getVar() const
	{
		return derefVar;
	}

	operator Token*() const
	{
		return atTok;
	}

	~NDereference()
	{
		delete derefVar;
		delete atTok;
	}

	ADD_ID(NDereference)
};

class NAddressOf : public NVariable
{
	NVariable* addVar;
	Token* token;

public:
	explicit NAddressOf(NVariable* addVar, Token* token)
	: addVar(addVar), token(token) {}

	NVariable* getVar() const
	{
		return addVar;
	}

	operator Token*() const
	{
		return token;
	}

	~NAddressOf()
	{
		delete addVar;
	}

	ADD_ID(NAddressOf)
};

class NParameter : public NDeclaration
{
	NDataType* type;

public:
	NParameter(NDataType* type, Token* name)
	: NDeclaration(name), type(type) {}

	NDataType* getType() const
	{
		return type;
	}

	~NParameter()
	{
		delete type;
	}

	ADD_ID(NParameter)
};
typedef NodeList<NParameter> NParameterList;

class NVariableDeclGroup : public NStatement
{
	NDataType* type;
	NVariableDeclList* variables;

public:
	NVariableDeclGroup(NDataType* type, NVariableDeclList* variables)
	: type(type), variables(variables) {}

	NDataType* getType() const
	{
		return type;
	}

	NVariableDeclList* getVars() const
	{
		return variables;
	}

	~NVariableDeclGroup()
	{
		delete variables;
		delete type;
	}

	ADD_ID(NVariableDeclGroup)
};
typedef NodeList<NVariableDeclGroup> NVariableDeclGroupList;

class NAliasDeclaration : public NDeclaration
{
	NDataType* type;

public:
	NAliasDeclaration(Token* name, NDataType* type)
	: NDeclaration(name), type(type) {}

	NDataType* getType() const
	{
		return type;
	}

	~NAliasDeclaration()
	{
		delete type;
	}

	ADD_ID(NAliasDeclaration)
};

class NStructDeclaration : public NDeclaration
{
public:
	enum class CreateType {STRUCT, UNION, CLASS};

private:
	NVariableDeclGroupList* list;
	NAttributeList* attrs;
	CreateType ctype;

public:
	NStructDeclaration(Token* name, NVariableDeclGroupList* list, NAttributeList* attrs, CreateType ctype)
	: NDeclaration(name), list(list), attrs(attrs), ctype(ctype) {}

	CreateType getType() const
	{
		return ctype;
	}

	NVariableDeclGroupList* getVars() const
	{
		return list;
	}

	NAttributeList* getAttrs() const
	{
		return attrs;
	}

	~NStructDeclaration()
	{
		delete list;
		delete attrs;
	}

	ADD_ID(NStructDeclaration)
};

class NEnumDeclaration : public NDeclaration
{
	NVariableDeclList* variables;
	NDataType* baseType;

public:
	NEnumDeclaration(Token* name, NVariableDeclList* variables, NDataType* baseType = nullptr)
	: NDeclaration(name), variables(variables), baseType(baseType) {}

	NVariableDeclList* getVarList() const
	{
		return variables;
	}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	~NEnumDeclaration()
	{
		delete variables;
		delete baseType;
	}

	ADD_ID(NEnumDeclaration)
};

class NFunctionDeclaration : public NDeclaration
{
	NDataType* rtype;
	NParameterList* params;
	NStatementList* body;
	NAttributeList* attrs;

public:
	NFunctionDeclaration(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body, NAttributeList* attrs)
	: NDeclaration(name), rtype(rtype), params(params), body(body), attrs(attrs) {}

	NDataType* getRType() const
	{
		return rtype;
	}

	NParameterList* getParams() const
	{
		return params;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	NAttributeList* getAttrs() const
	{
		return attrs;
	}

	~NFunctionDeclaration()
	{
		delete rtype;
		delete params;
		delete body;
		delete attrs;
	}

	ADD_ID(NFunctionDeclaration)
};

// forward declaration
class NClassDeclaration;

class NClassMember : public NDeclaration
{
protected:
	NClassDeclaration* theClass;

public:
	enum class MemberType { CONSTRUCTOR, DESTRUCTOR, STRUCT, FUNCTION };

	explicit NClassMember(Token* name)
	: NDeclaration(name), theClass(nullptr) {}

	void setClass(NClassDeclaration* cl)
	{
		theClass = cl;
	}

	NClassDeclaration* getClass() const
	{
		return theClass;
	}

	virtual MemberType memberType() const = 0;
};
typedef NodeList<NClassMember> NClassMemberList;

class NMemberInitializer : public NStatement
{
	Token* name;
	NExpressionList *expression;

public:
	NMemberInitializer(Token* name, NExpressionList* expression)
	: name(name), expression(expression) {}

	Token* getName() const
	{
		return name;
	}

	NExpressionList* getExp() const
	{
		return expression;
	}

	~NMemberInitializer()
	{
		delete name;
		delete expression;
	}

	ADD_ID(NMemberInitializer)
};
typedef NodeList<NMemberInitializer> NInitializerList;

class NClassDeclaration : public NDeclaration
{
	NClassMemberList* list;
	NAttributeList* attrs;

public:
	NClassDeclaration(Token* name, NClassMemberList* list, NAttributeList* attrs)
	: NDeclaration(name), list(list), attrs(attrs)
	{
		for (auto i : *list)
			i->setClass(this);
	}

	NClassMemberList* getList() const
	{
		return list;
	}

	NAttributeList* getAttrs() const
	{
		return attrs;
	}

	~NClassDeclaration()
	{
		delete list;
		delete attrs;
	}

	ADD_ID(NClassDeclaration)
};

class NClassStructDecl : public NClassMember
{
	NVariableDeclGroupList* list;

public:
	NClassStructDecl(Token* name, NVariableDeclGroupList* list)
	: NClassMember(name), list(list) {}

	MemberType memberType() const
	{
		return MemberType::STRUCT;
	}

	NVariableDeclGroupList* getVarList() const
	{
		return list;
	}

	~NClassStructDecl()
	{
		delete list;
	}

	ADD_ID(NClassStructDecl)
};

class NClassFunctionDecl : public NClassMember
{
	NDataType* rtype;
	NParameterList* params;
	NStatementList* body;
	NAttributeList* attrs;

public:
	NClassFunctionDecl(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body, NAttributeList* attrs = nullptr)
	: NClassMember(name), rtype(rtype), params(params), body(body), attrs(attrs) {}

	MemberType memberType() const
	{
		return MemberType::FUNCTION;
	}

	NParameterList* getParams() const
	{
		return params;
	}

	NDataType* getRType() const
	{
		return rtype;
	}

	void setRType(NDataType* type)
	{
		delete rtype;
		rtype = type;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	void setBody(NStatementList* other)
	{
		delete body;
		body = other;
	}

	NAttributeList* getAttrs() const
	{
		return attrs;
	}

	~NClassFunctionDecl()
	{
		delete rtype;
		delete params;
		delete body;
		delete attrs;
	}

	ADD_ID(NClassFunctionDecl)
};

class NClassConstructor : public NClassFunctionDecl
{
	NInitializerList* initList;

public:
	NClassConstructor(Token* name, NParameterList* params, NInitializerList* initList, NStatementList* body)
	: NClassFunctionDecl(name, nullptr, params, body), initList(initList) {}

	MemberType memberType() const
	{
		return MemberType::CONSTRUCTOR;
	}

	NInitializerList* getInitList() const
	{
		return initList;
	}

	~NClassConstructor()
	{
		delete initList;
	}

	ADD_ID(NClassConstructor)
};

class NClassDestructor : public NClassFunctionDecl
{
public:
	NClassDestructor(Token* name, NStatementList* body)
	: NClassFunctionDecl(name, nullptr, new NParameterList, body) {}

	MemberType memberType() const
	{
		return MemberType::DESTRUCTOR;
	}

	ADD_ID(NClassDestructor)
};

class NConditionStmt : public NStatement
{
protected:
	NExpression* condition;
	NStatementList* body;

public:
	NConditionStmt(NExpression* condition, NStatementList* body)
	: condition(condition), body(body) {}

	NExpression* getCond() const
	{
		return condition;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	~NConditionStmt()
	{
		delete condition;
		delete body;
	}

	ADD_ID(NConditionStmt)
};

class NLoopStatement : public NConditionStmt
{
public:
	explicit NLoopStatement(NStatementList* body)
	: NConditionStmt(nullptr, body) {}

	ADD_ID(NLoopStatement)
};

class NWhileStatement : public NConditionStmt
{
	bool isDoWhile;
	bool isUntil;

public:
	NWhileStatement(NExpression* condition, NStatementList* body, bool isDoWhile = false, bool isUntil = false)
	: NConditionStmt(condition, body), isDoWhile(isDoWhile), isUntil(isUntil) {}

	bool doWhile() const
	{
		return isDoWhile;
	}

	bool until() const
	{
		return isUntil;
	}

	ADD_ID(NWhileStatement)
};

class NSwitchCase : public NStatement
{
	NIntConst* value;
	NStatementList* body;
	Token* token;

public:
	NSwitchCase(Token* token, NStatementList* body, NIntConst* value = nullptr)
	: value(value), body(body), token(token) {}

	NIntConst* getValue() const
	{
		return value;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	operator Token*() const
	{
		return token;
	}

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

	ADD_ID(NSwitchCase)
};
typedef NodeList<NSwitchCase> NSwitchCaseList;

class NSwitchStatement : public NStatement
{
	NExpression* value;
	NSwitchCaseList* cases;

public:
	NSwitchStatement(NExpression* value, NSwitchCaseList* cases)
	: value(value), cases(cases) {}

	NExpression* getValue() const
	{
		return value;
	}

	NSwitchCaseList* getCases() const
	{
		return cases;
	}

	~NSwitchStatement()
	{
		delete value;
		delete cases;
	}

	ADD_ID(NSwitchStatement)
};

class NForStatement : public NConditionStmt
{
	NStatementList* preStm;
	NExpressionList* postExp;

public:
	NForStatement(NStatementList* preStm, NExpression* condition, NExpressionList* postExp, NStatementList* body)
	: NConditionStmt(condition, body), preStm(preStm), postExp(postExp) {}

	NStatementList* getPreStm() const
	{
		return preStm;
	}

	NExpressionList* getPostExp() const
	{
		return postExp;
	}

	~NForStatement()
	{
		delete preStm;
		delete postExp;
	}

	ADD_ID(NForStatement)
};

class NIfStatement : public NConditionStmt
{
	NStatementList* elseBody;

public:
	NIfStatement(NExpression* condition, NStatementList* ifBody, NStatementList* elseBody)
	: NConditionStmt(condition, ifBody), elseBody(elseBody) {}

	NStatementList* getElseBody() const
	{
		return elseBody;
	}

	~NIfStatement()
	{
		delete elseBody;
	}

	ADD_ID(NIfStatement)
};

class NLabelStatement : public NDeclaration
{
public:
	explicit NLabelStatement(Token* name)
	: NDeclaration(name) {}

	ADD_ID(NLabelStatement)
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

	NExpression* getValue() const
	{
		return value;
	}

	operator Token*() const
	{
		return retToken;
	}

	~NReturnStatement()
	{
		delete value;
		delete retToken;
	}

	ADD_ID(NReturnStatement)
};

class NGotoStatement : public NJumpStatement
{
	Token* name;

public:
	explicit NGotoStatement(Token* name)
	: name(name) {}

	Token* getName() const
	{
		return name;
	}

	~NGotoStatement()
	{
		delete name;
	}

	ADD_ID(NGotoStatement)
};

class NLoopBranch : public NJumpStatement
{
	Token* token;
	int type;
	NIntConst* level;

public:
	NLoopBranch(Token* token, int type, NIntConst* level = nullptr)
	: token(token), type(type), level(level) {}

	operator Token*() const
	{
		return token;
	}

	int getType() const
	{
		return type;
	}

	NIntConst* getLevel() const
	{
		return level;
	}

	~NLoopBranch()
	{
		delete token;
		delete level;
	}

	ADD_ID(NLoopBranch)
};

class NDeleteStatement : public NStatement
{
	NVariable *variable;

public:
	explicit NDeleteStatement(NVariable* variable)
	: variable(variable) {}

	NVariable* getVar() const
	{
		return variable;
	}

	~NDeleteStatement()
	{
		delete variable;
	}

	ADD_ID(NDeleteStatement)
};

class NDestructorCall : public NStatement
{
	NVariable* baseVar;
	Token* thisToken;

public:
	NDestructorCall(NVariable* baseVar, Token* thisToken)
	: baseVar(baseVar), thisToken(thisToken) {}

	NVariable* getVar() const
	{
		return baseVar;
	}

	Token* getThisToken() const
	{
		return thisToken;
	}

	~NDestructorCall()
	{
		delete baseVar;
		delete thisToken;
	}

	ADD_ID(NDestructorCall)
};

class NOperatorExpr : public NExpression
{
protected:
	int oper;
	Token* opTok;

public:
	NOperatorExpr(int oper, Token* opTok)
	: oper(oper), opTok(opTok) {}

	int getOp() const
	{
		return oper;
	}

	operator Token*() const
	{
		return opTok;
	}

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

	NVariable* getLhs() const
	{
		return lhs;
	}

	NExpression* getRhs() const
	{
		return rhs;
	}

	~NAssignment()
	{
		delete lhs;
		delete rhs;
	}

	ADD_ID(NAssignment)
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

	NExpression* getCondition() const
	{
		return condition;
	}

	NExpression* getTrueVal() const
	{
		return trueVal;
	}

	NExpression* getFalseVal() const
	{
		return falseVal;
	}

	operator Token*() const
	{
		return colTok;
	}

	~NTernaryOperator()
	{
		delete condition;
		delete trueVal;
		delete falseVal;
		delete colTok;
	}

	ADD_ID(NTernaryOperator)
};

class NNewExpression : public NExpression
{
	NDataType* type;
	Token* token;
	NExpressionList* args;

public:
	NNewExpression(Token* token, NDataType* type, NExpressionList* args = nullptr)
	: type(type), token(token), args(args) {}

	NDataType* getType() const
	{
		return type;
	}

	NExpressionList* getArgs() const
	{
		return args;
	}

	operator Token*() const
	{
		return token;
	}

	~NNewExpression()
	{
		delete type;
		delete token;
		delete args;
	}

	ADD_ID(NNewExpression)
};

class NBinaryOperator : public NOperatorExpr
{
protected:
	NExpression* lhs;
	NExpression* rhs;

public:
	NBinaryOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NOperatorExpr(oper, opToken), lhs(lhs), rhs(rhs) {}

	NExpression* getLhs() const
	{
		return lhs;
	}

	NExpression* getRhs() const
	{
		return rhs;
	}

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

	ADD_ID(NLogicalOperator)
};

class NCompareOperator : public NBinaryOperator
{
public:
	NCompareOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	ADD_ID(NCompareOperator)
};

class NBinaryMathOperator : public NBinaryOperator
{
public:
	NBinaryMathOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	ADD_ID(NBinaryMathOperator)
};

class NNullCoalescing : public NBinaryOperator
{
public:
	NNullCoalescing(Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(0, opToken, lhs, rhs) {}

	ADD_ID(NNullCoalescing)
};

class NArrowOperator : public NVariable
{
public:
	enum OfType { DATA, EXP };

private:
	OfType type;
	NDataType* dtype;
	NExpression* exp;
	Token* name;
	NDataType* arg;

public:
	NArrowOperator(NDataType* dtype, Token* name, NDataType* arg)
	: type(DATA), dtype(dtype), exp(nullptr), name(name), arg(arg) {}

	NArrowOperator(NExpression* exp, Token* name, NDataType* arg)
	: type(EXP), dtype(nullptr), exp(exp), name(name), arg(arg) {}

	OfType getType() const
	{
		return type;
	}

	NExpression* getExp() const
	{
		return exp;
	}

	NDataType* getDataType() const
	{
		return dtype;
	}

	Token* getName() const
	{
		return name;
	}

	NDataType* getArg() const
	{
		return arg;
	}

	operator Token*() const
	{
		switch (type) {
		default:
		case DATA: return *dtype;
		case EXP: return *exp;
		}
	}

	bool isConstant() const
	{
		return type == EXP? exp->isConstant() : true;
	}

	~NArrowOperator()
	{
		delete dtype;
		delete exp;
		delete name;
		delete arg;
	}

	ADD_ID(NArrowOperator)
};

class NUnaryOperator : public NOperatorExpr
{
protected:
	NExpression* unary;

public:
	NUnaryOperator(int oper, Token* opToken, NExpression* unary)
	: NOperatorExpr(oper, opToken), unary(unary) {}

	NExpression* getExp() const
	{
		return unary;
	}

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

	ADD_ID(NUnaryMathOperator)
};

class NFunctionCall : public NVariable
{
	Token* name;
	NExpressionList* arguments;

public:
	NFunctionCall(Token* name, NExpressionList* arguments)
	: name(name), arguments(arguments) {}

	operator Token*() const
	{
		return getName();
	}

	NExpressionList* getArguments() const
	{
		return arguments;
	}

	Token* getName() const
	{
		return name;
	}

	~NFunctionCall()
	{
		delete name;
		delete arguments;
	}

	ADD_ID(NFunctionCall)
};

class NMemberFunctionCall : public NVariable
{
	NVariable* baseVar;
	Token* funcName;
	NExpressionList* arguments;

public:
	NMemberFunctionCall(NVariable* baseVar, Token* funcName, NExpressionList* arguments)
	: baseVar(baseVar), funcName(funcName), arguments(arguments) {}

	NVariable* getBaseVar() const
	{
		return baseVar;
	}

	operator Token*() const
	{
		return getName();
	}

	Token* getName() const
	{
		return funcName;
	}

	NExpressionList* getArguments() const
	{
		return arguments;
	}

	~NMemberFunctionCall()
	{
		delete baseVar;
		delete funcName;
		delete arguments;
	}

	ADD_ID(NMemberFunctionCall)
};

class NIncrement : public NOperatorExpr
{
	NVariable* variable;
	bool isPostfix;

public:
	NIncrement(int oper, Token* opToken, NVariable* variable, bool isPostfix)
	: NOperatorExpr(oper, opToken), variable(variable), isPostfix(isPostfix) {}

	NVariable* getVar() const
	{
		return variable;
	}

	bool postfix() const
	{
		return isPostfix;
	}

	~NIncrement()
	{
		delete variable;
	}

	ADD_ID(NIncrement)
};

#endif
