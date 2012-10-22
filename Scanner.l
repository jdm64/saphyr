%baseclass-header "scannerbase.h"
%class-header "scanner.h"
%implementation-header "scanner.ih"
%lex-source "scanner.cpp"

SPACE		[ \t\n\r]
DIGIT		[0-9]
LETTER		[a-zA-Z_]

%%

{SPACE}+	;

"<<"	{ return ParserBase::TT_LSHIFT; }
">>"	{ return ParserBase::TT_RSHIFT; }
"<="	{ return ParserBase::TT_LEQ; }
">="	{ return ParserBase::TT_GEQ; }
"!="	{ return ParserBase::TT_NEQ; }
"=="	{ return ParserBase::TT_EQ; }
"&&"	{ return ParserBase::TT_LOG_AND; }
"||"	{ return ParserBase::TT_LOG_OR; }

true		{ return ParserBase::TT_TRUE; }
false		{ return ParserBase::TT_FALSE; }

void		{ return ParserBase::TT_VOID; }
bool		{ return ParserBase::TT_BOOL; }
int		{ return ParserBase::TT_INT; }
int8		{ return ParserBase::TT_INT8; }
int16		{ return ParserBase::TT_INT16; }
int32		{ return ParserBase::TT_INT32; }
int64		{ return ParserBase::TT_INT64; }
float		{ return ParserBase::TT_FLOAT; }
double		{ return ParserBase::TT_DOUBLE; }

return		{ return ParserBase::TT_RETURN; }

{DIGIT}+			{ SAVE_TOKEN return ParserBase::TT_INTEGER; }
{DIGIT}+'.'{DIGIT}+		{ SAVE_TOKEN return ParserBase::TT_FLOATING; }
{LETTER}+({LETTER}|{DIGIT})*	{ SAVE_TOKEN return ParserBase::TT_IDENTIFIER; }

.	{ return matched()[0]; }
