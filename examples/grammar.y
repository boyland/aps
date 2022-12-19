/* Complete parser for grammar.y */

/*
 * This fragment is intended to be used either by bison or ScalaBison,
 * but will need language specific parts to compile as C/C++ or Scala
 */

%token <Symbol> TERMINAL NONTERMINAL
%token SEMICOLON PIPE COLON

%type <Grammar> grammar
%type <Items> items
%type <Item> item
%type <ItemsMany> items_many
%type <Productions> productions production_alt
%%

grammar : productions
     { $$ = grammar($1); }
        ;

productions : /* NOTHING */
   { $$ = productions_none(); }
      | production_alt SEMICOLON productions
   { $$ = productions_append($1, $3); }
      ;

production_alt : NONTERMINAL COLON items_many
   { $$ = prods($1, $3); }
      ;

items_many : items
  { $$ = items_many_single($1); }
    | items_many PIPE items
  { $$ = items_many_append($1, items_many_single($3)); }
    ;

items :  /* NOTHING */
   { $$ = items_none(); }
      | item items
   { $$ = items_append(items_single($1), $2); }
      ;

item : TERMINAL
  { $$ = terminal($1); }
     | NONTERMINAL
  { $$ = nonterminal($1); }
     ;

%%

