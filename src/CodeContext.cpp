/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2020, Justin Madru (justin.jdm64@gmail.com)
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

#include <iostream>
#include "CodeContext.h"
#include "Instructions.h"

#define smart_label(block, token, placeholder) unique_ptr<LabelBlock>(new LabelBlock((block), (token), (placeholder)))

void ScopeTable::storeSymbol(const RValue& var, const string& name, bool isParam)
{
	table[name].push_back(var);
	if (!isParam && var.stype()->isDestructable()) {
		destructables.push_back(var);
	}
}

VecRValue ScopeTable::loadSymbol(const string& name) const
{
	auto varData = table.find(name);
	return varData != table.end()? varData->second : VecRValue();
}

VecRValue ScopeTable::getDestructables()
{
	return destructables;
}

bool GlobalContext::hasErrors() const
{
	return !errors.empty();
}

void GlobalContext::addError(const string& error, Token* token)
{
	if (token)
		errors.push_back({*token, error});
	else
		errors.push_back({Token(), error});
}

bool GlobalContext::handleErrors() const
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

NAttributeList* GlobalContext::storeAttr(NAttributeList* list)
{
	if (list) {
		list = list->move<NAttributeList>(false);
		attrs.push_back(unique_ptr<NAttributeList>(list));
	}
	return list;
}

void GlobalContext::pushFile(const path& filename)
{
	filesStack.push_back(filename);
	allFiles.insert(filename);
}

bool GlobalContext::fileLoaded(const path& filename)
{
	return allFiles.find(filename) != allFiles.end();
}

void CodeContext::validateFunction()
{
	for (auto& item : labelBlocks) {
		if (item.second->isPlaceholder)
			addError("label " + item.first + " not defined", &item.second->token);
	}
}

CodeContext::BlockCount CodeContext::loopBranchLevel(const BlockCountVec& branchBlocks, int level) const
{
	int blockCount = branchBlocks.size();
	int idx = level > 0? blockCount - level : -level - 1;
	return (idx >= 0 && idx < blockCount)? branchBlocks[idx] : make_pair(nullptr, 0);
}

CodeContext CodeContext::newForTemplate(CodeContext& context, const vector<pair<string, SType*>>& templateMappings)
{
	CodeContext newCtx(context.globalCtx);
	newCtx.templateArgs = templateMappings;
	return newCtx;
}

CodeContext::operator LLVMContext&()
{
	return globalCtx.module->getContext();
}

Module* CodeContext::getModule() const
{
	return globalCtx.module;
}

TypeManager& CodeContext::getTypeManager() const
{
	return globalCtx.typeManager;
}

IRBuilder<>& CodeContext::IB()
{
	return irBuilder;
}

bool CodeContext::hasErrors() const
{
	return globalCtx.hasErrors();
}

size_t CodeContext::errorCount() const
{
	return globalCtx.errors.size();
}

void CodeContext::addError(const string& error, Token* token)
{
	globalCtx.addError(error, token);
}

bool CodeContext::handleErrors() const
{
	return globalCtx.handleErrors();
}

NAttributeList* CodeContext::storeAttr(NAttributeList* list)
{
	return globalCtx.storeAttr(list);
}

void CodeContext::popFile()
{
	globalCtx.filesStack.pop_back();
}

void CodeContext::pushFile(const path& filename)
{
	globalCtx.pushFile(filename);
}

bool CodeContext::fileLoaded(const path& filename)
{
	return globalCtx.fileLoaded(filename);
}

path CodeContext::currFile() const
{
	return globalCtx.filesStack.back();
}

void CodeContext::storeGlobalSymbol(RValue var, const string& name)
{
	globalCtx.globalTable.storeSymbol(var, name);
}

VecRValue CodeContext::loadSymbolGlobal(const string& name) const
{
	return globalCtx.globalTable.loadSymbol(name);
}

NTemplatedDeclaration* CodeContext::getTemplate(const string& name)
{
	return globalCtx.typeManager.getTemplateType(name);
}

void CodeContext::storeTemplate(const string& name, NTemplatedDeclaration* decl)
{
	globalCtx.typeManager.storeTemplate(name, decl);
}

bool CodeContext::inTemplate() const
{
	return !templateArgs.empty();
}

SType* CodeContext::getTemplateArg(const string& name)
{
	auto it = find_if(templateArgs.begin(), templateArgs.end(),
		[&name](auto i){ return i.first == name; });
	return it != templateArgs.end() ? it->second : nullptr;
}

VecSType CodeContext::getTemplateArgs()
{
	VecSType args;
	transform(templateArgs.begin(), templateArgs.end(), back_inserter(args), [](auto i){ return i.second; });
	return args;
}

void CodeContext::pushLocalTable()
{
	localTable.push_back(ScopeTable());
}

void CodeContext::popLocalTable()
{
	auto toDestroy = localTable.back().getDestructables();
	localTable.pop_back();

	for (auto it = toDestroy.rbegin(); it != toDestroy.rend(); it++)
		Inst::CallDestructor(*this, *it, {}, nullptr);
}

void CodeContext::storeLocalSymbol(RValue var, const string& name, bool isParam)
{
	localTable.back().storeSymbol(var, name, isParam);
}

VecRValue CodeContext::loadSymbol(const string& name) const
{
	auto data = loadSymbolLocal(name);
	return data.size() ? data : globalCtx.globalTable.loadSymbol(name);
}

VecRValue CodeContext::loadSymbolLocal(const string& name) const
{
	for (auto it = localTable.rbegin(); it != localTable.rend(); it++) {
		auto data = it->loadSymbol(name);
		if (data.size())
			return data;
	}
	return {};
}

VecRValue CodeContext::loadSymbolCurr(const string& name) const
{
	return localTable.empty() ? globalCtx.globalTable.loadSymbol(name) : localTable.back().loadSymbol(name);
}

VecRValue CodeContext::getDestructables(size_t level)
{
	VecRValue ret;
	for (auto i = level; i < localTable.size(); i++) {
		auto des = localTable[i].getDestructables();
		ret.insert(ret.end(), des.begin(), des.end());
	}
	return ret;
}

SFunction CodeContext::currFunction() const
{
	return currFunc;
}

STemplatedType* CodeContext::getThis() const
{
	return thisType;
}

void CodeContext::setThis(STemplatedType* type)
{
	thisType = type;
}

SClassType* CodeContext::getClass() const
{
	return currClass;
}

void CodeContext::setClass(SClassType* classType)
{
	currClass = classType;
}

void CodeContext::startFuncBlock(SFunction function)
{
	pushLocalTable();
	funcBlocks.clear();
	funcBlocks.push_back(BasicBlock::Create(getModule()->getContext(), "", function));
	irBuilder.SetInsertPoint(currBlock());
	currFunc = function;
}

void CodeContext::endFuncBlock()
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

void CodeContext::pushBlock(BasicBlock* block)
{
	block->moveAfter(currBlock());
	funcBlocks.push_back(block);
	irBuilder.SetInsertPoint(currBlock());
}

void CodeContext::popLoopBranchBlocks(int type)
{
	if (type & BranchType::BREAK)
		breakBlocks.pop_back();
	if (type & BranchType::CONTINUE)
		continueBlocks.pop_back();
	if (type & BranchType::REDO)
		redoBlocks.pop_back();
}

BasicBlock* CodeContext::currBlock() const
{
	return funcBlocks.back();
}

// NOTE: can only be used inside a function to add a new block
BasicBlock* CodeContext::createBlock() const
{
	return BasicBlock::Create(getModule()->getContext(), "", currBlock()->getParent());
}

CodeContext::BlockCount CodeContext::getBreakBlock(int level) const
{
	return loopBranchLevel(breakBlocks, level);
}

BasicBlock* CodeContext::createBreakBlock()
{
	auto block = createBlock();
	breakBlocks.push_back({block, localTable.size()});
	return block;
}

CodeContext::BlockCount CodeContext::getContinueBlock(int level) const
{
	return loopBranchLevel(continueBlocks, level);
}

BasicBlock* CodeContext::createContinueBlock()
{
	auto block = createBlock();
	continueBlocks.push_back({block, localTable.size()});
	return block;
}

CodeContext::BlockCount CodeContext::getRedoBlock(int level) const
{
	return loopBranchLevel(redoBlocks, level);
}

BasicBlock* CodeContext::createRedoBlock()
{
	auto block = createBlock();
	redoBlocks.push_back({block, localTable.size()});
	return block;
}

LabelBlock* CodeContext::getLabelBlock(const string& name)
{
	return labelBlocks[name].get();
}

LabelBlock* CodeContext::createLabelBlock(Token* name, bool isPlaceholder)
{
	LabelBlockPtr &item = labelBlocks[name->str];
	if (!item.get()) {
		item = smart_label(createBlock(), name, isPlaceholder);
		item.get()->block->setName(name->str);
	}
	return item.get();
}
