#!/usr/bin/env bash

rm -f parser*
bisonc++ -V Parser.y

DVAL=$(grep d_val_ parserbase.h | tr -s ' ;' ' ' | cut -f3 -d' ')

sed -i -e '
/Scanner d_scanner;/c\	uPtr<Scanner> d_scanner;
/scannerobject/a\	uPtr<NStatementList> root;
/public:/a\	Token getError() { string token = d_scanner->matched().size()? d_scanner->matched() : "<EOF>"; return Token("Syntax error on: " + token, d_scanner->filename(), d_scanner->lineNr(), d_scanner->colNr()); }
/public:/a\	NStatementList* getRoot() { return root.get(); }
/public:/a\	Parser(string filename){ d_scanner = uPtr<Scanner>(new Scanner(filename, "-")); d_scanner->setSval(&'"$DVAL"'); }
/public:/a\	void setFilename(string name) { d_scanner->setFilename(name); }
' parser.h

sed -i -e '
/return d_scanner.lex();/c\	return d_scanner->lex();
/Syntax error/d
' parser.ih

sed -i -e '
/Syntax error/d
' parser.cpp
