#!/usr/bin/env bash

if [[ $CXX == "g++" ]]; then
	CXX_PKG="g++"
fi

if [[ $LLVM_VER == "-3.8" ]]; then
	echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.8 main" | sudo tee /etc/apt/sources.list.d/llvm38.list > /dev/null
	echo "UNITTEST_ARG = +-3.8" > src/Configfile
elif [[ $LLVM_VER == "-3.9" ]]; then
	echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.9 main" | sudo tee /etc/apt/sources.list.d/llvm39.list > /dev/null
	echo "UNITTEST_ARG = +-3.9" > src/Configfile
elif [[ $LLVM_VER == "-4.0" ]]; then
	echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-4.0 main" | sudo tee /etc/apt/sources.list.d/llvm40.list > /dev/null
	echo "UNITTEST_ARG = +-4.0" > src/Configfile
fi

sudo apt-get update -qq
sudo apt-get install -qq -y --force-yes flexc++ bisonc++ llvm$LLVM_VER-dev $CXX_PKG libboost-program-options-dev libboost-filesystem-dev python3 binutils

if [[ $COVERAGE == "true" ]]; then
	sudo pip install -U cpp-coveralls
fi
