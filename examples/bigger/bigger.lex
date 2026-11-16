%{
  /* Scanner rules for the Bigger example language. */
%}

%%

"//".*	{}

"integer"	{ return INTEGER; }
"string"	{ return STRING; }
"boolean"	{ return BOOLEAN; }
"array"	{ return ARRAY; }
"if"	{ return IF; }
"else"	{ return ELSE; }
"while"	{ return WHILE; }
"substring"	{ return SUBSTRING; }
"concat"	{ return CONCAT; }
"true"	{ return BOOL_LITERAL(true); }
"false"	{ return BOOL_LITERAL(false); }

"=="	{ return EQUAL; }
"||"	{ return LOGOR; }

[{}\[\]();,=+<]	{ return YYCHAR(yytext); }

[a-zA-Z_][a-zA-Z_0-9]*	{ return ID(yytext); }

0|[1-9][0-9]*	{ return INT_LITERAL(yytext); }

\"([^\"\n\\]|\\.)*\"	{ return STR_LITERAL(yytext.substring(1, yytext.length - 1)); }

[ \t\r\n]+	{}

.	{ throw new IllegalArgumentException(
	    "Unexpected character '" + yytext + "' at line " + getLineNumber()); }

<<EOF>> { return YYEOFT; }
