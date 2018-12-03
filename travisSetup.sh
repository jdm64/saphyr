#!/usr/bin/env bash

if [[ $CXX == "g++" ]]; then
	CXX_PKG="g++"
fi

echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-4.0 main" | sudo tee /etc/apt/sources.list.d/llvm40.list > /dev/null
echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-5.0 main" | sudo tee /etc/apt/sources.list.d/llvm50.list > /dev/null
echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-6.0 main" | sudo tee /etc/apt/sources.list.d/llvm60.list > /dev/null
echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-7   main" | sudo tee /etc/apt/sources.list.d/llvm70.list > /dev/null

wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -

OTHER_PKGS=""

if [[ $LLVM_VER == "-3.8" ]] || [[ $LLVM_VER == "-4.0" ]]; then
	VER=${LLVM_VER:1}
	echo "UNITTEST_ARG = +$VER-5.0" > src/Configfile
	OTHER_PKGS="llvm-5.0-dev"
elif [[ $LLVM_VER == "-6.0" ]] || [[ $LLVM_VER == "-7" ]]; then
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
fi

sudo apt-get update -qq
sudo apt-get install -qq -y flexc++ bisonc++ llvm$LLVM_VER-dev $CXX_PKG libboost-program-options-dev libboost-filesystem-dev python3 binutils chrpath $OTHER_PKGS

if [[ $COVERAGE == "true" ]]; then
	sudo pip install -U cpp-coveralls
fi
