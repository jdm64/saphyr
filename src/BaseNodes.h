/* Saphyr, a C style compiler using LLVM
 * Copyright (C) 2009-2015, Justin Madru (justin.jdm64@gmail.com)
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
	NSizeOfOperator,
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
	NOpaqueDecl,
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
	L* move()
	{
		auto other = new L;
		for (const auto item : *this)
			other->add(item);

		doDelete = false;
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
};

#endif
