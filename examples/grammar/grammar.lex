%{
  /* Scanner for 'grammar' */
%}

%%

":"	{ return COLON; }

";"	{ return SEMICOLON; }

"|"	{ return PIPE; }

[a-z][\w]*	{ return TERMINAL(yytext); }

[A-Z][\w]*	{ return NONTERMINAL(yytext); }

[\s\r\n]  { /* ignore spaces */ }

"//".*    { /* ignore comments */ }

<<EOF>> { return YYEOFT; }
