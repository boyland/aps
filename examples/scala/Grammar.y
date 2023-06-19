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
%type <Productions> productions production
%%

grammar : productions
     { $$ = grammar($1); }
        ;

productions : /* NOTHING */
   { $$ = productions_none(); }
      | productions production
   { $$ = productions_append($1, $2); }
      ;

production : NONTERMINAL COLON items_many SEMICOLON
   { $$ = prods($1, $3); }
      ;

items_many : items
  { $$ = items_many_single($1); }
    | items_many PIPE items
  { $$ = items_many_append($1, items_many_single($3)); }
    ;

items :  /* NOTHING */
   { $$ = items_none(); }
      | items item
   { $$ = items_append($1, items_single($2)); }
      ;

item : TERMINAL
  { $$ = terminal($1); }
     | NONTERMINAL
  { $$ = nonterminal($1); }
     ;

%%

var scanner : GrammarScanner = null;
var filename : String = "<unknown>";
var result : Grammar = null;

def get_result() : Grammar = result;

def reset(sc : GrammarScanner, fn : String) : Unit = {
  filename = fn;
  scanner = sc;
  yyreset(sc)
};

override def get_line_number() : Int = scanner.getLineNumber();

/* This function is called automatically when Bison detects a parse error. */
def yyerror(message : String) : Unit = {
  print (filename + ":" + scanner.getLineNumber() + ": " +
	     message + ", at or near " + yycur + "\n");
};

override def grammar(b:Productions) : Grammar = {
  var res = super.grammar(b);
  result = res;
  res
}
