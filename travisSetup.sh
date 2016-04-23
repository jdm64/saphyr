#!/usr/bin/env bash

if [[ $CXX == "g++" ]]; then
	CXX_PKG="g++"
fi

sudo cp /etc/apt/sources.list /etc/apt/sources.list.d/trusty.list
sudo sed -i "s/precise/trusty/g" /etc/apt/sources.list.d/trusty.list

if [[ $LLVM_VER == "-3.8" ]]; then
	echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.8 main" | sudo tee /etc/apt/sources.list.d/llvm38.list > /dev/null
	echo "UNITTEST_ARG = +-3.8" > src/Configfile
elif [[ $LLVM_VER == "-3.7" ]]; then
	echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.7 main" | sudo tee /etc/apt/sources.list.d/llvm37.list > /dev/null
	echo "UNITTEST_ARG = +-3.7" > src/Configfile
fi

sudo apt-get update -qq
sudo apt-get install -qq -y --force-yes flexc++ bisonc++ llvm$LLVM_VER-dev $CXX_PKG libboost-program-options-dev python3

if [[ $COVERAGE == "true" ]]; then
	sudo pip install cpp-coveralls
fi
