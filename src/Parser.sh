#!/usr/bin/env bash

rm -f parser*
bisonc++ -V Parser.y

sed -i -e '
/Scanner d_scanner;/c\	unique_ptr<Scanner> d_scanner;
/scannerobject/a\	unique_ptr<NStatementList> root;
/public:/a\	Token getError() { string token = d_scanner->matched().size()? d_scanner->matched() : "<EOF>"; return Token("Syntax error on: " + token, d_scanner->filename(), d_scanner->lineNr(), d_scanner->colNr()); }
/public:/a\	NStatementList* getRoot() { return root.get(); }
/public:/a\	Parser(string filename){ d_scanner = unique_ptr<Scanner>(new Scanner(filename, "-")); d_scanner->setSval(&d_val__); }
' parser.h

sed -i -e '
/return d_scanner.lex();/c\	return d_scanner->lex();
/Syntax error/d
' parser.ih

sed -i -e '
/Syntax error/d
' parser.cpp
