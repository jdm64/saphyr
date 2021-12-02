#!/usr/bin/env bash

rm -f scanner*
flexc++ Scanner.l

rm Scanner.ih

STYPE=$(grep "union STYPE" parserbase.h | cut -f2 -d' ')

PTR=$(grep "d_input->" scannerbase.h | wc -l)
if [ $PTR == "0" ]; then
	PTR="."
else
	PTR="->"
fi


sed -i -e '
/insert lexFunctionDecl/a\void setSval(ParserBase::'$STYPE'* dval){ sval = dval; } ParserBase::'$STYPE'* sval;
/nsert baseclass_h/a\#include "parserbase.h"
/insert lexFunctionDecl/a\virtual string getDisplayFName() = 0;
' scanner.h

sed -i -e '
/d_lineNr(1)/a ,d_col(1)
/d_lineNr(lineNr)/a ,d_col(1)
/++d_lineNr/acol_max = d_col; d_col = 0;
/default:/ad_col++;
/ch < 0x100/i--d_col;
/--d_lineNr;/c{ --d_lineNr; d_col = col_max; }
/$insert class_ih/a#include "scanner.h"
/#include "Scanner.ih"/c\#define SAVE_TOKEN sval->t_tok = new Token(matched(), getDisplayFName(), lineNr(), colNr() - matched().length());
' scanner.cpp

sed -i -e '
/size_t d_lineNr;/asize_t d_col; size_t col_max;
/size_t lineNr()/isize_t colNr() const { return d_col; }
/insert interactiveDecl/isize_t colNr() { return d_input'$PTR'colNr(); }
/ setFilename/i\public:
/ setFilename/a\protected:
' scannerbase.h
