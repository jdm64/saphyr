%scanner scanner.h
%filenames parser
%parsefun-source parser.cpp

%union {
	int t_int;
	std::string* t_str;
	NDataType* t_dtype;
	NVariable* t_var;
	NParameter* t_param;
	NVariableDecl* t_var_decl;
	NStatement* t_stm;
	NExpression* t_exp;
	NSwitchCase* t_case;
	NFunctionPrototype* t_func_pro;
	NStatementList* t_stmlist;
	NExpressionList* t_explist;
	NParameterList* t_parlist;
	NVariableDeclList* t_varlist;
	NSwitchCaseList* t_caslist;
}

// predefined constants
%token <t_int> TT_FALSE TT_TRUE
// qualifiers
%token <t_int> TT_AUTO TT_VOID TT_BOOL TT_INT TT_INT8 TT_INT16 TT_INT32 TT_INT64 TT_FLOAT TT_DOUBLE
%token <t_int> TT_UINT TT_UINT8 TT_UINT16 TT_UINT32 TT_UINT64
// operators
%token <t_int> TT_LSHIFT TT_RSHIFT TT_LEQ TT_EQ TT_NEQ TT_GEQ TT_LOG_AND TT_LOG_OR
%token <t_int> TT_ASG_MUL TT_ASG_DIV TT_ASG_MOD TT_ASG_ADD TT_ASG_SUB TT_ASG_LSH
%token <t_int> TT_ASG_RSH TT_ASG_AND TT_ASG_OR TT_ASG_XOR TT_INC TT_DEC TT_DQ_MARK
// keywords
%token TT_RETURN TT_WHILE TT_DO TT_UNTIL TT_CONTINUE TT_REDO TT_BREAK TT_FOR TT_IF TT_GOTO TT_SWITCH TT_CASE
%token TT_DEFAULT
%left TT_ELSE
// constants and names
%token <t_str> TT_INTEGER TT_FLOATING TT_IDENTIFIER TT_INT_BIN TT_INT_OCT TT_INT_HEX

// data types
%type <t_dtype> data_type base_type
// parameter
%type <t_param> parameter
// variable
%type <t_var> variable_expresion
// variable declaration
%type <t_var_decl> variable global_variable
// operators
%type <t_int> multiplication_operator addition_operator shift_operator greater_or_less_operator equals_operator
%type <t_int> assignment_operator unary_operator increment_decrement_operator
// keywords
%type <t_int> branch_keyword base_type_keyword
// statements
%type <t_stm> statement declaration function_declaration while_loop branch_statement
%type <t_stm> variable_declarations condition_statement global_variable_declaration
%type <t_case> switch_case
// expressions
%type <t_exp> expression assignment equals_expression greater_or_less_expression bit_or_expression bit_xor_expression
%type <t_exp> bit_and_expression shift_expression addition_expression multiplication_expression unary_expression
%type <t_exp> primary_expression function_call logical_or_expression logical_and_expression expression_or_empty
%type <t_exp> value_expression ternary_expression increment_decrement_expression null_coalescing_expression
// function prototype
%type <t_func_pro> function_prototype
// lists
%type <t_stmlist> statement_list declaration_list compound_statement statement_list_or_empty single_statement
%type <t_stmlist> declaration_or_expression_list else_statement function_body
%type <t_varlist> variable_list global_variable_list
%type <t_explist> expression_list
%type <t_parlist> parameter_list
%type <t_caslist> switch_case_list

%%

file
	: declaration_list
	{
		programBlock = $1;
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
	;
global_variable_declaration
	: data_type global_variable_list ';'
	{
		$$ = new NVariableDeclGroup($1, $2);
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
		$$ = new NSwitchStatement($3, $6);
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
		$$ = new NReturnStatement($2);
	}
	| TT_FOR '(' declaration_or_expression_list ';' expression_or_empty ';' expression_list ')' single_statement
	{
		$$ = new NForStatement($3, $5, $7, $9);
	}
	;
while_loop
	: TT_WHILE '(' expression_or_empty ')' single_statement
	{
		$$ = new NWhileStatement($3, $5);
	}
	| TT_DO single_statement TT_WHILE '(' expression_or_empty ')' ';'
	{
		$$ = new NWhileStatement($5, $2, true);
	}
	| TT_UNTIL '(' expression_or_empty ')' single_statement
	{
		$$ = new NWhileStatement($3, $5, false, true);
	}
	| TT_DO single_statement TT_UNTIL '(' expression_or_empty ')' ';'
	{
		$$ = new NWhileStatement($5, $2, true, true);
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
	: TT_CASE TT_INTEGER ':' statement_list_or_empty
	{
		$$ = new NSwitchCase(new NIntConst($2), $4);
	}
	| TT_DEFAULT ':' statement_list_or_empty
	{
		$$ = new NSwitchCase($3);
	}
	;
branch_statement
	: branch_keyword ';'
	{
		$$ = new NLoopBranch($1);
	}
	;
branch_keyword
	: TT_CONTINUE { $$ = TT_CONTINUE; }
	| TT_BREAK { $$ = TT_BREAK; }
	| TT_REDO { $$ = TT_REDO; }
	;
condition_statement
	: TT_IF '(' expression ')' single_statement else_statement
	{
		$$ = new NIfStatement($3, $5, $6);
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
variable_declarations
	: data_type variable_list
	{
		$$ = new NVariableDeclGroup($1, $2);
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
		$$ = new NVariableDecl($1, $3);
	}
	;
global_variable
	: TT_IDENTIFIER
	{
		$$ = new NGlobalVariableDecl($1);
	}
	| TT_IDENTIFIER '=' expression
	{
		$$ = new NGlobalVariableDecl($1, $3);
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
	: base_type
	| '[' TT_INTEGER ']' data_type
	{
		$$ = new NArrayType($2, $4);
	}
	;
base_type
	: base_type_keyword
	{
		$$ = new NBaseType($1);
	}
	;
base_type_keyword
	: TT_AUTO { $$ = TT_AUTO; }
	| TT_VOID { $$ = TT_VOID; }
	| TT_BOOL { $$ = TT_BOOL; }
	| TT_INT { $$ = TT_INT; }
	| TT_INT8 { $$ = TT_INT8; }
	| TT_INT16 { $$ = TT_INT16; }
	| TT_INT32 { $$ = TT_INT32; }
	| TT_INT64 { $$ = TT_INT64; }
	| TT_UINT { $$ = TT_UINT; }
	| TT_UINT8 { $$ = TT_UINT8; }
	| TT_UINT16 { $$ = TT_UINT16; }
	| TT_UINT32 { $$ = TT_UINT32; }
	| TT_UINT64 { $$ = TT_UINT64; }
	| TT_FLOAT { $$ = TT_FLOAT; }
	| TT_DOUBLE { $$ = TT_DOUBLE; }
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
		$$ = $1->copy<NStatementList>();
		delete $1;
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
		$$ = new NAssignment($2, $1, $3);
	}
	;
ternary_expression
	: logical_or_expression
	| logical_or_expression '?' logical_or_expression ':' logical_or_expression
	{
		$$ = new NTernaryOperator($1, $3, $5);
	}
	;
assignment_operator
	: '=' { $$ = '='; }
	| TT_ASG_MUL { $$ = '*'; }
	| TT_ASG_DIV { $$ = '/'; }
	| TT_ASG_MOD { $$ = '%'; }
	| TT_ASG_ADD { $$ = '+'; }
	| TT_ASG_SUB { $$ = '-'; }
	| TT_ASG_LSH { $$ = TT_LSHIFT; }
	| TT_ASG_RSH { $$ = TT_RSHIFT; }
	| TT_ASG_AND { $$ = '&'; }
	| TT_ASG_OR  { $$ = '^'; }
	| TT_ASG_XOR { $$ = '|'; }
	;
logical_or_expression
	: logical_and_expression
	| logical_or_expression TT_LOG_OR logical_and_expression
	{
		$$ = new NLogicalOperator(TT_LOG_OR, $1, $3);
	}
	;
logical_and_expression
	: equals_expression
	| logical_and_expression TT_LOG_AND equals_expression
	{
		$$ = new NLogicalOperator(TT_LOG_AND, $1, $3);
	}
	;
equals_expression
	: greater_or_less_expression
	| equals_expression equals_operator greater_or_less_expression
	{
		$$ = new NCompareOperator($2, $1, $3);
	}
	;
equals_operator
	: TT_EQ { $$ = TT_EQ; }
	| TT_NEQ { $$ = TT_NEQ; }
	;
greater_or_less_expression
	: bit_or_expression
	| greater_or_less_expression greater_or_less_operator bit_or_expression
	{
		$$ = new NCompareOperator($2, $1, $3);
	}
	;
greater_or_less_operator
	: '<' { $$ = '<'; }
	| '>' { $$ = '>'; }
	| TT_LEQ { $$ = TT_LEQ; }
	| TT_GEQ { $$ = TT_GEQ; }
	;
bit_or_expression
	: bit_xor_expression
	| bit_or_expression '|' bit_xor_expression
	{
		$$ = new NBinaryMathOperator('|', $1, $3);
	}
	;
bit_xor_expression
	: bit_and_expression
	| bit_xor_expression '^' bit_and_expression
	{
		$$ = new NBinaryMathOperator('^', $1, $3);
	}
	;
bit_and_expression
	: shift_expression
	| bit_and_expression '&' shift_expression
	{
		$$ = new NBinaryMathOperator('&', $1, $3);
	}
	;
shift_expression
	: addition_expression
	| shift_expression shift_operator addition_expression
	{
		$$ = new NBinaryMathOperator($2, $1, $3);
	}
	;
shift_operator
	: TT_LSHIFT { $$ = TT_LSHIFT; }
	| TT_RSHIFT { $$ = TT_RSHIFT; }
	;
addition_expression
	: multiplication_expression
	| addition_expression addition_operator multiplication_expression
	{
		$$ = new NBinaryMathOperator($2, $1, $3);
	}
	;
addition_operator
	: '+' { $$ = '+'; }
	| '-' { $$ = '-'; }
	;
multiplication_expression
	: null_coalescing_expression
	| multiplication_expression multiplication_operator null_coalescing_expression
	{
		$$ = new NBinaryMathOperator($2, $1, $3);
	}
	;
multiplication_operator
	: '*' { $$ = '*'; }
	| '/' { $$ = '/'; }
	| '%' { $$ = '%'; }
	;
null_coalescing_expression
	: unary_expression
	| unary_expression TT_DQ_MARK unary_expression
	{
		$$ = new NNullCoalescing($1, $3);
	}
	;
unary_expression
	: primary_expression
	| unary_operator primary_expression
	{
		$$ = new NUnaryMathOperator($1, $2);
	}
	;
unary_operator
	: '+' { $$ = '+'; }
	| '-' { $$ = '-'; }
	| '!' { $$ = '!'; }
	| '~' { $$ = '~'; }
	;
primary_expression
	: function_call
	| value_expression
	| increment_decrement_expression
	| '(' expression ')'
	{
		$$ = $2;
	}
	;
function_call
	: TT_IDENTIFIER '(' expression_list ')'
	{
		$$ = new NFunctionCall($1, $3);
	}
	;
increment_decrement_expression
	: variable_expresion
	{
		$$ = $1;
	}
	| increment_decrement_operator variable_expresion
	{
		$$ = new NIncrement($2, $1, false);
	}
	| variable_expresion increment_decrement_operator
	{
		$$ = new NIncrement($1, $2, true);
	}
	;
increment_decrement_operator
	: TT_INC { $$ = TT_INC; }
	| TT_DEC { $$ = TT_DEC; }
	;
variable_expresion
	: TT_IDENTIFIER
	{
		$$ = new NVariable($1);
	}
	| variable_expresion '[' expression ']'
	{
		$$ = new NArrayVariable($1, $3);
	}
	;
value_expression
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
	| TT_FLOATING
	{
		$$ = new NFloatConst($1);
	}
	| TT_TRUE
	{
		$$ = new NBoolConst(true);
	}
	| TT_FALSE
	{
		$$ = new NBoolConst(false);
	}
	;
