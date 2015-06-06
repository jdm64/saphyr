#!/usr/bin/env bash

file=$(basename "$1")
fileext=${file##*.}
filename=${file%.*}

if [ $fileext == "c" ]; then
	clang -S -emit-llvm "$1"
else
	clang++ -std=c++11 -S -emit-llvm "$1"
fi

llc -march=cpp "$filename".ll -o "$filename".ll.cpp
