#ifndef APS_LEX_H
#define APS_LEX_H

/* define yy things used in both aps.y and aps.lex */

#ifndef yylex
#define yylex aps_yylex
#endif
#ifndef yyerror
#define yyerror aps_yyerror
#endif
#define yylineno aps_yylineno
#define yyfilename aps_yyfilename
#define yytoken aps_yytoken
#ifndef yydebug
#define yydebug aps_yydebug
#endif

#define LEX_ERROR ' ' /* psuedo token for communication with yyerror */
extern int yylineno;
extern char *yyfilename;
extern int yytoken;

extern int yydebug;
#define YY_USER_ACTION if (yydebug) {printf("Lexed: \"%s\"\n",yytext);}

extern void set_code_name(Symbol sym, String s);
extern String get_code_name(Symbol sym);

#endif
