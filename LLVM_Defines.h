#ifndef __LLVM_DEFINES_H__
#define __LLVM_DEFINES_H__

#include <llvm/Config/config.h>

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5
	#define _LLVM_IR_VERIFIER_H <llvm/IR/Verifier.h>
	#define _LLVM_IR_PRINTING_PASSES_H <llvm/IR/IRPrintingPasses.h>
	#define _LLVM_IR_CFG_H <llvm/IR/CFG.h>
#else
	#define _LLVM_IR_VERIFIER_H <llvm/Analysis/Verifier.h>
	#define _LLVM_IR_PRINTING_PASSES_H <llvm/Assembly/PrintModulePass.h>
	#define _LLVM_IR_CFG_H <llvm/Support/CFG.h>
#endif

#endif
