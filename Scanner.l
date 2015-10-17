%baseclass-header "scannerbase.h"
%class-header "scanner.h"
%implementation-header "scanner.ih"
%lex-source "scanner.cpp"

SPACE		[ \t\n\r]
BIN		[01][']*
OCT		[0-7][']*
DEC		[0-9][']*
HEX		[0-9a-fA-F][']*
LETTER		[a-zA-Z_]
NUMBER		{DEC}+
NAME		{LETTER}({LETTER}|{DEC})*
FLOAT		{NUMBER}'.'{NUMBER}([eE][+-]?{NUMBER})?
SUFFIX		_{NAME}

%%

"/*"(.*|[\n\r])+"*/"	;
"//".*			;
{SPACE}+		;


"<<"	{ SAVE_TOKEN return ParserBase::TT_LSHIFT; }
">>"	{ SAVE_TOKEN return ParserBase::TT_RSHIFT; }
"<="	{ SAVE_TOKEN return ParserBase::TT_LEQ; }
">="	{ SAVE_TOKEN return ParserBase::TT_GEQ; }
"!="	{ SAVE_TOKEN return ParserBase::TT_NEQ; }
"=="	{ SAVE_TOKEN return ParserBase::TT_EQ; }
"&&"	{ SAVE_TOKEN return ParserBase::TT_LOG_AND; }
"||"	{ SAVE_TOKEN return ParserBase::TT_LOG_OR; }
"*="	{ SAVE_TOKEN return ParserBase::TT_ASG_MUL; }
"/="	{ SAVE_TOKEN return ParserBase::TT_ASG_DIV; }
"%="	{ SAVE_TOKEN return ParserBase::TT_ASG_MOD; }
"+="	{ SAVE_TOKEN return ParserBase::TT_ASG_ADD; }
"-="	{ SAVE_TOKEN return ParserBase::TT_ASG_SUB; }
"<<="	{ SAVE_TOKEN return ParserBase::TT_ASG_LSH; }
">>="	{ SAVE_TOKEN return ParserBase::TT_ASG_RSH; }
"&="	{ SAVE_TOKEN return ParserBase::TT_ASG_AND; }
"^="	{ SAVE_TOKEN return ParserBase::TT_ASG_OR; }
"|="	{ SAVE_TOKEN return ParserBase::TT_ASG_XOR; }
"++"	{ SAVE_TOKEN return ParserBase::TT_INC; }
"--"	{ SAVE_TOKEN return ParserBase::TT_DEC; }
"??"	{ SAVE_TOKEN return ParserBase::TT_DQ_MARK; }
"??="	{ SAVE_TOKEN return ParserBase::TT_ASG_DQ; }

true		{ SAVE_TOKEN return ParserBase::TT_TRUE; }
false		{ SAVE_TOKEN return ParserBase::TT_FALSE; }
null		{ SAVE_TOKEN return ParserBase::TT_NULL; }

auto		{ SAVE_TOKEN return ParserBase::TT_AUTO; }
void		{ SAVE_TOKEN return ParserBase::TT_VOID; }
bool		{ SAVE_TOKEN return ParserBase::TT_BOOL; }
int		{ SAVE_TOKEN return ParserBase::TT_INT; }
int8		{ SAVE_TOKEN return ParserBase::TT_INT8; }
int16		{ SAVE_TOKEN return ParserBase::TT_INT16; }
int32		{ SAVE_TOKEN return ParserBase::TT_INT32; }
int64		{ SAVE_TOKEN return ParserBase::TT_INT64; }
uint		{ SAVE_TOKEN return ParserBase::TT_UINT; }
uint8		{ SAVE_TOKEN return ParserBase::TT_UINT8; }
uint16		{ SAVE_TOKEN return ParserBase::TT_UINT16; }
uint32		{ SAVE_TOKEN return ParserBase::TT_UINT32; }
uint64		{ SAVE_TOKEN return ParserBase::TT_UINT64; }
float		{ SAVE_TOKEN return ParserBase::TT_FLOAT; }
double		{ SAVE_TOKEN return ParserBase::TT_DOUBLE; }

alias		{ return ParserBase::TT_ALIAS; }
break		{ SAVE_TOKEN return ParserBase::TT_BREAK; }
case		{ SAVE_TOKEN return ParserBase::TT_CASE; }
continue	{ SAVE_TOKEN return ParserBase::TT_CONTINUE; }
default		{ SAVE_TOKEN return ParserBase::TT_DEFAULT; }
delete		{ SAVE_TOKEN return ParserBase::TT_DELETE; }
do		{ return ParserBase::TT_DO; }
else		{ return ParserBase::TT_ELSE; }
enum		{ return ParserBase::TT_ENUM; }
for		{ return ParserBase::TT_FOR; }
goto		{ return ParserBase::TT_GOTO; }
if		{ return ParserBase::TT_IF; }
loop		{ return ParserBase::TT_LOOP; }
new		{ SAVE_TOKEN return ParserBase::TT_NEW; }
redo		{ SAVE_TOKEN return ParserBase::TT_REDO; }
return		{ SAVE_TOKEN return ParserBase::TT_RETURN; }
sizeof		{ SAVE_TOKEN return ParserBase::TT_SIZEOF; }
struct		{ return ParserBase::TT_STRUCT; }
switch		{ return ParserBase::TT_SWITCH; }
union		{ return ParserBase::TT_UNION; }
until		{ return ParserBase::TT_UNTIL; }
vec		{ return ParserBase::TT_VEC; }
while		{ return ParserBase::TT_WHILE; }

0b{BIN}+{SUFFIX}?	{ SAVE_TOKEN return ParserBase::TT_INT_BIN; }
0o{OCT}+{SUFFIX}?	{ SAVE_TOKEN return ParserBase::TT_INT_OCT; }
0x{HEX}+{SUFFIX}?	{ SAVE_TOKEN return ParserBase::TT_INT_HEX; }
{NUMBER}{SUFFIX}?	{ SAVE_TOKEN return ParserBase::TT_INTEGER; }
{FLOAT}{SUFFIX}?	{ SAVE_TOKEN return ParserBase::TT_FLOATING; }
{NAME}			{ SAVE_TOKEN return ParserBase::TT_IDENTIFIER; }

\'(\\.|[^'])\'	{ SAVE_TOKEN return ParserBase::TT_CHAR_LIT; }
\"(\\.|[^"])*\"	{ SAVE_TOKEN return ParserBase::TT_STR_LIT; }
`(\\.|[^`])*`	{ SAVE_TOKEN return ParserBase::TT_STR_LIT; }
.		{ SAVE_TOKEN return matched()[0]; }
