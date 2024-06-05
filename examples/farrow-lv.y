/* Farrow's live-variable analysis parser */

/*
 * This fragment is intended to be used either by bison or ScalaBison,
 * but will need language specific parts to compile as C/C++ or Scala
 */

%token <Symbol> ID LITERAL
%token SEMICOLON EQ PLUS IF THEN ELSE WHILE DO END

%type <Stmt> stmt
%type <Stmts> stmts 
%type <Expression> expr
%type <Program> program

%left EQ
%left IF WHILE
%left PLUS

%%

program : stmts
     { $$ = program($1); }
        ;

stmts : /* NOTHING */
   { $$ = stmts_empty(); }
      | stmt SEMICOLON stmts
   { $$ = stmts_append($1, $3); }
      ;

stmt : ID EQ expr
   { $$ = stmt_assign($1, $3); }
      | IF expr THEN stmts ELSE stmts END
   { $$ = stmt_if($2, $4, $6); }
      | WHILE expr DO stmts END
   { $$ = stmt_while($2, $4); }
      ;

expr : ID
   { $$ = expr_var($1); }
      | LITERAL
   { $$ = expr_lit($1); }
      | expr PLUS expr
   { $$ = expr_add($1, $3); }
      ;
%%

