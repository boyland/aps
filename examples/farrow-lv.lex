%{
  /* Farrow's live-variable analysis lexer */
%}

%%

";"	{ return SEMICOLON; }
"="	{ return EQ; }
"+"	{ return PLUS; }

"IF"	{ return IF; }
"WHILE"	{ return WHILE; }
"THEN"	{ return THEN; }
"ELSE"	{ return ELSE; }
"DO"	{ return DO; }
"END"	{ return END; }

[a-z][_A-Za-z0-9]*	{ return ID(yytext); }
\" ([^\"]*) \" | ([1-9][0-9]*) 	{ return LITERAL(yytext); }

[\s\r\n]  { /* ignore spaces */ }

"//".*    { /* ignore comments */ }

<<EOF>> { return YYEOFT; }
