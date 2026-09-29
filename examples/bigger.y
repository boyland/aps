/* Parser grammar for the Bigger example language. */

%token INTEGER STRING BOOLEAN ARRAY
%token IF ELSE WHILE SUBSTRING
%token EQUAL CONCAT LOGOR

%token <String> ID
%token <Integer> INT_LITERAL
%token <String> STR_LITERAL
%token <Boolean> BOOL_LITERAL

%type <Program> program
%type <Block> block
%type <Decls> decls
%type <Decl> decl
%type <Type> type
%type <Stmts> stmts
%type <Stmt> stmt
%type <Expr> expr

%left LOGOR
%left EQUAL
%left '<'
%left '+'
%left '['

%%

program : block
	{ $$ = program($1); }
	;

block : '{' decls stmts '}'
	{ $$ = block($2, $3); }
	;

decls : /* NOTHING */
	{ $$ = no_decls(); }
      | decls decl
	{ $$ = xcons_decls($1, $2); }
      ;

decl : type ID ';'
	{ $$ = decl($2, $1); }
     ;

type : INTEGER
	{ $$ = integer(); }
     | STRING
	{ $$ = string(); }
     | BOOLEAN
	{ $$ = boolean(); }
     | ARRAY '[' type ']'
	{ $$ = array($3); }
     ;

stmts : /* NOTHING */
	{ $$ = no_stmts(); }
      | stmts stmt
	{ $$ = xcons_stmts($1, $2); }
      ;

stmt : block
	{ $$ = block_stmt($1); }
     | expr '=' expr ';'
	{ $$ = assign_stmt($1, $3); }
     | IF '(' expr ')' stmt ELSE stmt
	{ $$ = if_stmt($3, $5, $7); }
     | WHILE '(' expr ')' stmt
	{ $$ = while_stmt($3, $5); }
     ;

expr : ID
	{ $$ = variable($1); }
     | INT_LITERAL
	{ $$ = intconstant($1); }
     | STR_LITERAL
	{ $$ = strconstant($1); }
     | BOOL_LITERAL
	{ $$ = boolconstant($1); }
     | '(' expr ')'
	{ $$ = $2; }
     | expr '<' expr
	{ $$ = less($1, $3); }
     | expr EQUAL expr
	{ $$ = equalf($1, $3); }
     | expr '+' expr
	{ $$ = plus($1, $3); }
     | expr LOGOR expr
	{ $$ = logor($1, $3); }
     | CONCAT '(' expr ',' expr ')'
	{ $$ = concat($3, $5); }
     | SUBSTRING '(' expr ',' expr ',' expr ')'
	{ $$ = substring($3, $5, $7); }
     | expr '[' expr ']'
	{ $$ = array_ref($1, $3); }
     ;

%%
