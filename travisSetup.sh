#!/usr/bin/env bash

if [[ $LLVM_VER == "-3.8" ]] || [[ $LLVM_VER == "-4.0" ]]; then
	VER=${LLVM_VER:1}
	echo "UNITTEST_ARG = +$VER-5.0" > src/Configfile
fi

if [[ $COVERAGE == "true" ]]; then
	sudo pip install -U cpp-coveralls
fi
