%baseclass-preinclude AST.h
%scanner scanner.h
%filenames parser
%parsefun-source parser.cpp

%union {
	int t_int;
	Token* t_tok;
	struct {
		Token* tok;
		int op;
	} t_tok_int;
	NIntConst* t_const_int;
	NDataType* t_dtype;
	NVariable* t_var;
	NParameter* t_param;
	NVariableDecl* t_var_decl;
	NStatement* t_stm;
	NExpression* t_exp;
	NSwitchCase* t_case;
	NFunctionPrototype* t_func_pro;
	NDataTypeList* t_typelist;
	NStatementList* t_stmlist;
	NExpressionList* t_explist;
	NParameterList* t_parlist;
	NVariableDeclList* t_varlist;
	NSwitchCaseList* t_caslist;
	NVariableDeclGroupList* t_var_dec_list;
}

// predefined constants
%token <t_tok> TT_FALSE TT_TRUE TT_NULL
// base types
%token <t_tok> TT_AUTO TT_VOID TT_BOOL TT_INT TT_INT8 TT_INT16 TT_INT32 TT_INT64 TT_FLOAT TT_DOUBLE
%token <t_tok> TT_UINT TT_UINT8 TT_UINT16 TT_UINT32 TT_UINT64
// operators
%token <t_tok> TT_LSHIFT TT_RSHIFT TT_LEQ TT_EQ TT_NEQ TT_GEQ TT_LOG_AND TT_LOG_OR
%token <t_tok> TT_ASG_MUL TT_ASG_DIV TT_ASG_MOD TT_ASG_ADD TT_ASG_SUB TT_ASG_LSH
%token <t_tok> TT_ASG_RSH TT_ASG_AND TT_ASG_OR TT_ASG_XOR TT_INC TT_DEC TT_DQ_MARK
%token <t_tok> TT_ASG_DQ
// keywords
%token TT_RETURN TT_WHILE TT_DO TT_UNTIL TT_CONTINUE TT_REDO TT_BREAK TT_FOR TT_IF TT_GOTO TT_SWITCH TT_CASE
%token TT_DEFAULT TT_SIZEOF TT_STRUCT TT_UNION TT_ENUM TT_DELETE TT_NEW TT_LOOP TT_ALIAS TT_VEC
%left TT_ELSE
// constants and names
%token <t_tok> TT_INTEGER TT_FLOATING TT_IDENTIFIER TT_INT_BIN TT_INT_OCT TT_INT_HEX TT_CHAR_LIT TT_STR_LIT

// integer constant
%type <t_const_int> integer_constant
// data types
%type <t_dtype> data_type base_type explicit_data_type
// parameter
%type <t_param> parameter
// variable
%type <t_var> variable_expresion explicit_variable_expresion function_call
// variable declaration
%type <t_var_decl> variable global_variable
// operators
%type <t_tok_int> multiplication_operator addition_operator shift_operator greater_or_less_operator equals_operator
%type <t_tok_int> assignment_operator unary_operator increment_decrement_operator
// keywords
%type <t_tok_int> branch_keyword
// statements
%type <t_stm> statement declaration function_declaration while_loop branch_statement
%type <t_stm> variable_declarations condition_statement global_variable_declaration
%type <t_stm> struct_declaration union_declaration enum_declaration alias_declaration
%type <t_case> switch_case
// expressions
%type <t_exp> expression assignment equals_expression greater_or_less_expression bit_or_expression bit_xor_expression
%type <t_exp> bit_and_expression shift_expression addition_expression multiplication_expression unary_expression
%type <t_exp> primary_expression logical_or_expression logical_and_expression expression_or_empty
%type <t_exp> value_expression ternary_expression increment_decrement_expression null_coalescing_expression
%type <t_exp> sizeof_expression paren_expression new_expression
// function prototype
%type <t_func_pro> function_prototype
// lists
%type <t_stmlist> statement_list declaration_list compound_statement statement_list_or_empty single_statement
%type <t_stmlist> declaration_or_expression_list else_statement function_body
%type <t_varlist> variable_list global_variable_list
%type <t_explist> expression_list
%type <t_parlist> parameter_list
%type <t_caslist> switch_case_list
%type <t_typelist> data_type_list
%type <t_var_dec_list> variable_declarations_list

%%

file
	: declaration_list
	{
		root = unique_ptr<NStatementList>($1);
	}
	;
declaration_list
	: declaration
	{
		$$ = new NStatementList;
		$$->addItem($1);
	}
	| declaration_list declaration
	{
		$1->addItem($2);
	}
	;
declaration
	: function_declaration
	| global_variable_declaration
	| alias_declaration
	| struct_declaration
	| union_declaration
	| enum_declaration
	;
global_variable_declaration
	: data_type global_variable_list ';'
	{
		$$ = new NVariableDeclGroup($1, $2);
	}
	;
alias_declaration
	: TT_ALIAS TT_IDENTIFIER '=' data_type ';'
	{
		$$ = new NAliasDeclaration($2, $4);
	}
	;
struct_declaration
	: TT_STRUCT TT_IDENTIFIER '{' variable_declarations_list '}'
	{
		$$ = new NStructDeclaration($2, $4);
	}
	;
union_declaration
	: TT_UNION TT_IDENTIFIER '{' variable_declarations_list '}'
	{
		$$ = new NUnionDeclaration($2, $4);
	}
	;
enum_declaration
	: TT_ENUM TT_IDENTIFIER '{' variable_list '}'
	{
		$$ = new NEnumDeclaration($2, $4);
	}
	;
function_declaration
	: function_prototype function_body
	{
		$$ = new NFunctionDeclaration($1, $2);
	}
	;
function_prototype
	: data_type TT_IDENTIFIER '(' parameter_list ')'
	{
		$$ = new NFunctionPrototype($2, $1, $4);
	}
	;
function_body
	: compound_statement
	| ';'
	{
		$$ = nullptr;
	}
	;
compound_statement
	: '{' statement_list_or_empty '}'
	{
		$$ = $2;
	}
	;
statement_list_or_empty
	:
	{
		$$ = new NStatementList;
	}
	| statement_list
	;
single_statement
	: compound_statement
	| statement
	{
		$$ = new NStatementList;
		$$->addItem($1);
	}
	| ';'
	{
		$$ = new NStatementList;
	}
	;
statement_list
	: statement
	{
		$$ = new NStatementList;
		$$->addItem($1);
	}
	| statement_list statement
	{
		$1->addItem($2);
	}
	;
statement
	: variable_declarations ';'
	| while_loop
	| branch_statement
	| condition_statement
	| expression ';'
	{
		$$ = $1;
	}
	| TT_SWITCH '(' expression ')' '{' switch_case_list '}'
	{
		$$ = new NSwitchStatement($2.t_tok, $3, $6);
	}
	| TT_IDENTIFIER ':'
	{
		$$ = new NLabelStatement($1);
	}
	| TT_GOTO TT_IDENTIFIER ';'
	{
		$$ = new NGotoStatement($2);
	}
	| TT_RETURN expression_or_empty ';'
	{
		$$ = new NReturnStatement($1.t_tok, $2);
	}
	| TT_DELETE variable_expresion ';'
	{
		$$ = new NDeleteStatement($1.t_tok, $2);
	}
	| TT_FOR '(' declaration_or_expression_list ';' expression_or_empty ';' expression_list ')' single_statement
	{
		$$ = new NForStatement($3, $5, $6.t_tok, $7, $9);
	}
	| TT_LOOP single_statement
	{
		$$ = new NLoopStatement($2);
	}
	;
while_loop
	: TT_WHILE '(' expression_or_empty ')' single_statement
	{
		$$ = new NWhileStatement($2.t_tok, $3, $5);
	}
	| TT_DO single_statement TT_WHILE '(' expression_or_empty ')' ';'
	{
		$$ = new NWhileStatement($4.t_tok, $5, $2, true);
	}
	| TT_UNTIL '(' expression_or_empty ')' single_statement
	{
		$$ = new NWhileStatement($2.t_tok, $3, $5, false, true);
	}
	| TT_DO single_statement TT_UNTIL '(' expression_or_empty ')' ';'
	{
		$$ = new NWhileStatement($4.t_tok, $5, $2, true, true);
	}
	;
switch_case_list
	: switch_case
	{
		$$ = new NSwitchCaseList;
		$$->addItem($1);
	}
	| switch_case_list switch_case
	{
		$1->addItem($2);
	}
	;
switch_case
	: TT_CASE integer_constant ':' statement_list_or_empty
	{
		$$ = new NSwitchCase($1.t_tok, $4, $2);
	}
	| TT_DEFAULT ':' statement_list_or_empty
	{
		$$ = new NSwitchCase($1.t_tok, $3);
	}
	;
branch_statement
	: branch_keyword ';'
	{
		$$ = new NLoopBranch(($1).tok, ($1).op);
	}
	| branch_keyword integer_constant ';'
	{
		$$ = new NLoopBranch(($1).tok, ($1).op, $2);
	}
	;
branch_keyword
	: TT_CONTINUE { $$ = {$1.t_tok, TT_CONTINUE}; }
	| TT_BREAK    { $$ = {$1.t_tok, TT_BREAK}; }
	| TT_REDO     { $$ = {$1.t_tok, TT_REDO}; }
	;
condition_statement
	: TT_IF '(' expression ')' single_statement else_statement
	{
		$$ = new NIfStatement($2.t_tok, $3, $5, $6);
	}
	;
else_statement
	:
	{
		$$ = nullptr;
	}
	| TT_ELSE single_statement
	{
		$$ = $2;
	}
	;
variable_declarations_list
	: variable_declarations ';'
	{
		$$ = new NVariableDeclGroupList;
		$$->addItem(static_cast<NVariableDeclGroup*>($1));
	}
	| variable_declarations_list variable_declarations ';'
	{
		$1->addItem(static_cast<NVariableDeclGroup*>($2));
	}
	;
variable_declarations
	: explicit_data_type variable_list
	{
		$$ = new NVariableDeclGroup($1, $2);
	}
	| TT_IDENTIFIER variable_list
	{
		auto type = new NUserType($1);
		$$ = new NVariableDeclGroup(type, $2);
	}
	;
variable_list
	: variable
	{
		$$ = new NVariableDeclList;
		$$->addItem($1);
	}
	| variable_list ',' variable
	{
		$1->addItem($3);
	}
	;
global_variable_list
	: global_variable
	{
		$$ = new NVariableDeclList;
		$$->addItem($1);
	}
	| global_variable_list ',' global_variable
	{
		$1->addItem($3);
	}
	;
variable
	: TT_IDENTIFIER
	{
		$$ = new NVariableDecl($1);
	}
	| TT_IDENTIFIER '=' expression
	{
		$$ = new NVariableDecl($1, $2.t_tok, $3);
	}
	;
global_variable
	: TT_IDENTIFIER
	{
		$$ = new NGlobalVariableDecl($1);
	}
	| TT_IDENTIFIER '=' expression
	{
		$$ = new NGlobalVariableDecl($1, $2.t_tok, $3);
	}
	;
parameter_list
	:
	{
		$$ = new NParameterList;
	}
	| parameter
	{
		$$ = new NParameterList;
		$$->addItem($1);
	}
	| parameter_list ',' parameter
	{
		$1->addItem($3);
	}
	;
parameter
	: data_type TT_IDENTIFIER
	{
		$$ = new NParameter($1, $2);
	}
	;
data_type
	: explicit_data_type
	| TT_IDENTIFIER
	{
		$$ = new NUserType($1);
	}
	;
explicit_data_type
	: base_type
	| '[' integer_constant ']' data_type
	{
		$$ = new NArrayType($4, $2);
	}
	| '[' ']' data_type
	{
		$$ = new NArrayType($3);
	}
	| TT_VEC '<' integer_constant ',' data_type '>'
	{
		$$ = new NVecType($1.t_tok, $3, $5);
	}
	| '@' data_type
	{
		$$ = new NPointerType($2);
	}
	| '@' '(' data_type_list ')' data_type
	{
		$$ = new NFuncPointerType($1.t_tok, $5, $3);
	}
	;
data_type_list
	:
	{
		$$ = new NDataTypeList;
	}
	| data_type
	{
		$$ = new NDataTypeList;
		$$->addItem($1);
	}
	| data_type_list ',' data_type
	{
		$1->addItem($3);
	}
	;
base_type
	: TT_AUTO   { $$ = new NBaseType($1, TT_AUTO);   }
	| TT_VOID   { $$ = new NBaseType($1, TT_VOID);   }
	| TT_BOOL   { $$ = new NBaseType($1, TT_BOOL);   }
	| TT_INT    { $$ = new NBaseType($1, TT_INT);    }
	| TT_INT8   { $$ = new NBaseType($1, TT_INT8);   }
	| TT_INT16  { $$ = new NBaseType($1, TT_INT16);  }
	| TT_INT32  { $$ = new NBaseType($1, TT_INT32);  }
	| TT_INT64  { $$ = new NBaseType($1, TT_INT64);  }
	| TT_UINT   { $$ = new NBaseType($1, TT_UINT);   }
	| TT_UINT8  { $$ = new NBaseType($1, TT_UINT8);  }
	| TT_UINT16 { $$ = new NBaseType($1, TT_UINT16); }
	| TT_UINT32 { $$ = new NBaseType($1, TT_UINT32); }
	| TT_UINT64 { $$ = new NBaseType($1, TT_UINT64); }
	| TT_FLOAT  { $$ = new NBaseType($1, TT_FLOAT);  }
	| TT_DOUBLE { $$ = new NBaseType($1, TT_DOUBLE); }
	;
expression_list
	:
	{
		$$ = new NExpressionList;
	}
	| expression
	{
		$$ = new NExpressionList;
		$$->addItem($1);
	}
	| expression_list ',' expression
	{
		$1->addItem($3);
	}
	;
expression_or_empty
	:
	{
		$$ = nullptr;
	}
	| expression
	;
declaration_or_expression_list
	: expression_list
	{
		$$ = $1->move<NStatementList>();
	}
	| variable_declarations
	{
		$$ = new NStatementList;
		$$->addItem($1);
	}
	;
expression
	: assignment
	;
assignment
	: ternary_expression
	| variable_expresion assignment_operator expression
	{
		$$ = new NAssignment(($2).op, ($2).tok, $1, $3);
	}
	;
ternary_expression
	: new_expression
	| new_expression '?' new_expression ':' new_expression
	{
		$$ = new NTernaryOperator($1, $3, $4.t_tok, $5);
	}
	;
assignment_operator
	: '=' { $$ = {$1.t_tok, '='}; }
	| TT_ASG_MUL { $$ = {$1, '*'}; }
	| TT_ASG_DIV { $$ = {$1, '/'}; }
	| TT_ASG_MOD { $$ = {$1, '%'}; }
	| TT_ASG_ADD { $$ = {$1, '+'}; }
	| TT_ASG_SUB { $$ = {$1, '-'}; }
	| TT_ASG_LSH { $$ = {$1, TT_LSHIFT}; }
	| TT_ASG_RSH { $$ = {$1, TT_RSHIFT}; }
	| TT_ASG_AND { $$ = {$1, '&'}; }
	| TT_ASG_OR  { $$ = {$1, '^'}; }
	| TT_ASG_XOR { $$ = {$1, '|'}; }
	| TT_ASG_DQ  { $$ = {$1, TT_DQ_MARK}; }
	;
new_expression
	: logical_or_expression
	| TT_NEW data_type
	{
		$$ = new NNewExpression($1.t_tok, $2);
	}
	;
logical_or_expression
	: logical_and_expression
	| logical_or_expression TT_LOG_OR logical_and_expression
	{
		$$ = new NLogicalOperator(TT_LOG_OR, $2, $1, $3);
	}
	;
logical_and_expression
	: equals_expression
	| logical_and_expression TT_LOG_AND equals_expression
	{
		$$ = new NLogicalOperator(TT_LOG_AND, $2, $1, $3);
	}
	;
equals_expression
	: greater_or_less_expression
	| equals_expression equals_operator greater_or_less_expression
	{
		$$ = new NCompareOperator(($2).op, ($2).tok, $1, $3);
	}
	;
equals_operator
	: TT_EQ { $$ = {$1, TT_EQ}; }
	| TT_NEQ { $$ = {$1, TT_NEQ}; }
	;
greater_or_less_expression
	: bit_or_expression
	| greater_or_less_expression greater_or_less_operator bit_or_expression
	{
		$$ = new NCompareOperator(($2).op, ($2).tok, $1, $3);
	}
	;
greater_or_less_operator
	: '<' { $$ = {$1.t_tok, '<'}; }
	| '>' { $$ = {$1.t_tok, '>'}; }
	| TT_LEQ { $$ = {$1, TT_LEQ}; }
	| TT_GEQ { $$ = {$1, TT_GEQ}; }
	;
bit_or_expression
	: bit_xor_expression
	| bit_or_expression '|' bit_xor_expression
	{
		$$ = new NBinaryMathOperator('|', $2.t_tok, $1, $3);
	}
	;
bit_xor_expression
	: bit_and_expression
	| bit_xor_expression '^' bit_and_expression
	{
		$$ = new NBinaryMathOperator('^', $2.t_tok, $1, $3);
	}
	;
bit_and_expression
	: shift_expression
	| bit_and_expression '&' shift_expression
	{
		$$ = new NBinaryMathOperator('&', $2.t_tok, $1, $3);
	}
	;
shift_expression
	: addition_expression
	| shift_expression shift_operator addition_expression
	{
		$$ = new NBinaryMathOperator(($2).op, ($2).tok, $1, $3);
	}
	;
shift_operator
	: TT_LSHIFT { $$ = {$1, TT_LSHIFT}; }
	| TT_RSHIFT { $$ = {$1, TT_RSHIFT}; }
	;
addition_expression
	: multiplication_expression
	| addition_expression addition_operator multiplication_expression
	{
		$$ = new NBinaryMathOperator(($2).op, ($2).tok, $1, $3);
	}
	;
addition_operator
	: '+' { $$ = {$1.t_tok, '+'}; }
	| '-' { $$ = {$1.t_tok, '-'}; }
	;
multiplication_expression
	: null_coalescing_expression
	| multiplication_expression multiplication_operator null_coalescing_expression
	{
		$$ = new NBinaryMathOperator(($2).op, ($2).tok, $1, $3);
	}
	;
multiplication_operator
	: '*' { $$ = {$1.t_tok, '*'}; }
	| '/' { $$ = {$1.t_tok, '/'}; }
	| '%' { $$ = {$1.t_tok, '%'}; }
	;
null_coalescing_expression
	: unary_expression
	| unary_expression TT_DQ_MARK unary_expression
	{
		$$ = new NNullCoalescing($2, $1, $3);
	}
	;
unary_expression
	: primary_expression
	| increment_decrement_expression
	| sizeof_expression
	| unary_operator primary_expression
	{
		$$ = new NUnaryMathOperator(($1).op, ($1).tok, $2);
	}
	;
unary_operator
	: '+' { $$ = {$1.t_tok, '+'}; }
	| '-' { $$ = {$1.t_tok, '-'}; }
	| '!' { $$ = {$1.t_tok, '!'}; }
	| '~' { $$ = {$1.t_tok, '~'}; }
	;
sizeof_expression
	: TT_SIZEOF explicit_variable_expresion
	{
		$$ = new NSizeOfOperator($1.t_tok, $2);
	}
	| TT_SIZEOF explicit_data_type
	{
		$$ = new NSizeOfOperator($1.t_tok, $2);
	}
	| TT_SIZEOF TT_IDENTIFIER
	{
		$$ = new NSizeOfOperator($1.t_tok, $2);
	}
	| TT_SIZEOF '(' explicit_variable_expresion ')'
	{
		$$ = new NSizeOfOperator($1.t_tok, $3);
	}
	| TT_SIZEOF '(' explicit_data_type ')'
	{
		$$ = new NSizeOfOperator($1.t_tok, $3);
	}
	| TT_SIZEOF '(' TT_IDENTIFIER ')'
	{
		$$ = new NSizeOfOperator($1.t_tok, $3);
	}
	;
primary_expression
	: value_expression
	| variable_expresion
	{
		$$ = $1;
	}
	| paren_expression
	{
		$$ = $1;
	}
	;
paren_expression
	: '(' expression ')'
	{
		$$ = $2;
	}
	| paren_expression '[' expression ']'
	{
		$$ = new NArrayVariable(new NExprVariable($1), $2.t_tok, $3);
	}
	| paren_expression '.' TT_IDENTIFIER
	{
		$$ = new NMemberVariable(new NExprVariable($1), $3, $2.t_tok);
	}
	;
function_call
	: TT_IDENTIFIER '(' expression_list ')'
	{
		$$ = new NFunctionCall($1, $3);
	}
	;
increment_decrement_expression
	: increment_decrement_operator variable_expresion
	{
		$$ = new NIncrement(($1).op, ($1).tok, $2, false);
	}
	| variable_expresion increment_decrement_operator
	{
		$$ = new NIncrement(($2).op, ($2).tok, $1, true);
	}
	;
increment_decrement_operator
	: TT_INC { $$ = {$1, TT_INC}; }
	| TT_DEC { $$ = {$1, TT_DEC}; }
	;
variable_expresion
	: TT_IDENTIFIER
	{
		$$ = new NBaseVariable($1);
	}
	| function_call
	| explicit_variable_expresion
	;
explicit_variable_expresion
	: variable_expresion '[' expression ']'
	{
		$$ = new NArrayVariable($1, $2.t_tok, $3);
	}
	| variable_expresion '.' TT_IDENTIFIER
	{
		$$ = new NMemberVariable($1, $3, $2.t_tok);
	}
	| variable_expresion '@'
	{
		$$ = new NDereference($1, $2.t_tok);
	}
	| variable_expresion '$'
	{
		$$ = new NAddressOf($1);
	}
	;
value_expression
	: integer_constant
	{
		$$ = $1;
	}
	| TT_CHAR_LIT
	{
		$$ = new NCharConst($1);
	}
	| TT_STR_LIT
	{
		$$ = new NStringLiteral($1);
	}
	| TT_FLOATING
	{
		$$ = new NFloatConst($1);
	}
	| TT_TRUE
	{
		$$ = new NBoolConst($1, true);
	}
	| TT_FALSE
	{
		$$ = new NBoolConst($1, false);
	}
	| TT_NULL
	{
		$$ = new NNullPointer($1);
	}
	;
integer_constant
	: TT_INTEGER
	{
		$$ = new NIntConst($1);
	}
	| TT_INT_BIN
	{
		$$ = new NIntConst($1, 2);
	}
	| TT_INT_OCT
	{
		$$ = new NIntConst($1, 8);
	}
	| TT_INT_HEX
	{
		$$ = new NIntConst($1, 16);
	}
	;
