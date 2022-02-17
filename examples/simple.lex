%{
  /* An incomplete scanner for 'simple' */
%}

%%

[{};=]	{ return YYCHAR(yytext); }

"int"	{ return INT; }
"string"	{ return STRING; }

[a-zA-Z_][a-zA-Z_0-9]*	{ return ID(yytext); }

[0-9]+	{ return INT_LITERAL(yytext); }

\"([^\"\n\\]|\\(.|\n))*\"	{ return STR_LITERAL(yytext); }

[ \t\r\n]+	{}

<<EOF>> { return SimpleTokens.YYEOF(); }
