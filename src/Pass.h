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
#ifndef __PASS_H__
#define __PASS_H__

#include <llvm/Pass.h>

using namespace llvm;

class SimpleBlockClean : public FunctionPass
{
	bool removeBranchBlock(BasicBlock* block);

public:
	static char ID;

	SimpleBlockClean()
	: FunctionPass(ID) {}

	const char* getPassName() const
	{
		return "SimpleBlockClean";
	}

	bool runOnFunction(Function &func);
};

#endif
