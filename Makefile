CXX = g++
WARNINGS = -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-sign-compare
CXXFLAGS = -std=gnu++0x `llvm-config --cxxflags` -ggdb -O3 $(WARNINGS) -fexceptions -D__STRICT_ANSI__
LDFLAGS = -lLLVM-`llvm-config --version`

objs = scanner.o parser.o Type.o Function.o Instructions.o GenCode.o Pass.o main.o

compiler : $(objs)
	$(CXX) $(objs) -o saphyr $(LDFLAGS)

parser :
	rm -f scanner* parser*
	flexc++ Scanner.l
	bisonc++ -V Parser.y
	sed -i -e '/insert lexFunctionDecl/a\void setSval(ParserBase::STYPE__ *dval){ sval = dval; } ParserBase::STYPE__* sval;' scanner.h
	sed -i -e '/nsert baseclass_h/a\#include "parserbase.h"' scanner.h
	sed -i -e '/insert class_h/a\#include "parserbase.h"' scanner.ih
	sed -i -e '/insert class_h/a\#define SAVE_TOKEN sval->t_str = new std::string(matched());' scanner.ih
	sed -i -e '/define ParserBase_h_included/a\#include "AST.h"' parserbase.h
	sed -i -e '/Scanner d_scanner;/c\	Scanner* d_scanner;' parser.h
	sed -i -e '/public:/a\	Parser(string filename){ d_scanner = new Scanner(filename, "-"); d_scanner->setSval(&d_val__); }' parser.h
	sed -i -e '/return d_scanner.lex();/c\	return d_scanner->lex();' parser.ih

clean :
	rm -f saphyr *.o *~

fullclean : clean
	rm -f parser* scanner*

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
