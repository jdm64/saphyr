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

typedef unique_ptr<LabelBlock> LabelBlockPtr;
#define smart_label(block, token, placeholder) unique_ptr<LabelBlock>(new LabelBlock(block, token, placeholder))

class ScopeTable
{
	map<string, RValue> table;

public:
	void storeSymbol(const RValue& var, const string& name)
	{
		table[name] = var;
	}

	RValue loadSymbol(const string& name) const
	{
		auto varData = table.find(name);
		return varData != table.end()? varData->second : RValue();
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

	void addError(string error, Token* token)
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

		for (auto& error : errors) {
			cout << error.first.filename << ":" << error.first.line << ":" << error.first.col << ": " << error.second << endl;
		}
		cout << "found " << errors.size() << " errors" << endl;
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
	typedef vector<llvm::BasicBlock*> BlockVector;
	typedef BlockVector::iterator block_iterator;

	GlobalContext& globalCtx;

	vector<pair<string, SType*>> templateArgs;

	IRBuilder<> irBuilder;
	SFunction currFunc;
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
	: globalCtx(context), irBuilder(context.module->getContext()), currClass(nullptr) {}

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

	void addError(string error, Token* token)
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

	virtual path currFile() const
	{
		return globalCtx.filesStack.back();
	}

	void storeGlobalSymbol(RValue var, const string& name)
	{
		globalCtx.globalTable.storeSymbol(var, name);
	}

	RValue loadSymbolGlobal(const string& name) const
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
		for (auto item : templateArgs) {
			if (item.first == name)
				return item.second;
		}
		return nullptr;
	}

	vector<SType*> getTemplateArgs()
	{
		vector<SType*> args;
		for (auto item : templateArgs)
			args.push_back(item.second);
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

	void storeLocalSymbol(RValue var, const string& name)
	{
		localTable.back().storeSymbol(var, name);
	}

	RValue loadSymbol(const string& name) const
	{
		auto var = loadSymbolLocal(name);
		return var? var : globalCtx.globalTable.loadSymbol(name);
	}

	RValue loadSymbolLocal(const string& name) const
	{
		for (auto it = localTable.rbegin(); it != localTable.rend(); it++) {
			auto var = it->loadSymbol(name);
			if (var)
				return var;
		}
		return {};
	}

	RValue loadSymbolCurr(const string& name) const
	{
		return localTable.empty() ? globalCtx.globalTable.loadSymbol(name) : localTable.back().loadSymbol(name);
	}

	SFunction currFunction() const
	{
		return currFunc;
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
