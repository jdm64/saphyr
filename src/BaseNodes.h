/* Saphyr, a C style compiler using LLVM
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

#ifndef __BASE_NODES__
#define __BASE_NODES__

#include <string>

using namespace std;

enum class NodeId
{
	NAttribute,
	NAttrValue,

	// datatypes
	NArrayType,
	NBaseType,
	NConstType,
	NFuncPointerType,
	NPointerType,
	NThisType,
	NUserType,
	NVecType,

	// expressions
	NAddressOf,
	NArrayVariable,
	NArrowOperator,
	NAssignment,
	NBaseVariable,
	NBinaryMathOperator,
	NBoolConst,
	NCharConst,
	NCompareOperator,
	NDereference,
	NExprVariable,
	NFloatConst,
	NFunctionCall,
	NIncrement,
	NIntConst,
	NLogicalOperator,
	NMemberFunctionCall,
	NMemberVariable,
	NNewExpression,
	NNullCoalescing,
	NNullPointer,
	NStringLiteral,
	NTernaryOperator,
	NUnaryMathOperator,

	// statements
	NAliasDeclaration,
	NClassConstructor,
	NClassDeclaration,
	NClassDestructor,
	NClassFunctionDecl,
	NClassStructDecl,
	NConditionStmt,
	NDeleteStatement,
	NDestructorCall,
	NEnumDeclaration,
	NExpressionStm,
	NForStatement,
	NFunctionDeclaration,
	NGlobalVariableDecl,
	NGotoStatement,
	NIfStatement,
	NImportStm,
	NLabelStatement,
	NLoopBranch,
	NLoopStatement,
	NMemberInitializer,
	NParameter,
	NReturnStatement,
	NStructDeclaration,
	NSwitchCase,
	NSwitchStatement,
	NVariableDecl,
	NVariableDeclGroup,
	NWhileStatement,
};

#define ADD_ID(CLASS) NodeId id() const { return NodeId::CLASS; }
#define VISIT_CASE(ID, NODE) case NodeId::ID: visit##ID(static_cast<ID*>(NODE)); break;
#define VISIT_CASE_RETURN(ID, NODE) case NodeId::ID: return visit##ID(static_cast<ID*>(NODE));
#define VISIT_CASE2_RETURN(ID, TWO, NODE) case NodeId::ID: return visit##TWO(static_cast<TWO*>(NODE));

class Node
{
public:
	virtual ~Node() {};

	virtual NodeId id() const = 0;

	virtual Node* copy() const = 0;
};

template<typename T>
class NodeList
{
	typedef vector<T*> container;
	typedef typename container::iterator iterator;

	bool doDelete;

protected:
	container list;

public:
	explicit NodeList(bool doDelete = true)
	: doDelete(doDelete) {}

	template<typename L>
	L* move(bool deleteThis = true)
	{
		auto other = new L;
		other->addAll(*this);

		doDelete = false;
		if (deleteThis)
			delete this;

		return other;
	}

	NodeList<T>* copy() const
	{
		auto ret = new NodeList<T>(doDelete);
		ret->list.reserve(list.size());
		for (auto item : list)
			ret->add(item->copy());
		return ret;
	}

	void setDelete(bool DoDelete)
	{
		doDelete = DoDelete;
	}

	void reserve(size_t size)
	{
		list.reserve(size);
	}

	void clear()
	{
		list.clear();
	}

	bool empty() const
	{
		return list.empty();
	}

	int size() const
	{
		return list.size();
	}

	iterator begin()
	{
		return list.begin();
	}

	iterator end()
	{
		return list.end();
	}

	void add(T* item)
	{
		list.push_back(item);
	}

	void addFront(T* item)
	{
		list.insert(list.begin(), item);
	}

	void addAll(NodeList<T>& other)
	{
		list.insert(list.end(), other.begin(), other.end());
	}

	T* at(int i)
	{
		return list.at(i);
	}

	T* front()
	{
		return list.empty()? nullptr : list.front();
	}

	T* back()
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

class Token
{
public:
	Token()
	: line(0), col(0) {}

	Token(const string& token, const string& filename = "", int lineNum = 0, int colNum = 0)
	: str(token), filename(filename), line(lineNum), col(colNum) {}

	Token(const Token& token, const string& str)
	: str(str), filename(token.filename), line(token.line), col(token.col) {}

	Token* copy()
	{
		return new Token(*this);
	}

	string str;
	string filename;
	int line;
	int col;

	static void unescape(string &val)
	{
		string str;

		val = val.substr(1, val.size() - 2);
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
		val = str;
	}

	static void remove(string& val, char c = '\'')
	{
		val.erase(std::remove(val.begin(), val.end(), c), val.end());
	}
};

typedef NodeList<Token> NIdentifierList;

class NAttrValue : public Node
{
	Token* val;

public:
	explicit NAttrValue(Token* value)
	: val(value)
	{
		Token::unescape(value->str);
	}

	NAttrValue(const NAttrValue& other)
	: val(other.val->copy())
	{
	}

	NAttrValue* copy() const
	{
		return new NAttrValue(*this);
	}

	operator Token*() const
	{
		return val;
	}

	string str() const
	{
		return val->str;
	}

	~NAttrValue()
	{
		delete val;
	}

	ADD_ID(NAttrValue)
};

class NAttrValueList : public NodeList<NAttrValue>
{
public:
	static NAttrValue* find(NAttrValueList* list, int index)
	{
		if (!list)
			return nullptr;
		return index < list->size()? list->at(index) : nullptr;
	}
};

class NAttribute : public Node
{
	Token* name;
	NAttrValueList* values;

public:
	explicit NAttribute(Token* name, NAttrValueList* values = nullptr)
	: name(name), values(values) {}

	NAttribute* copy() const
	{
		auto v2 = values ? values->copy() : nullptr;
		return new NAttribute(new Token(*name), static_cast<NAttrValueList*>(v2));
	}

	operator Token*() const
	{
		return name;
	}

	Token* getName() const
	{
		return name;
	}

	NAttrValueList* getValues() const
	{
		return values;
	}

	~NAttribute()
	{
		delete name;
		delete values;
	}

	ADD_ID(NAttribute)
};

class NAttributeList : public NodeList<NAttribute>
{
public:
	NAttributeList* copy() const
	{
		return static_cast<NAttributeList*>(NodeList<NAttribute>::copy());
	}

	static NAttribute* find(NAttributeList* list, const string& name)
	{
		if (!list)
			return nullptr;
		for (auto attr : *list) {
			if (name == attr->getName()->str)
				return attr;
		}
		return nullptr;
	}
};

#endif
