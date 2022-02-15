%{
  /* An incomplete scanner for 'simple' */
%}

%%

[{};=]	{ return YYCHAR(yytext); }

"int"	{ return INT; }
"string"	{ return STR; }

[a-zA-Z_][a-zA-Z_0-9]*	{ return ID(yytext); }

[0-9]+	{ return INTLITERAL(yytext); }

\"([^\"\n\\]|\\(.|\n))*\"	{ return STRLITERAL(yytext); }


