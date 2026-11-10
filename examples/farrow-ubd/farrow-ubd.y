/* Farrow's live-variable analysis parser */

/*
 * This fragment is intended to be used either by bison or ScalaBison,
 * but will need language specific parts to compile as C/C++ or Scala
 */

%token <Symbol> ID LITERAL
%token SEMICOLON EQ OPN_BRACE CLS_BRACE
%token PLUS MINUS MUL DIV

%type <Declaration> decl
%type <Declarations> decls
%type <Expression> expr
%type <Term> term
%type <Operation> op
%type <Program> program

%left EQ
%left OPN_BRACE CLS_BRACE
%left PLUS MINUS
%left MUL DIV

%%

program : decls
     { $$ = program(scope($1)); }
        ;

decls : /* NOTHING */
   { $$ = decls_empty(); }
      | decls decl
   { $$ = decls_append($1, $2); }
      | OPN_BRACE decls CLS_BRACE
   { $$ = scope($2); }
      ;

decl : ID EQ expr SEMICOLON
   { $$ = decl_assign($1, $3); }   	   
      ;

term : ID
   { $$ = term_variable($1); }
      | LITERAL
   { $$ = term_literal($1); }
      ;

expr : term
   { $$ = expr_term($1); }
      | expr op term
   { $$ = expr_apply($1, $2, $3); }
      ;

op   : PLUS
   { $$ = op_add(); }
      | MINUS
   { $$ = op_sub(); }
      | MUL
   { $$ = op_mul(); }
      | DIV
   { $$ = op_div(); }
      ;

%%

