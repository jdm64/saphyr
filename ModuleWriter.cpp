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

#include "ModuleWriter.h"
#include "LLVM_Defines.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <llvm/PassManager.h>
#include _LLVM_IR_PRINTING_PASSES_H
#include _LLVM_IR_VERIFIER_H
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>

#include "Pass.h"

bool ModuleWriter::validModule()
{
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5
	ostringstream buff;
	raw_os_ostream out(buff);
	if (verifyModule(module, &out)) {
		cout << "compiler error: broken module" << endl << endl
			<< buff.str() << endl;
		return true;
	}
#else
	string err;
	if (verifyModule(module, ReturnStatusAction, &err)) {
		cout << "compiler error: broken module" << endl << endl
			<< err << endl;
		return true;
	}
#endif
	return false;
}

int ModuleWriter::run()
{
	PassManager clean;
	clean.add(new SimpleBlockClean());
	clean.run(module);

	if (validModule())
		return 1;

	PassManager pm;

	fstream irFile(filename.substr(0, filename.rfind('.')) + ".ll", fstream::out);
	raw_os_ostream irStream(irFile);

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5
	pm.add(createPrintModulePass(irStream));
#else
	pm.add(createPrintModulePass(&irStream));
#endif

	pm.run(module);

	return 0;
}
