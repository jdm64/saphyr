%scanner scanner.h
%filenames parser
%parsefun-source parser.cpp

%union {
	int t_int;
	std::string* t_str;
	NQualifier* t_qual;
	NParameter* t_param;
	NStatement* t_stm;
	NExpression* t_exp;
	NStatementList* t_stmlist;
	NExpressionList* t_explist;
	NParameterList* t_parlist;
	NVariableDeclList* t_varlist;
}

// predefined constants
%token <t_int> TT_FALSE TT_TRUE
// qualifiers
%token <t_int> TT_VOID TT_BOOL TT_INT TT_INT8 TT_INT16 TT_INT32 TT_INT64 TT_FLOAT TT_DOUBLE
// operators
%token <t_int> TT_LSHIFT TT_RSHIFT TT_LEQ TT_EQ TT_NEQ TT_GEQ
// keywords
%token <t_int> TT_RETURN
// constants and names
%token <t_str> TT_INTEGER TT_FLOATING TT_IDENTIFIER

// qualifiers
%type <t_qual> type_qualifier
// parameter
%type <t_param> parameter
// operators
%type <t_int> multiplication_operator addition_operator shift_operator greater_or_less_operator equals_operator
// statements
%type <t_stm> statement declaration function_declaration variable_declarations
// expressions
%type <t_exp> expression assignment equals_expression greater_or_less_expression bit_or_expression bit_xor_expression
%type <t_exp> bit_and_expression shift_expression addition_expression multiplication_expression unary_expression
%type <t_exp> primary_expression function_call
// lists
%type <t_stmlist> statement_list declaration_list compound_statement
%type <t_varlist> variable_list
%type <t_explist> expression_list
%type <t_parlist> parameter_list

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
	;
function_declaration
	: type_qualifier TT_IDENTIFIER '(' parameter_list ')' compound_statement
	{
		$$ = new NFunctionDeclaration($1, $2, $4, $6);
	}
	;
type_qualifier
	: TT_VOID
	{
		$$ = new NQualifier(QualifierType::VOID);
	}
	| TT_BOOL
	{
		$$ = new NQualifier(QualifierType::BOOL);
	}
	| TT_INT
	{
		$$ = new NQualifier(QualifierType::INT);
	}
	| TT_INT8
	{
		$$ = new NQualifier(QualifierType::INT8);
	}
	| TT_INT16
	{
		$$ = new NQualifier(QualifierType::INT16);
	}
	| TT_INT32
	{
		$$ = new NQualifier(QualifierType::INT32);
	}
	| TT_INT64
	{
		$$ = new NQualifier(QualifierType::INT64);
	}
	| TT_FLOAT
	{
		$$ = new NQualifier(QualifierType::FLOAT);
	}
	| TT_DOUBLE
	{
		$$ = new NQualifier(QualifierType::DOUBLE);
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
	: type_qualifier TT_IDENTIFIER
	{
		$$ = new NParameter($1, $2);
	}
	;
compound_statement
	: '{' '}'
	{
		$$ = new NStatementList;
	}
	| '{' statement_list '}'
	{
		$$ = $2;
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
	| expression ';'
	| TT_RETURN ';'
	{
		$$ = new NReturnStatement(nullptr);
	}
	| TT_RETURN expression ';'
	{
		$$ = new NReturnStatement($2);
	}
	;
variable_declarations
	: type_qualifier variable_list
	{
		$$ = new NVariableDeclGroup($1, $2);
	}
	;
variable_list
	: TT_IDENTIFIER
	{
		$$ = new NVariableDeclList;
		$$->addItem(new NVariableDecl($1));
	}
	| variable_list ',' TT_IDENTIFIER
	{
		$1->addItem(new NVariableDecl($3));
	}
	;
expression
	: assignment
	| equals_expression
	;
assignment
	: TT_IDENTIFIER '=' equals_expression
	{
		$$ = new NAssignment(new NVariable($1), $3);
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
	: unary_expression
	| multiplication_expression multiplication_operator unary_expression
	{
		$$ = new NBinaryMathOperator($2, $1, $3);
	}
	;
multiplication_operator
	: '*' { $$ = '*'; }
	| '/' { $$ = '/'; }
	| '%' { $$ = '%'; }
	;
unary_expression
	: primary_expression
	| '+' primary_expression
	{
		$$ = new NBinaryMathOperator('+', new NIntConst(new string("0")), $2);
	}
	| '-' primary_expression
	{
		$$ = new NBinaryMathOperator('-', new NIntConst(new string("0")), $2);
	}
	;
primary_expression
	: function_call
	| '(' equals_expression ')'
	{
		$$ = $2;
	}
	| TT_IDENTIFIER
	{
		$$ = new NVariable($1);
	}
	| TT_INTEGER
	{
		$$ = new NIntConst($1);
	}
	| TT_FLOATING
	{
		$$ = new NFloatConst($1);
	}
	| TT_TRUE
	{
		$$ = new NIntConst(new string("1"), QualifierType::BOOL);
	}
	| TT_FALSE
	{
		$$ = new NIntConst(new string("0"), QualifierType::BOOL);
	}
	;
function_call
	: TT_IDENTIFIER '(' expression_list ')'
	{
		$$ = new NFunctionCall($1, $3);
	}
	| TT_IDENTIFIER '(' ')'
	{
		$$ = new NFunctionCall($1, new NExpressionList);
	}
	;
expression_list
	: expression
	{
		$$ = new NExpressionList;
		$$->addItem($1);
	}
	| expression_list ',' expression
	{
		$1->addItem($3);
	}
	;
