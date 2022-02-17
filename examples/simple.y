/* An incomplete parser for simple.y */

/*
 * This fragment is intended to be used either by bison or ScalaBison,
 * but will need language specific parts to compile as C/C++ or Scala
 */

%token INT
%token STRING

%token <String> ID
%token <Integer> INT_LITERAL
%token <String> STR_LITERAL

%type <Program> program
%type <Block> block
%type <Decls> decls
%type <Decl> decl
%type <Type> type
%type <Stmts> stmts
%type <Stmt> stmt
%type <Expr> expr

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

type : INT
	{ $$ = integer_type; }
     | STRING
	{ $$ = string_type; }
     ;

stmts : /* NOTHING */
	{ $$ = no_stmts(); }
      | stmts stmt
	{ $$ = xcons_stmts($1,$2); }
      ;

stmt: block
	{ $$ = block_stmt($1); }
    | expr '=' expr ';'
	{ $$ = assign_stmt($1,$3); }
    ;

expr : ID
	{ $$ = variable($1); }
     | INT_LITERAL
	{ $$ = intconstant($1); }
     | STR_LITERAL
	{ $$ = strconstant($1); }
     ;

%%

