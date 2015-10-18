/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2014, Justin Madru (justin.jdm64@gmail.com)
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

#include "parser.h"
#include "AST.h"
#include "CodeContext.h"
#include "ModuleWriter.h"

options_description progOpts;

void initOptions()
{
	progOpts.add_options()
		("help", "produce help message")
		("input", "input file")
		("llvmir", "output LLVM IR instead of object code");
}

void loadOptions(int argc, char** argv, variables_map &vm)
{
	positional_options_description fileOpt;
	fileOpt.add("input", -1);

	store(command_line_parser(argc, argv).options(progOpts).positional(fileOpt).run(), vm);
	notify(vm);
}

int main(int argc, char** argv)
{
	variables_map vm;

	initOptions();
	loadOptions(argc, argv, vm);

	if (vm.count("help")) {
		progOpts.print(cout);
		return 0;
	} else if (!vm.count("input")) {
		cout << "no input file provided" << endl;
		return 1;
	}

	string file = vm["input"].as<string>();
	Parser parser(file);

	if (parser.parse()) {
		return 1;
	}

	unique_ptr<Module> module(new Module(file, getGlobalContext()));
	CodeContext context(module.get());

	parser.getRoot()->genCode(context);
	if (context.handleErrors(file))
		return 2;

	ModuleWriter writer(*module.get(), file, vm);

	return writer.run();
}
