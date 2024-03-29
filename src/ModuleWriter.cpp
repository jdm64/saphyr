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

#include "ModuleWriter.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/ADT/Triple.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Target/TargetOptions.h>

#include "Pass.h"

using namespace llvm::legacy;

bool ModuleWriter::validModule()
{
	ostringstream buff;
	raw_os_ostream out(buff);
	if (verifyModule(module, &out)) {
		cout << "compiler error: broken module" << endl << endl
			<< buff.str() << endl;
		return true;
	}
	return false;
}

void ModuleWriter::initTarget()
{
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmPrinters();
	InitializeAllAsmParsers();
}

TargetMachine* ModuleWriter::getMachine()
{
	TargetOptions options;
	string err;
	string features;
	Triple triple;

	triple.setTriple(sys::getDefaultTargetTriple());
	auto target = TargetRegistry::lookupTarget(triple.getTriple(), err);
	return target->createTargetMachine(triple.getTriple(), sys::getHostCPUName(), features, options, Reloc::Model::Static, CodeModel::Medium, CodeGenOpt::Default);
}

int ModuleWriter::run()
{
	auto noClean = config.count("noclean");
	if (!noClean) {
		llvm::legacy::PassManager clean;
		clean.add(new SimpleBlockClean());
		clean.run(module);
	}

	auto noVerify = noClean || config.count("noverify");
	if (noVerify || config.count("llvmir"))
		outputIR();

	auto hasErrors = noVerify ? true : validModule();
	if (!hasErrors)
		outputNative();

	return hasErrors;
}

void ModuleWriter::outputIR()
{
	llvm::legacy::PassManager pm;

	fstream irFile(filename.substr(0, filename.rfind('.')) + ".ll", fstream::out);
	raw_os_ostream irStream(irFile);

	pm.add(createPrintModulePass(irStream));

	pm.run(module);
}

void ModuleWriter::outputNative()
{
	llvm::legacy::PassManager pm;

	initTarget();

	error_code error;
	auto oFile = filename.substr(0, filename.rfind('.')) + ".o";
	raw_fd_ostream objStream(oFile, error);
	if (error) {
		cout << "file error: " << error.message() << ": " << oFile << endl;
		return;
	}

	unique_ptr<TargetMachine> machine(getMachine());
#if LLVM_VERSION_MAJOR >= 10
	machine->addPassesToEmitFile(pm, objStream, nullptr, CGFT_ObjectFile);
#else
	machine->addPassesToEmitFile(pm, objStream, nullptr, TargetMachine::CGFT_ObjectFile);
#endif

	pm.run(module);
}
