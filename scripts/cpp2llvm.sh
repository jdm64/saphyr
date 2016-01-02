#!/usr/bin/env bash

# export LLVM_VER to use a non default llvm version

file=$(basename "$1")
fileext=${file##*.}
filename=${file%.*}

if [ $fileext == "c" ]; then
	clang$LLVM_VER -S -emit-llvm "$1"
else
	clang++$LLVM_VER -std=c++11 -S -emit-llvm "$1"
fi

llc$LLVM_VER -march=cpp "$filename".ll -o "$filename".ll.cpp
