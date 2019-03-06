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

	virtual bool isBlockStmt() const
	{
		return false;
	}

	virtual NStatement* copy() const override = 0;
};

typedef NodeList<NStatement> NStatementList;

class NExpression : public Node
{
public:
	virtual operator Token*() const = 0;

	virtual NExpression* copy() const override = 0;

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

	NExpressionStm* copy() const override
	{
		return new NExpressionStm(exp->copy());
	}

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
	string strVal;

public:
	explicit NConstant(Token* token, const string& strVal)
	: value(token), strVal(strVal) {}

	operator Token*() const override
	{
		return value;
	}

	bool isComplex() const override
	{
		return false;
	}

	const string& getStrVal() const
	{
		return strVal;
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
	: NConstant(token, token->str) {}

	NNullPointer* copy() const override
	{
		return new NNullPointer(value->copy());
	}

	ADD_ID(NNullPointer)
};

class NStringLiteral : public NConstant
{
public:
	explicit NStringLiteral(Token* str)
	: NConstant(str, str->str)
	{
		Token::unescape(strVal);
	}

	NStringLiteral(const NStringLiteral& other)
	: NConstant(other.value->copy(), other.strVal)
	{
	}

	NStringLiteral* copy() const override
	{
		return new NStringLiteral(*this);
	}

	ADD_ID(NStringLiteral)
};

class NIntLikeConst : public NConstant
{
public:
	explicit NIntLikeConst(Token* token)
	: NConstant(token, token->str) {}
};

class NBoolConst : public NIntLikeConst
{
	bool bvalue;

public:
	NBoolConst(Token* token, bool value)
	: NIntLikeConst(token), bvalue(value) {}

	NBoolConst* copy() const override
	{
		return new NBoolConst(value->copy(), bvalue);
	}

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
		strVal = value->str.substr(1, value->str.length() - 2);
	}

	NCharConst(const NCharConst& other)
	: NIntLikeConst(other.value->copy())
	{
		strVal = other.strVal;
	}

	NCharConst* copy() const override
	{
		return new NCharConst(*this);
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
		Token::remove(strVal);
	}

	NIntConst(const NIntConst& other)
	: NIntLikeConst(other.value->copy()), base(other.base)
	{
		strVal = other.strVal;
	}

	NIntConst* copy() const override
	{
		return new NIntConst(*this);
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
	: NConstant(value, value->str)
	{
		Token::remove(strVal);
	}

	NFloatConst(const NFloatConst& other)
	: NConstant(other.value->copy(), other.strVal)
	{
	}

	NFloatConst* copy() const override
	{
		return new NFloatConst(*this);
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

	NImportStm(const NImportStm& other)
	: filename(other.filename->copy())
	{
	}

	NImportStm* copy() const override
	{
		return new NImportStm(*this);
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

	virtual NDataType* copy() const override = 0;
};
typedef NodeList<NDataType> NDataTypeList;

class NNamedType : public NDataType
{
protected:
	Token* token;

public:
	explicit NNamedType(Token* token)
	: token(token) {}

	operator Token*() const override
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

	NBaseType* copy() const override
	{
		return new NBaseType(token->copy(), type);
	}

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

	NConstType* copy() const override
	{
		return new NConstType(constTok->copy(), type->copy());
	}

	operator Token*() const override
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

	NThisType* copy() const override
	{
		return new NThisType(token->copy());
	}

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

	NArrayType* copy() const override
	{
		auto sz = size ? size->copy() : nullptr;
		return new NArrayType(lBrac->copy(), baseType->copy(), sz);
	}

	operator Token*() const override
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
	NExpression* size;
	Token* vecToken;

public:
	NVecType(Token* vecToken, NExpression* size, NDataType* baseType)
	: baseType(baseType), size(size), vecToken(vecToken) {}

	NVecType* copy() const override
	{
		return new NVecType(vecToken->copy(), size->copy(), baseType->copy());
	}

	NExpression* getSize() const
	{
		return size;
	}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	operator Token*() const override
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
	NDataTypeList* templateArgs;

public:
	explicit NUserType(Token* name, NDataTypeList* templateArgs = nullptr)
	: NNamedType(name), templateArgs(templateArgs) {}

	NUserType* copy() const override
	{
		auto args = templateArgs ? templateArgs->copy() : nullptr;
		return new NUserType(token->copy(), args);
	}

	NDataTypeList* getTemplateArgs()
	{
		return templateArgs;
	}

	~NUserType()
	{
		delete templateArgs;
	}

	ADD_ID(NUserType)
};

class NPointerType : public NDataType
{
	NDataType* baseType;
	Token* atTok;

public:
	explicit NPointerType(NDataType* baseType, Token* atTok = nullptr)
	: baseType(baseType), atTok(atTok) {}

	NPointerType* copy() const override
	{
		auto at = atTok ? atTok->copy() : nullptr;
		return new NPointerType(baseType->copy(), at);
	}

	operator Token*() const override
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

	NFuncPointerType* copy() const override
	{
		return new NFuncPointerType(atTok->copy(), returnType->copy(), params->copy());
	}

	operator Token*() const override
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

	NVariableDecl(const NVariableDecl& other)
	: NDeclaration(other.name->copy())
	{
		initExp = other.initExp ? other.initExp->copy() : nullptr;
		initList = other.initList ? other.initList->copy() : nullptr;
		type = other.type ? other.type->copy() : nullptr;
	}

	NVariableDecl* copy() const override
	{
		return new NVariableDecl(*this);
	}

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

	NGlobalVariableDecl* copy() const override
	{
		auto in = initExp ? initExp-> copy() : nullptr;
		return new NGlobalVariableDecl(name->copy(), in);
	}

	ADD_ID(NGlobalVariableDecl)
};

class NVariable : public NExpression
{
public:
	virtual NVariable* copy() const override = 0;
};

class NBaseVariable : public NVariable
{
	Token* name;

public:
	explicit NBaseVariable(Token* name)
	: name(name) {}

	NBaseVariable* copy() const override
	{
		return new NBaseVariable(name->copy());
	}

	operator Token*() const override
	{
		return getName();
	}

	Token* getName() const
	{
		return name;
	}

	bool isComplex() const override
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

	NArrayVariable* copy() const override
	{
		return new NArrayVariable(arrVar->copy(), brackTok->copy(), index->copy());
	}

	NExpression* getIndex() const
	{
		return index;
	}

	operator Token*() const override
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

	NMemberVariable* copy() const override
	{
		return new NMemberVariable(baseVar->copy(), memberName->copy());
	}

	NVariable* getBaseVar() const
	{
		return baseVar;
	}

	operator Token*() const override
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

	NExprVariable* copy() const override
	{
		return new NExprVariable(expr->copy());
	}

	operator Token*() const override
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

	NDereference* copy() const override
	{
		return new NDereference(derefVar->copy(), atTok->copy());
	}

	NVariable* getVar() const
	{
		return derefVar;
	}

	operator Token*() const override
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

	NAddressOf* copy() const override
	{
		return new NAddressOf(addVar->copy(), token->copy());
	}

	NVariable* getVar() const
	{
		return addVar;
	}

	operator Token*() const override
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

	NParameter* copy() const override
	{
		return new NParameter(type->copy(), name->copy());
	}

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

	NVariableDeclGroup* copy() const override
	{
		return new NVariableDeclGroup(type->copy(), variables->copy());
	}

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

	NAliasDeclaration* copy() const override
	{
		return new NAliasDeclaration(name->copy(), type->copy());
	}

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

class NTemplatedDeclaration : public NDeclaration
{
protected:
	NIdentifierList* templateParams;
	NAttributeList* attrs;

public:
	NTemplatedDeclaration(Token* name, NIdentifierList* templateParams, NAttributeList* attrs)
	: NDeclaration(name), templateParams(templateParams), attrs(attrs) {}

	NTemplatedDeclaration* copy() const = 0;

	NIdentifierList* getTemplateParams() const
	{
		return templateParams;
	}

	NAttributeList* getAttrs() const
	{
		return attrs;
	}

	~NTemplatedDeclaration()
	{
		delete templateParams;
		delete attrs;
	}
};

class NStructDeclaration : public NTemplatedDeclaration
{
public:
	enum class CreateType {STRUCT, UNION, CLASS};

private:
	CreateType ctype;
	NVariableDeclGroupList* list;

public:
	NStructDeclaration(Token* name, CreateType ctype, NVariableDeclGroupList* list = nullptr, NIdentifierList* templateParams = nullptr, NAttributeList* attrs = nullptr)
	: NTemplatedDeclaration(name, templateParams, attrs), ctype(ctype), list(list) {}

	NStructDeclaration* copy() const override
	{
		auto ls = list ? list->copy() : nullptr;
		auto tp = templateParams ? templateParams->copy() : nullptr;
		auto at = attrs ? attrs->copy() : nullptr;
		return new NStructDeclaration(name->copy(), ctype, ls, tp, at);
	}

	CreateType getType() const
	{
		return ctype;
	}

	NVariableDeclGroupList* getVars() const
	{
		return list;
	}

	~NStructDeclaration()
	{
		delete list;
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

	NEnumDeclaration* copy() const override
	{
		auto bt = baseType ? baseType->copy() : nullptr;
		return new NEnumDeclaration(name->copy(), variables->copy(), bt);
	}

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
	NFunctionDeclaration(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body = nullptr, NAttributeList* attrs = nullptr)
	: NDeclaration(name), rtype(rtype), params(params), body(body), attrs(attrs) {}

	NFunctionDeclaration* copy() const override
	{
		auto bd = body ? body->copy() : nullptr;
		auto at = attrs ? attrs->copy() : nullptr;
		return new NFunctionDeclaration(name->copy(), rtype->copy(), params->copy(), bd, at);
	}

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

	virtual NClassMember* copy() const = 0;
};
typedef NodeList<NClassMember> NClassMemberList;

class NMemberInitializer : public NStatement
{
	Token* name;
	NExpressionList *expression;

public:
	NMemberInitializer(Token* name, NExpressionList* expression)
	: name(name), expression(expression) {}

	NMemberInitializer* copy() const override
	{
		return new NMemberInitializer(name->copy(), expression->copy());
	}

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

class NClassDeclaration : public NTemplatedDeclaration
{
	NClassMemberList* list;

public:
	NClassDeclaration(Token* name, NClassMemberList* list = nullptr, NIdentifierList* templateParams = nullptr, NAttributeList* attrs = nullptr)
	: NTemplatedDeclaration(name, templateParams, attrs), list(list)
	{
		if (list) {
			for (auto i : *list)
				i->setClass(this);
		}
	}

	NClassDeclaration* copy() const override
	{
		auto ls = list ? list->copy() : nullptr;
		auto tp = templateParams ? templateParams->copy() : nullptr;
		auto at = attrs ? attrs->copy() : nullptr;
		return new NClassDeclaration(name->copy(), ls, tp, at);
	}

	NClassMemberList* getMembers() const
	{
		return list;
	}

	void setMembers(NClassMemberList* members)
	{
		if (list)
			delete list;
		list = members;
	}

	~NClassDeclaration()
	{
		delete list;
	}

	ADD_ID(NClassDeclaration)
};

class NClassStructDecl : public NClassMember
{
	NVariableDeclGroupList* list;

public:
	NClassStructDecl(Token* name, NVariableDeclGroupList* list)
	: NClassMember(name), list(list) {}

	NClassStructDecl* copy() const override
	{
		return new NClassStructDecl(name->copy(), list->copy());
	}

	MemberType memberType() const override
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
protected:
	NDataType* rtype;
	NParameterList* params;
	NStatementList* body;
	NAttributeList* attrs;

public:
	NClassFunctionDecl(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body, NAttributeList* attrs = nullptr)
	: NClassMember(name), rtype(rtype), params(params), body(body), attrs(attrs) {}

	NClassFunctionDecl* copy() const override
	{
		auto at = attrs ? attrs->copy() : nullptr;
		return new NClassFunctionDecl(name->copy(), rtype->copy(), params->copy(), body->copy(), at);
	}

	MemberType memberType() const override
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
	NClassConstructor(Token* name,  NParameterList* params, NInitializerList* initList, NStatementList* body, NAttributeList* attrs = nullptr)
	: NClassFunctionDecl(name, nullptr, params, body, attrs), initList(initList) {}

	NClassConstructor* copy() const override
	{
		return new NClassConstructor(name->copy(), params->copy(), initList->copy(), body->copy(), attrs ? attrs->copy() : nullptr);
	}

	MemberType memberType() const override
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

	NClassDestructor* copy() const override
	{
		return new NClassDestructor(name->copy(), body->copy());
	}

	MemberType memberType() const override
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

	NConditionStmt* copy() const override
	{
		return new NConditionStmt(condition->copy(), body->copy());
	}

	bool isBlockStmt() const override
	{
		return true;
	}

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

	NLoopStatement* copy() const override
	{
		return new NLoopStatement(body->copy());
	}

	ADD_ID(NLoopStatement)
};

class NWhileStatement : public NConditionStmt
{
	bool isDoWhile;
	bool isUntil;

public:
	NWhileStatement(NExpression* condition, NStatementList* body, bool isDoWhile = false, bool isUntil = false)
	: NConditionStmt(condition, body), isDoWhile(isDoWhile), isUntil(isUntil) {}

	NWhileStatement* copy() const override
	{
		auto cn = condition ? condition->copy() : nullptr;
		return new NWhileStatement(cn, body->copy(), isDoWhile, isUntil);
	}

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
	NExpression* value;
	NStatementList* body;
	Token* token;

public:
	NSwitchCase(Token* token, NStatementList* body, NExpression* value = nullptr)
	: value(value), body(body), token(token) {}

	NSwitchCase* copy() const override
	{
		auto vl = value ? value->copy() : nullptr;
		return new NSwitchCase(token->copy(), body->copy(), vl);
	}

	NExpression* getValue() const
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

	NSwitchStatement* copy() const override
	{
		return new NSwitchStatement(value->copy(), cases->copy());
	}

	bool isBlockStmt() const override
	{
		return true;
	}

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

	NForStatement* copy() const override
	{
		auto cn = condition ? condition->copy() : nullptr;
		return new NForStatement(preStm->copy(), cn, postExp->copy(), body->copy());
	}

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
	NIfStatement(NExpression* condition, NStatementList* ifBody, NStatementList* elseBody = nullptr)
	: NConditionStmt(condition, ifBody), elseBody(elseBody) {}

	NIfStatement* copy() const override
	{
		auto el = elseBody ? elseBody->copy() : nullptr;
		return new NIfStatement(condition->copy(), body->copy(), el);
	}

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

	NLabelStatement* copy() const override
	{
		return new NLabelStatement(name->copy());
	}

	ADD_ID(NLabelStatement)
};

class NJumpStatement : public NStatement
{
public:
	bool isTerminator() const override
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

	NReturnStatement* copy() const override
	{
		auto rt = retToken ? retToken->copy() : nullptr;
		auto vl = value ? value->copy() : nullptr;
		return new NReturnStatement(rt, vl);
	}

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

	NGotoStatement* copy() const override
	{
		return new NGotoStatement(name->copy());
	}

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
	NExpression* level;

public:
	NLoopBranch(Token* token, int type, NExpression* level = nullptr)
	: token(token), type(type), level(level) {}

	NLoopBranch* copy() const override
	{
		auto lv = level ? level->copy() : nullptr;
		return new NLoopBranch(token->copy(), type, lv);
	}

	operator Token*() const
	{
		return token;
	}

	int getType() const
	{
		return type;
	}

	NExpression* getLevel() const
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

	NDeleteStatement* copy() const override
	{
		return new NDeleteStatement(variable->copy());
	}

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

	NDestructorCall* copy() const override
	{
		return new NDestructorCall(baseVar->copy(), thisToken->copy());
	}

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

	operator Token*() const override
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

	NAssignment* copy() const override
	{
		return new NAssignment(oper, opTok->copy(), lhs->copy(), rhs->copy());
	}

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

	NTernaryOperator* copy() const override
	{
		return new NTernaryOperator(condition->copy(), trueVal->copy(), colTok->copy(), falseVal->copy());
	}

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

	operator Token*() const override
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

	NNewExpression* copy() const override
	{
		auto ar = args ? args->copy() : nullptr;
		return new NNewExpression(token->copy(), type->copy(), ar);
	}

	NDataType* getType() const
	{
		return type;
	}

	NExpressionList* getArgs() const
	{
		return args;
	}

	operator Token*() const override
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

	NLogicalOperator* copy() const override
	{
		return new NLogicalOperator(oper, opTok->copy(), lhs->copy(), rhs->copy());
	}

	ADD_ID(NLogicalOperator)
};

class NCompareOperator : public NBinaryOperator
{
public:
	NCompareOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	NCompareOperator* copy() const override
	{
		return new NCompareOperator(oper, opTok->copy(), lhs->copy(), rhs->copy());
	}

	ADD_ID(NCompareOperator)
};

class NBinaryMathOperator : public NBinaryOperator
{
public:
	NBinaryMathOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	NBinaryMathOperator* copy() const override
	{
		return new NBinaryMathOperator(oper, opTok->copy(), lhs->copy(), rhs->copy());
	}

	ADD_ID(NBinaryMathOperator)
};

class NNullCoalescing : public NBinaryOperator
{
public:
	NNullCoalescing(Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(0, opToken, lhs, rhs) {}

	NNullCoalescing* copy() const override
	{
		return new NNullCoalescing(opTok->copy(), lhs->copy(), rhs->copy());
	}

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
	NDataTypeList* args;

public:
	NArrowOperator(NDataType* dtype, Token* name, NDataTypeList* args)
	: type(DATA), dtype(dtype), exp(nullptr), name(name), args(args) {}

	NArrowOperator(NExpression* exp, Token* name, NDataTypeList* args)
	: type(EXP), dtype(nullptr), exp(exp), name(name), args(args) {}

	NArrowOperator(const NArrowOperator& other)
	: type(other.type), name(other.name)
	{
		args = other.args ? other.args->copy() : nullptr;
		dtype = other.dtype ? other.dtype->copy() : nullptr;
		exp = other.exp ? other.exp->copy() : nullptr;
	}

	NArrowOperator* copy() const override
	{
		return new NArrowOperator(*this);
	}

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

	NDataTypeList* getArgs() const
	{
		return args;
	}

	operator Token*() const override
	{
		switch (type) {
		default:
		case DATA: return *dtype;
		case EXP: return *exp;
		}
	}

	~NArrowOperator()
	{
		delete dtype;
		delete exp;
		delete name;
		delete args;
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

	NUnaryMathOperator* copy() const override
	{
		return new NUnaryMathOperator(oper, opTok->copy(), unary->copy());
	}

	ADD_ID(NUnaryMathOperator)
};

class NFunctionCall : public NVariable
{
	Token* name;
	NExpressionList* arguments;

public:
	NFunctionCall(Token* name, NExpressionList* arguments)
	: name(name), arguments(arguments) {}

	NFunctionCall* copy() const override
	{
		return new NFunctionCall(name->copy(), arguments->copy());
	}

	operator Token*() const override
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

	NMemberFunctionCall* copy() const override
	{
		return new NMemberFunctionCall(baseVar->copy(), funcName->copy(), arguments->copy());
	}

	NVariable* getBaseVar() const
	{
		return baseVar;
	}

	operator Token*() const override
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

	NIncrement* copy() const override
	{
		return new NIncrement(oper, opTok->copy(), variable->copy(), isPostfix);
	}

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
