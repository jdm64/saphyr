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

class CodeContext
{
	vector<BasicBlock*> funcBlocks;
	map<string, Value*> symtable;

	vector<BasicBlock*> continueBlocks;
	vector<BasicBlock*> breakBlocks;
	vector<BasicBlock*> redoBlocks;

	Module* module;
	int errors;

public:
	CodeContext(string& filename)
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

	void storeVar(AllocaInst* var, string* name = nullptr)
	{
		symtable[name? *name : var->getName().str()] = var;
	}

	Value* loadVar(string* var)
	{
		auto varData = symtable.find(*var);

		if (varData == symtable.end())
			return nullptr;
		return varData->second;
	}

	BasicBlock* currBlock()
	{
		return funcBlocks.back();
	}

	void startFuncBlock(Function* function)
	{
		funcBlocks.clear();
		funcBlocks.push_back(BasicBlock::Create(module->getContext(), "", function));
	}

	void endFuncBlock()
	{
		symtable.clear();
		funcBlocks.clear();
		continueBlocks.clear();
		breakBlocks.clear();
		redoBlocks.clear();
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
		if (continueBlocks.empty())
			return nullptr;
		return continueBlocks.back();
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
		if (breakBlocks.empty())
			return nullptr;
		return breakBlocks.back();
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
		if (redoBlocks.empty())
			return nullptr;
		return redoBlocks.back();
	}

	// NOTE: can only be used inside a function to add a new block
	BasicBlock* createBlock()
	{
		return BasicBlock::Create(module->getContext(), "", currBlock()->getParent());
	}

	void genCode(NStatementList stms);
};

#endif
