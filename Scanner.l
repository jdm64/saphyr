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
"??"	{ return ParserBase::TT_DQ_MARK; }
"??="	{ return ParserBase::TT_ASG_DQ; }

true		{ SAVE_TOKEN return ParserBase::TT_TRUE; }
false		{ SAVE_TOKEN return ParserBase::TT_FALSE; }
null		{ SAVE_TOKEN return ParserBase::TT_NULL; }

auto		{ return ParserBase::TT_AUTO; }
void		{ return ParserBase::TT_VOID; }
bool		{ return ParserBase::TT_BOOL; }
int		{ return ParserBase::TT_INT; }
int8		{ return ParserBase::TT_INT8; }
int16		{ return ParserBase::TT_INT16; }
int32		{ return ParserBase::TT_INT32; }
int64		{ return ParserBase::TT_INT64; }
uint		{ return ParserBase::TT_UINT; }
uint8		{ return ParserBase::TT_UINT8; }
uint16		{ return ParserBase::TT_UINT16; }
uint32		{ return ParserBase::TT_UINT32; }
uint64		{ return ParserBase::TT_UINT64; }
float		{ return ParserBase::TT_FLOAT; }
double		{ return ParserBase::TT_DOUBLE; }

alias		{ return ParserBase::TT_ALIAS; }
break		{ return ParserBase::TT_BREAK; }
case		{ return ParserBase::TT_CASE; }
continue	{ return ParserBase::TT_CONTINUE; }
default		{ return ParserBase::TT_DEFAULT; }
delete		{ return ParserBase::TT_DELETE; }
do		{ return ParserBase::TT_DO; }
else		{ return ParserBase::TT_ELSE; }
enum		{ return ParserBase::TT_ENUM; }
for		{ return ParserBase::TT_FOR; }
goto		{ return ParserBase::TT_GOTO; }
if		{ return ParserBase::TT_IF; }
loop		{ return ParserBase::TT_LOOP; }
new		{ return ParserBase::TT_NEW; }
redo		{ return ParserBase::TT_REDO; }
return		{ return ParserBase::TT_RETURN; }
sizeof		{ return ParserBase::TT_SIZEOF; }
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
.		{ return matched()[0]; }
