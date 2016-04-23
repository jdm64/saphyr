#!/usr/bin/env bash

LLVM_VERSION=$(grep "LLVM_VER" Configfile 2> /dev/null | cut -f2 -d'=' | xargs)
if [[ -z "${LLVM_VERSION}" ]]; then
	LLVM_VERSION=$LLVM_VER
fi

VERSION=$(llvm-config$LLVM_VERSION --version)
VER_ARR=(${VERSION//./ })

cat << 'EOF'
#ifndef __LLVM_DEFINES_H__
#define __LLVM_DEFINES_H__

EOF

if (( ${VER_ARR[0]} >= 3 && ${VER_ARR[1]} >= 5 )); then
	if (( ${VER_ARR[1]} >= 8 )); then
		echo "#include <llvm/Config/llvm-config.h>"
	else
		echo "#include <llvm/Config/config.h>"
	fi

	if (( ${VER_ARR[1]} >= 7 )); then
		echo "#define _LLVM_IR_PASS_MANAGER_H <llvm/IR/LegacyPassManager.h>"
	else
		echo "#define _LLVM_IR_PASS_MANAGER_H <llvm/PassManager.h>"
	fi

cat << 'EOF'
#define _LLVM_IR_VERIFIER_H <llvm/IR/Verifier.h>
#define _LLVM_IR_PRINTING_PASSES_H <llvm/IR/IRPrintingPasses.h>
#define _LLVM_IR_CFG_H <llvm/IR/CFG.h>
EOF

else

cat << 'EOF'
#include <llvm/Config/config.h>
#define _LLVM_IR_PASS_MANAGER_H <llvm/PassManager.h>
#define _LLVM_IR_VERIFIER_H <llvm/Analysis/Verifier.h>
#define _LLVM_IR_PRINTING_PASSES_H <llvm/Assembly/PrintModulePass.h>
#define _LLVM_IR_CFG_H <llvm/Support/CFG.h>
EOF

fi

echo
echo "#endif"
