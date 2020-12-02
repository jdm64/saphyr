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

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include "Value.h"

using namespace boost::filesystem;
using namespace boost::program_options;

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

class ScopeTable
{
	map<string, VecRValue> table;
	VecRValue destructables;

public:
	void storeSymbol(const RValue& var, const string& name, bool isParam = false);

	VecRValue loadSymbol(const string& name) const;

	VecRValue getDestructables();
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

	bool hasErrors() const;

	void addError(const string& error, Token* token);

	/*
	 * Returns true on errors
	 */
	bool handleErrors() const;

	NAttributeList* storeAttr(NAttributeList* list);

	void pushFile(const path& filename);

	bool fileLoaded(const path& filename);
};

class CodeContext
{
	using BlockVector = vector<BasicBlock*>;
	using BlockCount = pair<BasicBlock*,size_t>;
	using BlockCountVec = vector<BlockCount>;

	GlobalContext& globalCtx;
	variables_map& conf;

	vector<pair<string, SType*>> templateArgs;

	shared_ptr<IRBuilder<>> irBuilder;
	SFunction currFunc;
	STemplatedType* thisType = nullptr;
	SClassType* currClass = nullptr;
	vector<ScopeTable> localTable;

	BlockVector funcBlocks;
	BlockCountVec continueBlocks;
	BlockCountVec breakBlocks;
	BlockCountVec redoBlocks;
	map<string, LabelBlockPtr> labelBlocks;

	void validateFunction();

	BlockCount loopBranchLevel(const BlockCountVec& branchBlocks, int level) const;

public:
	explicit CodeContext(GlobalContext& context, variables_map& config)
	: globalCtx(context), conf(config), irBuilder(new IRBuilder<>(context.module->getContext())) {}

	static CodeContext newForTemplate(CodeContext& context, const vector<pair<string, SType*>>& templateMappings);

	static CodeContext newForLambda(CodeContext& context);

	variables_map& config() const;

	/**
	 * global context functions
	 **/

	operator LLVMContext&();

	Module* getModule() const;

	TypeManager& getTypeManager() const;

	IRBuilder<>& IB();

	bool hasErrors() const;

	size_t errorCount() const;

	void addError(const string& error, Token* token);

	bool handleErrors() const;

	NAttributeList* storeAttr(NAttributeList* list);

	void popFile();

	void pushFile(const path& filename);

	bool fileLoaded(const path& filename);

	path currFile() const;

	void storeGlobalSymbol(RValue var, const string& name);

	VecRValue loadSymbolGlobal(const string& name) const;

	/**
	 * local context functions
	 **/

	NTemplatedDeclaration* getTemplate(const string& name);

	void storeTemplate(const string& name, NTemplatedDeclaration* decl);

	bool inTemplate() const;

	SType* getTemplateArg(const string& name);

	VecSType getTemplateArgs();

	void pushLocalTable();

	void popLocalTable();

	void storeLocalSymbol(RValue var, const string& name, bool isParam = false);

	VecRValue loadSymbol(const string& name) const;

	VecRValue loadSymbolLocal(const string& name) const;

	VecRValue loadSymbolCurr(const string& name) const;

	VecRValue getDestructables(size_t level);

	SFunction currFunction() const;

	STemplatedType* getThis() const;

	void setThis(STemplatedType* type);

	SClassType* getClass() const;

	void setClass(SClassType* classType);

	void startFuncBlock(SFunction function);

	void endFuncBlock();

	void startTmpFunction(Token* prefix);

	void endTmpFunction();

	void pushBlock(BasicBlock* block);

	void popLoopBranchBlocks(int type);

	BasicBlock* currBlock() const;

	// NOTE: can only be used inside a function to add a new block
	BasicBlock* createBlock() const;

	BlockCount getBreakBlock(int level = 1) const;

	BasicBlock* createBreakBlock();

	BlockCount getContinueBlock(int level = 1) const;

	BasicBlock* createContinueBlock();

	BlockCount getRedoBlock(int level = 1) const;

	BasicBlock* createRedoBlock();

	LabelBlock* getLabelBlock(const string& name);

	LabelBlock* createLabelBlock(Token* name, bool isPlaceholder);
};

#endif
