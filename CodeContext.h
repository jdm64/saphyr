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
#ifndef __CODE_CONTEXT__
#define __CODE_CONTEXT__

#include <map>
#include <stack>
#include <iostream>
#include <llvm/Instructions.h>
#include <llvm/LLVMContext.h>
#include <llvm/BasicBlock.h>
#include <llvm/Module.h>
#include <llvm/Type.h>

using namespace std;
using namespace llvm;

// forward declarations
template<typename NType>
class NodeList;
class NStatement;
typedef NodeList<NStatement> NStatementList;

struct LabelBlock
{
	BasicBlock* block;
	bool isPlaceholder;

	LabelBlock(BasicBlock* block, bool isPlaceholder)
	: block(block), isPlaceholder(isPlaceholder) {}
};

class VarTable
{
	map<string, AllocaInst*> table;

public:
	void storeVar(AllocaInst* var, string* name)
	{
		table[*name] = var;
	}

	AllocaInst* loadVar(string* name)
	{
		auto varData = table.find(*name);
		return varData != table.end()? varData->second : nullptr;
	}
};

class SymbolTable
{
	vector<VarTable> localTable;

public:
	void storeLocalVar(AllocaInst* var, string* name)
	{
		localTable.back().storeVar(var, name);
	}

	void pushLocalTable()
	{
		localTable.push_back(VarTable());
	}

	void popLocalTable()
	{
		localTable.pop_back();
	}

	void clearLocalTable()
	{
		localTable.clear();
	}

	AllocaInst* loadVar(string* name)
	{
		for (auto it = localTable.rbegin(); it != localTable.rend(); it++) {
			auto var = it->loadVar(name);
			if (var)
				return var;
		}
		return nullptr;
	}

	AllocaInst* loadVarCurr(string* name)
	{
		return localTable.back().loadVar(name);
	}
};

class CodeContext : public SymbolTable
{
	vector<BasicBlock*> funcBlocks;
	vector<BasicBlock*> continueBlocks;
	vector<BasicBlock*> breakBlocks;
	vector<BasicBlock*> redoBlocks;
	map<string, LabelBlock*> labelBlocks;

	Module* module;
	string filename;
	int errors;

	void validateFunction()
	{
		for (auto& item : labelBlocks) {
			if (item.second->isPlaceholder) {
				cout << "error: label \"" << item.first << "\" not defined" << endl;
				incErrCount();
			}
		}
	}

public:
	CodeContext(string& filename)
	: filename(filename)
	{
		module = new Module(filename, getGlobalContext());
		errors = 0;
	}

	LLVMContext& getContext()
	{
		return module->getContext();
	}

	Module* getModule()
	{
		return module;
	}

	Function* getFunction(string* name)
	{
		return module->getFunction(*name);
	}

	void incErrCount()
	{
		errors++;
	}

	BasicBlock* currBlock()
	{
		return funcBlocks.back();
	}

	void startFuncBlock(Function* function)
	{
		pushLocalTable();
		funcBlocks.clear();
		funcBlocks.push_back(BasicBlock::Create(module->getContext(), "", function));
	}

	void endFuncBlock()
	{
		validateFunction();

		clearLocalTable();
		funcBlocks.clear();
		continueBlocks.clear();
		breakBlocks.clear();
		redoBlocks.clear();

		for (auto& item : labelBlocks)
			delete item.second;
		labelBlocks.clear();
	}

	void pushBlock(BasicBlock* block)
	{
		funcBlocks.push_back(block);
	}

	void pushContinueBlock(BasicBlock* block)
	{
		continueBlocks.push_back(block);
	}
	void popContinueBlock()
	{
		continueBlocks.pop_back();
	}
	BasicBlock* getContinueBlock()
	{
		return continueBlocks.empty()? nullptr : continueBlocks.back();
	}

	void pushBreakBlock(BasicBlock* block)
	{
		breakBlocks.push_back(block);
	}
	void popBreakBlock()
	{
		breakBlocks.pop_back();
	}
	BasicBlock* getBreakBlock()
	{
		return breakBlocks.empty()? nullptr : breakBlocks.back();
	}

	void pushRedoBlock(BasicBlock* block)
	{
		redoBlocks.push_back(block);
	}
	void popRedoBlock()
	{
		redoBlocks.pop_back();
	}
	BasicBlock* getRedoBlock()
	{
		return redoBlocks.empty()?  nullptr : redoBlocks.back();
	}

	LabelBlock* getLabelBlock(string* name)
	{
		auto iter = labelBlocks.find(*name);
		return iter != labelBlocks.end()? iter->second : nullptr;
	}
	void setLabelBlock(string* name, LabelBlock* label)
	{
		labelBlocks[*name] = label;
	}

	// NOTE: can only be used inside a function to add a new block
	BasicBlock* createBlock()
	{
		return BasicBlock::Create(module->getContext(), "", currBlock()->getParent());
	}

	void genCode(NStatementList stms);
};

#endif
