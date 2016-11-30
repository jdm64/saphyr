-include Configfile

# only set CXX if it's not defined
ifeq "$(origin CXX)" "default"
	CXX = clang++
endif

# build in release mode unless debug is true
ifeq ($(DEBUG),true)
	O_LEVEL = -ggdb -O0
else
	O_LEVEL = -O3
endif

# set coverage only if using g++
ifeq ($(COVERAGE),true)
	COV_CXX = --coverage
	COV_LD  = -coverage
endif

# set LLVM_VER to build with a non-default llvm version
# example: export LLVM_VER="-3.5"

WARNINGS = -Wall -Wextra -pedantic -Wno-unused-parameter
CXXFLAGS = -std=c++11 `llvm-config$(LLVM_VER) --cxxflags` $(O_LEVEL) $(COV_CXX) $(WARNINGS) -frtti -fexceptions -D__STRICT_ANSI__
LDFLAGS = -lboost_program_options `llvm-config$(LLVM_VER) --ldflags` -lLLVM-`llvm-config$(LLVM_VER) --version` $(COV_LD)
TARGET = ../saphyr

objs = parser.o scanner.o Type.o Value.o Instructions.o Builder.o CGNInt.o CGNDataType.o CGNVariable.o CGNExpression.o CGNStatement.o CGNImportStm.o Pass.o ModuleWriter.o main.o

compiler : LLVM_Defines.sh frontend $(objs)
	$(CXX) $(objs) -o $(TARGET) $(LDFLAGS)

LLVM_Defines.sh : LLVM_Defines.h

LLVM_Defines.h :
	./LLVM_Defines.sh > LLVM_Defines.h

frontend : parser.cpp scanner.cpp

frontend-docker :
	sudo docker run --rm -v $(PWD):/usr/src/saphyr -w /usr/src/saphyr jdm64/saphyr make frontend

parser.cpp : Parser.y
	rm -f parser*
	bisonc++ -V Parser.y
	sed -i -e '/Scanner d_scanner;/c\	unique_ptr<Scanner> d_scanner;' parser.h
	sed -i -e '/scannerobject/a\	unique_ptr<NStatementList> root;' parser.h
	sed -i -e '/public:/a\	NStatementList* getRoot() { return root.get(); }' parser.h
	sed -i -e '/public:/a\	Parser(string filename){ d_scanner = unique_ptr<Scanner>(new Scanner(filename, "-")); d_scanner->setSval(&d_val__); }' parser.h
	sed -i -e '/return d_scanner.lex();/c\	return d_scanner->lex();' parser.ih
	sed -i -e '/error("Syntax error");/c\	error(("Syntax error: line " + to_string(d_scanner->lineNr())).c_str());' parser.cpp

scanner.cpp : Scanner.l parser.cpp
	rm -f scanner*
	flexc++ Scanner.l
	sed -i -e '/insert lexFunctionDecl/a\void setSval(ParserBase::STYPE__ *dval){ sval = dval; } ParserBase::STYPE__* sval;' scanner.h
	sed -i -e '/nsert baseclass_h/a\#include "parserbase.h"' scanner.h
	sed -i -e '/insert class_h/a\#include "parserbase.h"' scanner.ih
	sed -i -e '/insert class_h/a\#define SAVE_TOKEN sval->t_tok = new Token(matched(), filename(), lineNr());' scanner.ih

clean :
	rm -f $(TARGET) *.o *~

frontend-clean :
	rm -f parser* scanner*

fullclean : clean frontend-clean
	rm -f LLVM_Defines.h

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

cppcheck :
	cppcheck --enable=all --inconclusive ./ 1> /dev/null

analyze : clean
	scan-build make compiler

tests : compiler
	cd ../tests; ./unitTest.py $(UNITTEST_ARG)

docker-dev :
	sudo docker run -it --rm -v $(PWD)/../:/usr/src/saphyr -w /usr/src/saphyr/src jdm64/saphyr bash
