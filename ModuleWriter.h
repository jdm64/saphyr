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

#ifndef __MODULE_WRITER_H__
#define __MODULE_WRITER_H__

#include <boost/program_options.hpp>
#include <llvm/PassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Target/TargetMachine.h>

using namespace std;
using namespace llvm;
using namespace boost::program_options;

class ModuleWriter
{
	Module& module;
	string filename;
	variables_map config;

	bool validModule();

	tool_output_file* getOutFile(const string& name);

	static void initTarget();

	TargetMachine* getMachine();

	void outputIR();

	void outputNative();

public:
	ModuleWriter(Module &module, string filename, variables_map& config)
	: module(module), filename(std::move(filename)), config(config) {}

	int run();
};

#endif
