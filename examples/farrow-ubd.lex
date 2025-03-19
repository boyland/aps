%{
  /* Farrow's use-before-declaration analysis lexer */
%}

%%

";"	{ return SEMICOLON; }
"="	{ return EQ; }
"+"	{ return PLUS; }
"-"	{ return MINUS; }
"/"	{ return DIV; }
"*"	{ return MUL; }
"{"	{ return OPN_BRACE; }
"}"	{ return CLS_BRACE; }

[a-z][_A-Za-z0-9]*	{ return ID(yytext); }
[1-9][0-9]* 	{ return LITERAL(yytext); }

[\s\r\n]  { /* ignore spaces */ }

"//".*    { /* ignore comments */ }

<<EOF>> { return YYEOFT; }
