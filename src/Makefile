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
else
	COVERAGE = false
endif

# location to install
ifeq ($(INSTALL_DIR),)
	INSTALL_DIR = /usr/local/bin/
endif

# set LLVM_VER to build with a non-default llvm version
# example: export LLVM_VER="-3.5"

WARNINGS = -Wall -Wextra -pedantic -Wno-unused-parameter
CXXFLAGS = -std=c++11 `llvm-config$(LLVM_VER) --cxxflags` $(O_LEVEL) $(COV_CXX) $(WARNINGS) -frtti -fexceptions -D__STRICT_ANSI__ -DCOPY_NODES=$(COVERAGE)
LDFLAGS = -lboost_program_options -lboost_filesystem -lboost_system $(COV_LD)
COMPILER_LDFLAGS = $(LDFLAGS) `llvm-config$(LLVM_VER) --ldflags` -lLLVM-`llvm-config$(LLVM_VER) --version`
COMPILER = ../saphyr
FORMATTER = ../syfmt

objs = parser.o scanner.o Util.o

compiler_objs = $(objs) Type.o Value.o Instructions.o Builder.o CGNDataType.o \
	CGNVariable.o CGNExpression.o CGNStatement.o CGNImportStm.o Pass.o ModuleWriter.o \
	CGNImportList.o main.o

fmt_objs = $(objs) format/WriterUtil.o format/FMNDataType.o format/FMNExpression.o \
	format/FMNStatement.o format/fmtMain.o

all : formatter compiler

compiler : frontend $(compiler_objs)
	$(CXX) $(compiler_objs) -o $(COMPILER) $(COMPILER_LDFLAGS)

formatter : frontend $(fmt_objs)
	$(CXX) $(fmt_objs) -o $(FORMATTER) $(LDFLAGS)

frontend : parser.cpp scanner.cpp

frontend-docker :
	sudo docker run --rm -v $(PWD):/usr/src/saphyr -w /usr/src/saphyr jdm64/saphyr make frontend

parser.cpp : Parser.y
	./Parser.sh

scanner.cpp : Scanner.l parser.cpp
	./Scanner.sh

clean :
	rm -f $(COMPILER) $(FORMATTER) *.o *~ format/*.o format/*~

frontend-clean :
	rm -f parser* scanner*

fullclean : clean frontend-clean

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

cppcheck :
	cppcheck --enable=all --inconclusive ./ 1> /dev/null

analyze : clean
	scan-build$(LLVM_VER) make compiler

tests : all
	cd ../tests; ./unitTest.py $(UNITTEST_ARG)

docker-dev :
	sudo docker run -it --rm -v $(PWD)/../:/usr/src/saphyr -w /usr/src/saphyr/src jdm64/saphyr bash

appimage :
	sudo docker run -it --rm -v $(PWD)/../:/usr/src/saphyr -w /usr/src/saphyr/src jdm64/saphyr make appimage-filecopy
	cd ../scripts; ./buildAppImage-create.sh

appimage-filecopy : compiler
	cd ../scripts; ./buildAppImage-filecopy.sh

install : all
	sudo cp $(COMPILER) $(FORMATTER) $(INSTALL_DIR)
