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
#ifndef __CODE_CONTEXT_H__
#define __CODE_CONTEXT_H__

#include <stack>
#include <list>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include "AST.h"
#include "Value.h"

using namespace boost::program_options;
using namespace boost::filesystem;

enum BranchType { BREAK = 1, CONTINUE = 1 << 1, REDO = 1 << 2 };

struct LabelBlock
{
	BasicBlock* block;
	Token token;
	bool isPlaceholder;

	LabelBlock(BasicBlock* block, Token* token, bool isPlaceholder)
	: block(block), token(*token), isPlaceholder(isPlaceholder) {}
};

using LabelBlockPtr = unique_ptr<LabelBlock>;
#define smart_label(block, token, placeholder) unique_ptr<LabelBlock>(new LabelBlock((block), (token), (placeholder)))

class ScopeTable
{
	map<string, VecRValue> table;
	VecRValue destructables;

public:
	void storeSymbol(const RValue& var, const string& name, bool isParam = false)
	{
		table[name].push_back(var);
		if (!isParam && var.stype()->isClass()) {
			auto clType = static_cast<SClassType*>(var.stype());
			auto func = clType->getDestructor();
			if (func)
				destructables.push_back(var);
		}
	}

	VecRValue loadSymbol(const string& name) const
	{
		auto varData = table.find(name);
		return varData != table.end()? varData->second : VecRValue();
	}

	VecRValue getDestructables()
	{
		return destructables;
	}
};

class GlobalContext
{
	friend class CodeContext;

	Module* module;
	TypeManager typeManager;

	vector<pair<Token,string>> errors;
	list<unique_ptr<NAttributeList>> attrs;

	set<path> allFiles;
	vector<path> filesStack;

	ScopeTable globalTable;

public:
	explicit GlobalContext(Module* module)
	: module(module), typeManager(module) {}

	bool hasErrors() const
	{
		return !errors.empty();
	}

	void addError(const string& error, Token* token)
	{
		if (token)
			errors.push_back({*token, error});
		else
			errors.push_back({Token(), error});
	}

	/*
	 * Returns true on errors
	 */
	bool handleErrors() const
	{
		if (errors.empty())
			return false;

		string buff;
		for (auto& error : errors) {
			buff += error.first.filename + ":" + to_string(error.first.line) + ":" + to_string(error.first.col) + ": " + error.second + "\n";
		}
		buff += "found " + to_string(errors.size()) + " errors\n";
		cout << buff;
		return true;
	}

	NAttributeList* storeAttr(NAttributeList* list)
	{
		if (list) {
			list = list->move<NAttributeList>(false);
			attrs.push_back(unique_ptr<NAttributeList>(list));
		}
		return list;
	}

	void pushFile(const path& filename)
	{
		filesStack.push_back(filename);
		allFiles.insert(filename);
	}

	bool fileLoaded(const path& filename)
	{
		return allFiles.find(filename) != allFiles.end();
	}
};

class CodeContext
{
	using BlockVector = vector<llvm::BasicBlock*>;

	GlobalContext& globalCtx;

	vector<pair<string, SType*>> templateArgs;

	IRBuilder<> irBuilder;
	SFunction currFunc;
	STemplatedType* thisType;
	SClassType* currClass;
	vector<ScopeTable> localTable;

	BlockVector funcBlocks;
	BlockVector continueBlocks;
	BlockVector breakBlocks;
	BlockVector redoBlocks;
	map<string, LabelBlockPtr> labelBlocks;

	void validateFunction()
	{
		for (auto& item : labelBlocks) {
			if (item.second->isPlaceholder)
				addError("label " + item.first + " not defined", &item.second->token);
		}
	}

	BasicBlock* loopBranchLevel(const BlockVector& branchBlocks, int level) const
	{
		int blockCount = branchBlocks.size();
		int idx = level > 0? blockCount - level : -level - 1;
		return (idx >= 0 && idx < blockCount)? branchBlocks[idx] : nullptr;
	}

public:
	explicit CodeContext(GlobalContext& context)
	: globalCtx(context), irBuilder(context.module->getContext()), thisType(nullptr), currClass(nullptr) {}

	static CodeContext newForTemplate(CodeContext& context, const vector<pair<string, SType*>>& templateMappings)
	{
		CodeContext newCtx(context.globalCtx);
		newCtx.templateArgs = templateMappings;
		return newCtx;
	}

	/**
	 * global context functions
	 **/

	operator LLVMContext&()
	{
		return globalCtx.module->getContext();
	}

	Module* getModule() const
	{
		return globalCtx.module;
	}

	TypeManager& getTypeManager() const
	{
		return globalCtx.typeManager;
	}

	IRBuilder<>& IB()
	{
		return irBuilder;
	}

	bool hasErrors() const
	{
		return globalCtx.hasErrors();
	}

	size_t errorCount() const
	{
		return globalCtx.errors.size();
	}

	void addError(const string& error, Token* token)
	{
		globalCtx.addError(error, token);
	}

	bool handleErrors() const
	{
		return globalCtx.handleErrors();
	}

	NAttributeList* storeAttr(NAttributeList* list)
	{
		return globalCtx.storeAttr(list);
	}

	void popFile()
	{
		globalCtx.filesStack.pop_back();
	}

	void pushFile(const path& filename)
	{
		globalCtx.pushFile(filename);
	}

	bool fileLoaded(const path& filename)
	{
		return globalCtx.fileLoaded(filename);
	}

	path currFile() const
	{
		return globalCtx.filesStack.back();
	}

	void storeGlobalSymbol(RValue var, const string& name)
	{
		globalCtx.globalTable.storeSymbol(var, name);
	}

	VecRValue loadSymbolGlobal(const string& name) const
	{
		return globalCtx.globalTable.loadSymbol(name);
	}

	/**
	 * local context functions
	 **/

	NTemplatedDeclaration* getTemplate(const string& name)
	{
		return globalCtx.typeManager.getTemplateType(name);
	}

	void storeTemplate(const string& name, NTemplatedDeclaration* decl)
	{
		globalCtx.typeManager.storeTemplate(name, decl);
	}

	bool inTemplate() const
	{
		return !templateArgs.empty();
	}

	SType* getTemplateArg(const string& name)
	{
		auto it = find_if(templateArgs.begin(), templateArgs.end(),
			[&name](auto i){ return i.first == name; });
		return it != templateArgs.end() ? it->second : nullptr;
	}

	vector<SType*> getTemplateArgs()
	{
		vector<SType*> args;
		transform(templateArgs.begin(), templateArgs.end(), back_inserter(args), [](auto i){ return i.second; });
		return args;
	}

	void pushLocalTable()
	{
		localTable.push_back(ScopeTable());
	}

	void popLocalTable()
	{
		localTable.pop_back();
	}

	void storeLocalSymbol(RValue var, const string& name, bool isParam = false)
	{
		localTable.back().storeSymbol(var, name, isParam);
	}

	VecRValue loadSymbol(const string& name) const
	{
		auto data = loadSymbolLocal(name);
		return data.size() ? data : globalCtx.globalTable.loadSymbol(name);
	}

	VecRValue loadSymbolLocal(const string& name) const
	{
		for (auto it = localTable.rbegin(); it != localTable.rend(); it++) {
			auto data = it->loadSymbol(name);
			if (data.size())
				return data;
		}
		return {};
	}

	VecRValue loadSymbolCurr(const string& name) const
	{
		return localTable.empty() ? globalCtx.globalTable.loadSymbol(name) : localTable.back().loadSymbol(name);
	}

	VecRValue getDestructables()
	{
		VecRValue ret;
		for (auto tb : localTable) {
			auto des = tb.getDestructables();
			ret.insert(ret.end(), des.begin(), des.end());
		}
		return ret;
	}

	SFunction currFunction() const
	{
		return currFunc;
	}

	STemplatedType* getThis() const
	{
		return thisType;
	}

	void setThis(STemplatedType* type)
	{
		thisType = type;
	}

	SClassType* getClass() const
	{
		return currClass;
	}

	void setClass(SClassType* classType)
	{
		currClass = classType;
	}

	void startFuncBlock(SFunction function)
	{
		pushLocalTable();
		funcBlocks.clear();
		funcBlocks.push_back(BasicBlock::Create(getModule()->getContext(), "", function));
		irBuilder.SetInsertPoint(currBlock());
		currFunc = function;
	}

	void endFuncBlock()
	{
		validateFunction();

		localTable.clear();
		funcBlocks.clear();
		continueBlocks.clear();
		breakBlocks.clear();
		redoBlocks.clear();
		labelBlocks.clear();

		currFunc = SFunction();
	}

	void pushBlock(BasicBlock* block)
	{
		block->moveAfter(currBlock());
		funcBlocks.push_back(block);
		irBuilder.SetInsertPoint(currBlock());
	}

	void popLoopBranchBlocks(int type)
	{
		if (type & BranchType::BREAK)
			breakBlocks.pop_back();
		if (type & BranchType::CONTINUE)
			continueBlocks.pop_back();
		if (type & BranchType::REDO)
			redoBlocks.pop_back();
	}

	BasicBlock* currBlock() const
	{
		return funcBlocks.back();
	}

	// NOTE: can only be used inside a function to add a new block
	BasicBlock* createBlock() const
	{
		return BasicBlock::Create(getModule()->getContext(), "", currBlock()->getParent());
	}

	BasicBlock* getBreakBlock(int level = 1) const
	{
		return loopBranchLevel(breakBlocks, level);
	}

	BasicBlock* createBreakBlock()
	{
		auto block = createBlock();
		breakBlocks.push_back(block);
		return block;
	}

	BasicBlock* getContinueBlock(int level = 1) const
	{
		return loopBranchLevel(continueBlocks, level);
	}

	BasicBlock* createContinueBlock()
	{
		auto block = createBlock();
		continueBlocks.push_back(block);
		return block;
	}

	BasicBlock* getRedoBlock(int level = 1) const
	{
		return loopBranchLevel(redoBlocks, level);
	}

	BasicBlock* createRedoBlock()
	{
		auto block = createBlock();
		redoBlocks.push_back(block);
		return block;
	}

	LabelBlock* getLabelBlock(const string& name)
	{
		return labelBlocks[name].get();
	}

	LabelBlock* createLabelBlock(Token* name, bool isPlaceholder)
	{
		LabelBlockPtr &item = labelBlocks[name->str];
		if (!item.get()) {
			item = smart_label(createBlock(), name, isPlaceholder);
			item.get()->block->setName(name->str);
		}
		return item.get();
	}
};

#endif
