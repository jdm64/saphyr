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

using NStatementList = NodeList<NStatement>;

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

using NExpressionList = NodeList<NExpression>;

class NExpressionStm : public NStatement
{
	uPtr<NExpression> exp;

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
		return exp.get();
	}

	ADD_ID(NExpressionStm)
};

class NConstant : public NExpression
{
	uPtr<Token> value;
	string strVal;

public:
	explicit NConstant(Token* token, const string& strVal)
	: value(token), strVal(strVal) {}

	Token* getValueTok() const
	{
		return value.get();
	}

	operator Token*() const override
	{
		return getValueTok();
	}

	bool isComplex() const override
	{
		return false;
	}

	const string& getConstStr() const
	{
		return strVal;
	}

	string& getStr()
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
};

class NNullPointer : public NConstant
{
public:
	explicit NNullPointer(Token* token)
	: NConstant(token, token->str) {}

	NNullPointer* copy() const override
	{
		return new NNullPointer(getValueTok()->copy());
	}

	ADD_ID(NNullPointer)
};

class NStringLiteral : public NConstant
{
public:
	explicit NStringLiteral(Token* str)
	: NConstant(str, str->str)
	{
		Token::unescape(getStr());
	}

	NStringLiteral(const NStringLiteral& other)
	: NConstant(other.getValueTok()->copy(), other.getConstStr())
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
		return new NBoolConst(getValueTok()->copy(), bvalue);
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
		getStr() = getValueTok()->str.substr(1, getValueTok()->str.length() - 2);
	}

	NCharConst(const NCharConst& other)
	: NIntLikeConst(other.getValueTok()->copy())
	{
		getStr() = other.getConstStr();
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
	explicit NIntConst(Token* value, int base = 10)
	: NIntLikeConst(value), base(base)
	{
		Token::remove(getStr());
	}

	NIntConst(const NIntConst& other)
	: NIntLikeConst(other.getValueTok()->copy()), base(other.base)
	{
		getStr() = other.getConstStr();
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
		Token::remove(getStr());
	}

	NFloatConst(const NFloatConst& other)
	: NConstant(other.getValueTok()->copy(), other.getConstStr())
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
	uPtr<Token> filename;

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
		return filename.get();
	}

	ADD_ID(NImportStm)
};

class NDeclaration : public NStatement
{
	uPtr<Token> name;

public:
	explicit NDeclaration(Token* name)
	: name(name) {}

	Token* getName() const
	{
		return name.get();
	}
};

class NDataType : public Node
{
public:
	virtual operator Token*() const = 0;

	virtual NDataType* copy() const override = 0;
};
using NDataTypeList = NodeList<NDataType>;

class NNamedType : public NDataType
{
	uPtr<Token> token;

public:
	explicit NNamedType(Token* token)
	: token(token) {}

	operator Token*() const override
	{
		return getName();
	}

	Token* getName() const
	{
		return token.get();
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
		return new NBaseType(getName()->copy(), type);
	}

	int getType() const
	{
		return type;
	}

	ADD_ID(NBaseType)
};

class NConstType : public NDataType
{
	uPtr<Token> constTok;
	uPtr<NDataType> type;

public:
	NConstType(Token* constTok, NDataType* type)
	: constTok(constTok), type(type) {}

	NConstType* copy() const override
	{
		return new NConstType(constTok->copy(), type->copy());
	}

	operator Token*() const override
	{
		return constTok.get();
	}

	NDataType* getType() const
	{
		return type.get();
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
		return new NThisType(getName()->copy());
	}

	ADD_ID(NThisType);
};

class NArrayType : public NDataType
{
	uPtr<Token> lBrac;
	uPtr<NDataType> baseType;
	uPtr<NExpression> size;

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
		return lBrac.get();
	}

	NDataType* getBaseType() const
	{
		return baseType.get();
	}

	NExpression* getSize() const
	{
		return size.get();
	}

	ADD_ID(NArrayType)
};

class NVecType : public NDataType
{
	uPtr<NDataType> baseType;
	uPtr<NExpression> size;
	uPtr<Token> vecToken;

public:
	NVecType(Token* vecToken, NExpression* size, NDataType* baseType)
	: baseType(baseType), size(size), vecToken(vecToken) {}

	NVecType* copy() const override
	{
		return new NVecType(vecToken->copy(), size->copy(), baseType->copy());
	}

	NExpression* getSize() const
	{
		return size.get();
	}

	NDataType* getBaseType() const
	{
		return baseType.get();
	}

	operator Token*() const override
	{
		return vecToken.get();
	}

	ADD_ID(NVecType)
};

class NUserType : public NNamedType
{
	uPtr<NDataTypeList> templateArgs;

public:
	explicit NUserType(Token* name, NDataTypeList* templateArgs = nullptr)
	: NNamedType(name), templateArgs(templateArgs) {}

	NUserType* copy() const override
	{
		auto args = templateArgs ? templateArgs->copy() : nullptr;
		return new NUserType(getName()->copy(), args);
	}

	NDataTypeList* getTemplateArgs()
	{
		return templateArgs.get();
	}

	ADD_ID(NUserType)
};

class NPointerType : public NDataType
{
	uPtr<NDataType> baseType;
	uPtr<Token> atTok;

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
		return atTok.get();
	}

	NDataType* getBaseType() const
	{
		return baseType.get();
	}

	ADD_ID(NPointerType)
};

class NFuncPointerType : public NDataType
{
	uPtr<NDataType> returnType;
	uPtr<NDataTypeList> params;
	uPtr<Token> atTok;

public:
	NFuncPointerType(Token* atTok, NDataType* returnType, NDataTypeList* params)
	: returnType(returnType), params(params), atTok(atTok) {}

	NFuncPointerType* copy() const override
	{
		return new NFuncPointerType(atTok->copy(), returnType->copy(), params->copy());
	}

	operator Token*() const override
	{
		return atTok.get();
	}

	NDataType* getReturnType() const
	{
		return returnType.get();
	}

	NDataTypeList* getParams() const
	{
		return params.get();
	}

	ADD_ID(NFuncPointerType)
};

class NVariableDecl : public NDeclaration
{
	uPtr<NExpression> initExp;
	uPtr<NExpressionList> initList;
	NDataType* type;

public:
	explicit NVariableDecl(Token* name, NExpression* initExp = nullptr)
	: NDeclaration(name), initExp(initExp), initList(nullptr), type(nullptr) {}

	NVariableDecl(Token* name, NExpressionList* initList)
	: NDeclaration(name), initExp(nullptr), initList(initList), type(nullptr) {}

	NVariableDecl(const NVariableDecl& other)
	: NDeclaration(other.getName()->copy())
	{
		initExp.reset(other.initExp.get() ? other.initExp->copy() : nullptr);
		initList.reset(other.initList.get() ? other.initList->copy() : nullptr);
		type = other.type;
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
		return initExp.get();
	}

	NExpressionList* getInitList() const
	{
		return initList.get();
	}

	ADD_ID(NVariableDecl)
};

using NVariableDeclList = NodeList<NVariableDecl>;

class NGlobalVariableDecl : public NVariableDecl
{
public:
	explicit NGlobalVariableDecl(Token* name, NExpression* initExp = nullptr)
	: NVariableDecl(name, initExp) {}

	NGlobalVariableDecl* copy() const override
	{
		auto in = getInitExp() ? getInitExp()->copy() : nullptr;
		return new NGlobalVariableDecl(getName()->copy(), in);
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
	uPtr<Token> name;

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
		return name.get();
	}

	bool isComplex() const override
	{
		return false;
	}

	ADD_ID(NBaseVariable)
};

class NArrayVariable : public NVariable
{
	uPtr<NVariable> arrVar;
	uPtr<NExpression> index;
	uPtr<Token> brackTok;

public:
	NArrayVariable(NVariable* arrVar, Token* brackTok, NExpression* index)
	: arrVar(arrVar), index(index), brackTok(brackTok) {}

	NArrayVariable* copy() const override
	{
		return new NArrayVariable(arrVar->copy(), brackTok->copy(), index->copy());
	}

	NExpression* getIndex() const
	{
		return index.get();
	}

	operator Token*() const override
	{
		return brackTok.get();
	}

	NVariable* getArrayVar() const
	{
		return arrVar.get();
	}

	ADD_ID(NArrayVariable)
};

class NMemberVariable : public NVariable
{
	uPtr<NVariable> baseVar;
	uPtr<Token> memberName;

public:
	NMemberVariable(NVariable* baseVar, Token* memberName)
	: baseVar(baseVar), memberName(memberName) {}

	NMemberVariable* copy() const override
	{
		return new NMemberVariable(baseVar->copy(), memberName->copy());
	}

	NVariable* getBaseVar() const
	{
		return baseVar.get();
	}

	operator Token*() const override
	{
		return getMemberName();
	}

	Token* getMemberName() const
	{
		return memberName.get();
	}

	ADD_ID(NMemberVariable)
};

class NExprVariable : public NVariable
{
	uPtr<NExpression> expr;

public:
	explicit NExprVariable(NExpression* expr)
	: expr(expr) {}

	NExprVariable* copy() const override
	{
		return new NExprVariable(expr->copy());
	}

	operator Token*() const override
	{
		return *getExp();
	}

	NExpression* getExp() const
	{
		return expr.get();
	}

	ADD_ID(NExprVariable)
};

class NDereference : public NVariable
{
	uPtr<NVariable> derefVar;
	uPtr<Token> atTok;

public:
	NDereference(NVariable* derefVar, Token* atTok)
	: derefVar(derefVar), atTok(atTok) {}

	NDereference* copy() const override
	{
		return new NDereference(derefVar->copy(), atTok->copy());
	}

	NVariable* getVar() const
	{
		return derefVar.get();
	}

	operator Token*() const override
	{
		return atTok.get();
	}

	ADD_ID(NDereference)
};

class NAddressOf : public NVariable
{
	uPtr<NVariable> addVar;
	uPtr<Token> token;

public:
	explicit NAddressOf(NVariable* addVar, Token* token)
	: addVar(addVar), token(token) {}

	NAddressOf* copy() const override
	{
		return new NAddressOf(addVar->copy(), token->copy());
	}

	NVariable* getVar() const
	{
		return addVar.get();
	}

	operator Token*() const override
	{
		return token.get();
	}

	ADD_ID(NAddressOf)
};

class NParameter : public NDeclaration
{
	uPtr<NDataType> type;

public:
	NParameter(NDataType* type, Token* name)
	: NDeclaration(name), type(type) {}

	NParameter* copy() const override
	{
		return new NParameter(type->copy(), getName()->copy());
	}

	NDataType* getType() const
	{
		return type.get();
	}

	ADD_ID(NParameter)
};
using NParameterList = NodeList<NParameter>;

class NVariableDeclGroup : public NStatement
{
	uPtr<NDataType> type;
	uPtr<NVariableDeclList> variables;

public:
	NVariableDeclGroup(NDataType* type, NVariableDeclList* variables)
	: type(type), variables(variables) {}

	NVariableDeclGroup* copy() const override
	{
		return new NVariableDeclGroup(type->copy(), variables->copy());
	}

	NDataType* getType() const
	{
		return type.get();
	}

	NVariableDeclList* getVars() const
	{
		return variables.get();
	}

	ADD_ID(NVariableDeclGroup)
};
using NVariableDeclGroupList = NodeList<NVariableDeclGroup>;

class NAliasDeclaration : public NDeclaration
{
	uPtr<NDataType> type;

public:
	NAliasDeclaration(Token* name, NDataType* type)
	: NDeclaration(name), type(type) {}

	NAliasDeclaration* copy() const override
	{
		return new NAliasDeclaration(getName()->copy(), type->copy());
	}

	NDataType* getType() const
	{
		return type.get();
	}

	ADD_ID(NAliasDeclaration)
};

class NTemplatedDeclaration : public NDeclaration
{
	uPtr<NIdentifierList> templateParams;
	uPtr<NAttributeList> attrs;

public:
	NTemplatedDeclaration(Token* name, NIdentifierList* templateParams, NAttributeList* attrs)
	: NDeclaration(name), templateParams(templateParams), attrs(attrs) {}

	NTemplatedDeclaration* copy() const = 0;

	NIdentifierList* getTemplateParams() const
	{
		return templateParams.get();
	}

	NAttributeList* getAttrs() const
	{
		return attrs.get();
	}
};

class NStructDeclaration : public NTemplatedDeclaration
{
public:
	enum class CreateType {STRUCT, UNION, CLASS};

private:
	CreateType ctype;
	uPtr<NVariableDeclGroupList> list;

public:
	NStructDeclaration(Token* name, CreateType ctype, NVariableDeclGroupList* list = nullptr, NIdentifierList* templateParams = nullptr, NAttributeList* attrs = nullptr)
	: NTemplatedDeclaration(name, templateParams, attrs), ctype(ctype), list(list) {}

	NStructDeclaration* copy() const override
	{
		auto ls = list ? list->copy() : nullptr;
		auto tp = getTemplateParams() ? getTemplateParams()->copy() : nullptr;
		auto at = getAttrs() ? getAttrs()->copy() : nullptr;
		return new NStructDeclaration(getName()->copy(), ctype, ls, tp, at);
	}

	CreateType getType() const
	{
		return ctype;
	}

	NVariableDeclGroupList* getVars() const
	{
		return list.get();
	}

	ADD_ID(NStructDeclaration)
};

class NEnumDeclaration : public NDeclaration
{
	uPtr<NVariableDeclList> variables;
	uPtr<NDataType> baseType;

public:
	NEnumDeclaration(Token* name, NVariableDeclList* variables, NDataType* baseType = nullptr)
	: NDeclaration(name), variables(variables), baseType(baseType) {}

	NEnumDeclaration* copy() const override
	{
		auto bt = baseType ? baseType->copy() : nullptr;
		return new NEnumDeclaration(getName()->copy(), variables->copy(), bt);
	}

	NVariableDeclList* getVarList() const
	{
		return variables.get();
	}

	NDataType* getBaseType() const
	{
		return baseType.get();
	}

	ADD_ID(NEnumDeclaration)
};

class NFunctionDeclaration : public NDeclaration
{
	uPtr<NDataType> rtype;
	uPtr<NParameterList> params;
	uPtr<NStatementList> body;
	uPtr<NAttributeList> attrs;

public:
	NFunctionDeclaration(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body = nullptr, NAttributeList* attrs = nullptr)
	: NDeclaration(name), rtype(rtype), params(params), body(body), attrs(attrs) {}

	NFunctionDeclaration* copy() const override
	{
		auto bd = body ? body->copy() : nullptr;
		auto at = attrs ? attrs->copy() : nullptr;
		return new NFunctionDeclaration(getName()->copy(), rtype->copy(), params->copy(), bd, at);
	}

	NDataType* getRType() const
	{
		return rtype.get();
	}

	NParameterList* getParams() const
	{
		return params.get();
	}

	NStatementList* getBody() const
	{
		return body.get();
	}

	NAttributeList* getAttrs() const
	{
		return attrs.get();
	}

	ADD_ID(NFunctionDeclaration)
};

// forward declaration
class NClassDeclaration;

class NClassMember : public NDeclaration
{
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
using NClassMemberList = NodeList<NClassMember>;

class NMemberInitializer : public NStatement
{
	uPtr<Token> name;
	uPtr<NExpressionList> expression;

public:
	NMemberInitializer(Token* name, NExpressionList* expression)
	: name(name), expression(expression) {}

	NMemberInitializer* copy() const override
	{
		return new NMemberInitializer(name->copy(), expression->copy());
	}

	Token* getName() const
	{
		return name.get();
	}

	NExpressionList* getExp() const
	{
		return expression.get();
	}

	ADD_ID(NMemberInitializer)
};
using NInitializerList = NodeList<NMemberInitializer>;

class NClassDeclaration : public NTemplatedDeclaration
{
	uPtr<NClassMemberList> list;

public:
	explicit NClassDeclaration(Token* name, NClassMemberList* list = nullptr, NIdentifierList* templateParams = nullptr, NAttributeList* attrs = nullptr)
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
		auto tp = getTemplateParams() ? getTemplateParams()->copy() : nullptr;
		auto at = getAttrs() ? getAttrs()->copy() : nullptr;
		return new NClassDeclaration(getName()->copy(), ls, tp, at);
	}

	NClassMemberList* getMembers() const
	{
		return list.get();
	}

	void setMembers(NClassMemberList* members)
	{
		list.reset(members);
	}

	ADD_ID(NClassDeclaration)
};

class NClassStructDecl : public NClassMember
{
	uPtr<NVariableDeclGroupList> list;

public:
	NClassStructDecl(Token* name, NVariableDeclGroupList* list)
	: NClassMember(name), list(list) {}

	NClassStructDecl* copy() const override
	{
		return new NClassStructDecl(getName()->copy(), list->copy());
	}

	MemberType memberType() const override
	{
		return MemberType::STRUCT;
	}

	NVariableDeclGroupList* getVarList() const
	{
		return list.get();
	}

	ADD_ID(NClassStructDecl)
};

class NClassFunctionDecl : public NClassMember
{
	uPtr<NDataType> rtype;
	uPtr<NParameterList> params;
	uPtr<NStatementList> body;
	uPtr<NAttributeList> attrs;

public:
	NClassFunctionDecl(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body, NAttributeList* attrs = nullptr)
	: NClassMember(name), rtype(rtype), params(params), body(body), attrs(attrs) {}

	NClassFunctionDecl* copy() const override
	{
		auto at = attrs ? attrs->copy() : nullptr;
		return new NClassFunctionDecl(getName()->copy(), rtype->copy(), params->copy(), body->copy(), at);
	}

	MemberType memberType() const override
	{
		return MemberType::FUNCTION;
	}

	NParameterList* getParams() const
	{
		return params.get();
	}

	NDataType* getRType() const
	{
		return rtype.get();
	}

	void setRType(NDataType* type)
	{
		rtype.reset(type);
	}

	NStatementList* getBody() const
	{
		return body.get();
	}

	void setBody(NStatementList* other)
	{
		body.reset(other);
	}

	NAttributeList* getAttrs() const
	{
		return attrs.get();
	}

	ADD_ID(NClassFunctionDecl)
};

class NClassConstructor : public NClassFunctionDecl
{
	uPtr<NInitializerList> initList;

public:
	NClassConstructor(Token* name,  NParameterList* params, NInitializerList* initList, NStatementList* body, NAttributeList* attrs = nullptr)
	: NClassFunctionDecl(name, nullptr, params, body, attrs), initList(initList) {}

	NClassConstructor* copy() const override
	{
		return new NClassConstructor(getName()->copy(), getParams()->copy(), initList->copy(), getBody()->copy(), getAttrs() ? getAttrs()->copy() : nullptr);
	}

	MemberType memberType() const override
	{
		return MemberType::CONSTRUCTOR;
	}

	NInitializerList* getInitList() const
	{
		return initList.get();
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
		return new NClassDestructor(getName()->copy(), getBody()->copy());
	}

	MemberType memberType() const override
	{
		return MemberType::DESTRUCTOR;
	}

	ADD_ID(NClassDestructor)
};

class NConditionStmt : public NStatement
{
	uPtr<NExpression> condition;
	uPtr<NStatementList> body;

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
		return condition.get();
	}

	NStatementList* getBody() const
	{
		return body.get();
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
		return new NLoopStatement(getBody()->copy());
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
		auto cn = getCond() ? getCond()->copy() : nullptr;
		return new NWhileStatement(cn, getBody()->copy(), isDoWhile, isUntil);
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
	uPtr<NExpression> value;
	uPtr<NStatementList> body;
	uPtr<Token> token;

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
		return value.get();
	}

	NStatementList* getBody() const
	{
		return body.get();
	}

	operator Token*() const
	{
		return token.get();
	}

	bool isValueCase() const
	{
		return value.get();
	}

	bool isLastStmBranch() const
	{
		auto last = body->back();
		if (!last)
			return false;
		return last->isTerminator();
	}

	ADD_ID(NSwitchCase)
};
using NSwitchCaseList = NodeList<NSwitchCase>;

class NSwitchStatement : public NStatement
{
	uPtr<NExpression> value;
	uPtr<NSwitchCaseList> cases;

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
		return value.get();
	}

	NSwitchCaseList* getCases() const
	{
		return cases.get();
	}

	ADD_ID(NSwitchStatement)
};

class NForStatement : public NConditionStmt
{
	uPtr<NStatementList> preStm;
	uPtr<NExpressionList> postExp;

public:
	NForStatement(NStatementList* preStm, NExpression* condition, NExpressionList* postExp, NStatementList* body)
	: NConditionStmt(condition, body), preStm(preStm), postExp(postExp) {}

	NForStatement* copy() const override
	{
		auto cn = getCond() ? getCond()->copy() : nullptr;
		return new NForStatement(preStm->copy(), cn, postExp->copy(), getBody()->copy());
	}

	NStatementList* getPreStm() const
	{
		return preStm.get();
	}

	NExpressionList* getPostExp() const
	{
		return postExp.get();
	}

	ADD_ID(NForStatement)
};

class NIfStatement : public NConditionStmt
{
	uPtr<NStatementList> elseBody;

public:
	NIfStatement(NExpression* condition, NStatementList* ifBody, NStatementList* elseBody = nullptr)
	: NConditionStmt(condition, ifBody), elseBody(elseBody) {}

	NIfStatement* copy() const override
	{
		auto el = elseBody ? elseBody->copy() : nullptr;
		return new NIfStatement(getCond()->copy(), getBody()->copy(), el);
	}

	NStatementList* getElseBody() const
	{
		return elseBody.get();
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
		return new NLabelStatement(getName()->copy());
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

	virtual operator Token*() const = 0;
};

class NReturnStatement : public NJumpStatement
{
	uPtr<NExpression> value;
	uPtr<Token> retToken;

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
		return value.get();
	}

	operator Token*() const override
	{
		return retToken.get();
	}

	ADD_ID(NReturnStatement)
};

class NGotoStatement : public NJumpStatement
{
	uPtr<Token> name;

public:
	explicit NGotoStatement(Token* name)
	: name(name) {}

	NGotoStatement* copy() const override
	{
		return new NGotoStatement(name->copy());
	}

	operator Token*() const override
	{
		return getName();
	}

	Token* getName() const
	{
		return name.get();
	}

	ADD_ID(NGotoStatement)
};

class NLoopBranch : public NJumpStatement
{
	uPtr<Token> token;
	int type;
	uPtr<NExpression> level;

public:
	NLoopBranch(Token* token, int type, NExpression* level = nullptr)
	: token(token), type(type), level(level) {}

	NLoopBranch* copy() const override
	{
		auto lv = level ? level->copy() : nullptr;
		return new NLoopBranch(token->copy(), type, lv);
	}

	operator Token*() const override
	{
		return token.get();
	}

	int getType() const
	{
		return type;
	}

	NExpression* getLevel() const
	{
		return level.get();
	}

	ADD_ID(NLoopBranch)
};

class NDeleteStatement : public NStatement
{
	uPtr<NVariable> variable;

public:
	explicit NDeleteStatement(NVariable* variable)
	: variable(variable) {}

	NDeleteStatement* copy() const override
	{
		return new NDeleteStatement(variable->copy());
	}

	NVariable* getVar() const
	{
		return variable.get();
	}

	ADD_ID(NDeleteStatement)
};

class NDestructorCall : public NStatement
{
	uPtr<NVariable> baseVar;
	uPtr<Token> thisToken;

public:
	NDestructorCall(NVariable* baseVar, Token* thisToken)
	: baseVar(baseVar), thisToken(thisToken) {}

	NDestructorCall* copy() const override
	{
		return new NDestructorCall(baseVar->copy(), thisToken->copy());
	}

	NVariable* getVar() const
	{
		return baseVar.get();
	}

	Token* getThisToken() const
	{
		return thisToken.get();
	}

	ADD_ID(NDestructorCall)
};

class NOperatorExpr : public NExpression
{
	int oper;
	uPtr<Token> opTok;

public:
	NOperatorExpr(int oper, Token* opTok)
	: oper(oper), opTok(opTok) {}

	int getOp() const
	{
		return oper;
	}

	Token* getOpToken() const
	{
		return opTok.get();
	}

	operator Token*() const override
	{
		return getOpToken();
	}
};

class NAssignment : public NOperatorExpr
{
	uPtr<NVariable> lhs;
	uPtr<NExpression> rhs;

public:
	NAssignment(int oper, Token* opToken, NVariable* lhs, NExpression* rhs)
	: NOperatorExpr(oper, opToken), lhs(lhs), rhs(rhs) {}

	NAssignment* copy() const override
	{
		return new NAssignment(getOp(), getOpToken()->copy(), lhs->copy(), rhs->copy());
	}

	NVariable* getLhs() const
	{
		return lhs.get();
	}

	NExpression* getRhs() const
	{
		return rhs.get();
	}

	ADD_ID(NAssignment)
};

class NTernaryOperator : public NExpression
{
	uPtr<NExpression> condition;
	uPtr<NExpression> trueVal;
	uPtr<NExpression> falseVal;
	uPtr<Token> colTok;

public:
	NTernaryOperator(NExpression* condition, NExpression* trueVal, Token *colTok, NExpression* falseVal)
	: condition(condition), trueVal(trueVal), falseVal(falseVal), colTok(colTok) {}

	NTernaryOperator* copy() const override
	{
		return new NTernaryOperator(condition->copy(), trueVal->copy(), colTok->copy(), falseVal->copy());
	}

	NExpression* getCondition() const
	{
		return condition.get();
	}

	NExpression* getTrueVal() const
	{
		return trueVal.get();
	}

	NExpression* getFalseVal() const
	{
		return falseVal.get();
	}

	operator Token*() const override
	{
		return colTok.get();
	}

	ADD_ID(NTernaryOperator)
};

class NNewExpression : public NExpression
{
	uPtr<NDataType> type;
	uPtr<Token> token;
	uPtr<NExpressionList> args;

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
		return type.get();
	}

	NExpressionList* getArgs() const
	{
		return args.get();
	}

	operator Token*() const override
	{
		return token.get();
	}

	ADD_ID(NNewExpression)
};

class NBinaryOperator : public NOperatorExpr
{
	uPtr<NExpression> lhs;
	uPtr<NExpression> rhs;

public:
	NBinaryOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NOperatorExpr(oper, opToken), lhs(lhs), rhs(rhs) {}

	NExpression* getLhs() const
	{
		return lhs.get();
	}

	NExpression* getRhs() const
	{
		return rhs.get();
	}
};

class NLogicalOperator : public NBinaryOperator
{
public:
	NLogicalOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	NLogicalOperator* copy() const override
	{
		return new NLogicalOperator(getOp(), getOpToken()->copy(), getLhs()->copy(), getRhs()->copy());
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
		return new NCompareOperator(getOp(), getOpToken()->copy(), getLhs()->copy(), getRhs()->copy());
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
		return new NBinaryMathOperator(getOp(), getOpToken()->copy(), getLhs()->copy(), getRhs()->copy());
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
		return new NNullCoalescing(getOpToken()->copy(), getLhs()->copy(), getRhs()->copy());
	}

	ADD_ID(NNullCoalescing)
};

class NArrowOperator : public NVariable
{
public:
	enum OfType { DATA, EXP };

private:
	OfType type;
	uPtr<NDataType> dtype;
	uPtr<NExpression> exp;
	uPtr<Token> name;
	uPtr<NDataTypeList> args;

public:
	NArrowOperator(NDataType* dtype, Token* name, NDataTypeList* args)
	: type(DATA), dtype(dtype), exp(nullptr), name(name), args(args) {}

	NArrowOperator(NExpression* exp, Token* name, NDataTypeList* args)
	: type(EXP), dtype(nullptr), exp(exp), name(name), args(args) {}

	NArrowOperator(const NArrowOperator& other)
	: type(other.type), name(other.name.get())
	{
		args.reset(other.args ? other.args->copy() : nullptr);
		dtype.reset(other.dtype ? other.dtype->copy() : nullptr);
		exp.reset(other.exp ? other.exp->copy() : nullptr);
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
		return exp.get();
	}

	NDataType* getDataType() const
	{
		return dtype.get();
	}

	Token* getName() const
	{
		return name.get();
	}

	NDataTypeList* getArgs() const
	{
		return args.get();
	}

	operator Token*() const override
	{
		switch (type) {
		default:
		case DATA: return *getDataType();
		case EXP: return *getExp();
		}
	}

	ADD_ID(NArrowOperator)
};

class NUnaryOperator : public NOperatorExpr
{
	uPtr<NExpression> unary;

public:
	NUnaryOperator(int oper, Token* opToken, NExpression* unary)
	: NOperatorExpr(oper, opToken), unary(unary) {}

	NExpression* getExp() const
	{
		return unary.get();
	}
};

class NUnaryMathOperator : public NUnaryOperator
{
public:
	NUnaryMathOperator(int oper, Token* opToken, NExpression* unaryExp)
	: NUnaryOperator(oper, opToken, unaryExp) {}

	NUnaryMathOperator* copy() const override
	{
		return new NUnaryMathOperator(getOp(), getOpToken()->copy(), getExp()->copy());
	}

	ADD_ID(NUnaryMathOperator)
};

class NFunctionCall : public NVariable
{
	uPtr<Token> name;
	uPtr<NExpressionList> arguments;

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
		return arguments.get();
	}

	Token* getName() const
	{
		return name.get();
	}

	ADD_ID(NFunctionCall)
};

class NMemberFunctionCall : public NVariable
{
	uPtr<NVariable> baseVar;
	uPtr<Token> funcName;
	uPtr<NExpressionList> arguments;

public:
	NMemberFunctionCall(NVariable* baseVar, Token* funcName, NExpressionList* arguments)
	: baseVar(baseVar), funcName(funcName), arguments(arguments) {}

	NMemberFunctionCall* copy() const override
	{
		return new NMemberFunctionCall(baseVar->copy(), funcName->copy(), arguments->copy());
	}

	NVariable* getBaseVar() const
	{
		return baseVar.get();
	}

	operator Token*() const override
	{
		return getName();
	}

	Token* getName() const
	{
		return funcName.get();
	}

	NExpressionList* getArguments() const
	{
		return arguments.get();
	}

	ADD_ID(NMemberFunctionCall)
};

class NIncrement : public NOperatorExpr
{
	uPtr<NVariable> variable;
	bool isPostfix;

public:
	NIncrement(int oper, Token* opToken, NVariable* variable, bool isPostfix)
	: NOperatorExpr(oper, opToken), variable(variable), isPostfix(isPostfix) {}

	NIncrement* copy() const override
	{
		return new NIncrement(getOp(), getOpToken()->copy(), variable->copy(), isPostfix);
	}

	NVariable* getVar() const
	{
		return variable.get();
	}

	bool postfix() const
	{
		return isPostfix;
	}

	ADD_ID(NIncrement)
};

#endif
