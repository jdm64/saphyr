#!/usr/bin/env bash

rm -f parser*
bisonc++ -V Parser.y

rm parser.ih

sed -i -e '
/d_scanner;/d
/public:/a\	virtual void setRoot(NStatementList* ptr) = 0;
/void error();/c\	void error() {}
/int lex();/c\	virtual int lex() = 0;
/void print();/c\	void print() {}
/void exceptionHandler(std::exception const &exc);/c\	void exceptionHandler(std::exception const &exc) { throw exc; }
' parser.h

sed -i -e '
/Syntax error/d
/$insert class.ih/a\#include "BaseNodes.h"
/#include "parser.ih"/c\#include "parser.h"
' parser.cpp

DVAL=$(grep d_val_ parserbase.h | tr -s ' ;' ' ' | cut -f3 -d' ')
SVAL=$(grep d_val_ parserbase.h | tr -s ' ;' ' ' | cut -f2 -d' ')

sed -i -e '
/'$DVAL';/a\	'$SVAL'* getSval() { return &'$DVAL'; }
' parserbase.h
