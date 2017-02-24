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
	StartDataType,
	NArrayType,
	NBaseType,
	NFuncPointerType,
	NPointerType,
	NThisType,
	NUserType,
	NVecType,
	EndDataType,

	// expressions
	StartExpression,
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
	EndExpression,

	// statements
	StartStatement,
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
	EndStatement
};

#define NODEID_DIFF(LEFT, RIGHT) static_cast<int>(LEFT) - static_cast<int>(RIGHT)
#define ADD_ID(CLASS) NodeId id() { return NodeId::CLASS; }

class Node
{
public:
	virtual ~Node() {};

	virtual NodeId id() = 0;
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
	: line(0) {}

	Token(string token, string filename = "", int lineNum = 0)
	: str(std::move(token)), filename(std::move(filename)), line(lineNum) {}

	string str;
	string filename;
	int line;

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

class NAttrValue : public Node
{
	Token* val;

public:
	NAttrValue(Token* value)
	: val(value)
	{
		Token::unescape(value->str);
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
	}

	ADD_ID(NAttribute)
};

class NAttributeList : public NodeList<NAttribute>
{
public:
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
