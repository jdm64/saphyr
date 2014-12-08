CXX = g++
WARNINGS = -Wall -Wextra -pedantic -Wno-unused-parameter
CXXFLAGS = -std=gnu++0x `llvm-config --cxxflags` -ggdb -O3 $(WARNINGS) -fexceptions -D__STRICT_ANSI__
LDFLAGS = `llvm-config --ldflags` -lLLVM-`llvm-config --version`
TARGET = saphyr

objs = scanner.o parser.o Type.o Value.o Function.o Instructions.o GenCode.o Pass.o main.o

compiler : $(objs)
	$(CXX) $(objs) -o $(TARGET) $(LDFLAGS)

parser.cpp : Parser.y
	rm -f parser*
	bisonc++ -V Parser.y
	sed -i -e '/define ParserBase_h_included/a\#include "AST.h"' parserbase.h
	sed -i -e '/Scanner d_scanner;/c\	Scanner* d_scanner;' parser.h
	sed -i -e '/public:/a\	Parser(string filename){ d_scanner = new Scanner(filename, "-"); d_scanner->setSval(&d_val__); }' parser.h
	sed -i -e '/return d_scanner.lex();/c\	return d_scanner->lex();' parser.ih

scanner.cpp : Scanner.l parser.cpp
	rm -f scanner*
	flexc++ Scanner.l
	sed -i -e '/insert lexFunctionDecl/a\void setSval(ParserBase::STYPE__ *dval){ sval = dval; } ParserBase::STYPE__* sval;' scanner.h
	sed -i -e '/nsert baseclass_h/a\#include "parserbase.h"' scanner.h
	sed -i -e '/insert class_h/a\#include "parserbase.h"' scanner.ih
	sed -i -e '/insert class_h/a\#define SAVE_TOKEN sval->t_str = new std::string(matched());' scanner.ih

clean :
	rm -f $(TARGET) *.o *~

fullclean : clean
	rm -f parser* scanner*

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

cppcheck :
	cppcheck --enable=all --inconclusive ./ 1> /dev/null

analyze : clean
	scan-build make compiler

tests : compiler
	cd tests; ./unitTest.py
