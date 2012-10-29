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
"*="	{ return ParserBase::TT_ASG_MUL; }
"/="	{ return ParserBase::TT_ASG_DIV; }
"%="	{ return ParserBase::TT_ASG_MOD; }
"+="	{ return ParserBase::TT_ASG_ADD; }
"-="	{ return ParserBase::TT_ASG_SUB; }
"<<="	{ return ParserBase::TT_ASG_LSH; }
">>="	{ return ParserBase::TT_ASG_RSH; }
"&="	{ return ParserBase::TT_ASG_AND; }
"^="	{ return ParserBase::TT_ASG_OR; }
"|="	{ return ParserBase::TT_ASG_XOR; }
"++"	{ return ParserBase::TT_INC; }
"--"	{ return ParserBase::TT_DEC; }

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

break		{ return ParserBase::TT_BREAK; }
continue	{ return ParserBase::TT_CONTINUE; }
do		{ return ParserBase::TT_DO; }
for		{ return ParserBase::TT_FOR; }
redo		{ return ParserBase::TT_REDO; }
return		{ return ParserBase::TT_RETURN; }
until		{ return ParserBase::TT_UNTIL; }
while		{ return ParserBase::TT_WHILE; }

{DIGIT}+			{ SAVE_TOKEN return ParserBase::TT_INTEGER; }
{DIGIT}+'.'{DIGIT}+		{ SAVE_TOKEN return ParserBase::TT_FLOATING; }
{LETTER}+({LETTER}|{DIGIT})*	{ SAVE_TOKEN return ParserBase::TT_IDENTIFIER; }

.	{ return matched()[0]; }
