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
#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <llvm/Function.h>
#include "Type.h"

class SFunction
{
	friend class FunctionManager;

	Function* func;
	SFunctionType* sfuncTy;

	SFunction(Function* function, SFunctionType* type)
	: func(function), sfuncTy(type) {};

public:
	static SFunction* create(CodeContext& context, string* name, SFunctionType* type);

	operator Function*() const
	{
		return func;
	}

	int size() const
	{
		return func->size();
	}

	StringRef name() const
	{
		return func->getName();
	}

	SFunctionType* stype() const
	{
		return sfuncTy;
	}

	SType* returnTy() const
	{
		return sfuncTy->returnTy();
	}

	Function::arg_iterator arg_begin() const
	{
		return func->arg_begin();
	}

	Function::arg_iterator arg_end() const
	{
		return func->arg_end();
	}
};

#define smart_sfunc(func, type) unique_ptr<SFunction>(new SFunction(func, type))

class FunctionManager
{
	using SFuncPtr = unique_ptr<SFunction>;
	using SFuncTyPtr = unique_ptr<SFunctionType>;

	Module* module;

	map<string, SFuncPtr> table;
	SFunction* curr;

public:
	FunctionManager(Module* module)
	: module(module), curr(nullptr) {}

	SFunction* current() const
	{
		return curr;
	}

	void setCurrent(SFunction* function)
	{
		curr = function;
	}

	SFunction* getFunction(string* name)
	{
		return table[*name].get();
	}

	SFunction* create(string* name, SFunctionType* type)
	{
		SFuncPtr &item = table[*name];
		if (!item.get()) {
			auto func = Function::Create(*type, GlobalValue::ExternalLinkage, *name, module);
			item = smart_sfunc(func, type);
		}
		return item.get();
	}
};

#endif
