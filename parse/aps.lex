%{
#define yyin aps_yyin
#define yylex aps_yylex
#define yylval aps_yylval
#define yywrap() 1

struct symbol_info {
  struct string *code_name;
  int infix_info;
};

#define SYMBOL_INFO struct symbol_info
int symbol_info_size = sizeof(SYMBOL_INFO);

#include <stdio.h>
#include "jbb.h"
#include "jbb-string.h"
#include "jbb-symbol.h"
#include "aps-tree.h"
#include "aps.tab.h"
#include "aps-lex.h"

int yytoken = 0;
int yylineno = 1;
char *yyfilename = "<stdin>";
FILE *yyin;

static SYMBOL intern_special(char *);
static SYMBOL intern_special_np(char *);
static SYMBOL intern_symbol_np(char *);

static int warned_about_underline = FALSE;

%}

special		[~@#$%^&*+=<>/\\|]
	/* we have to use a fancy pattern to ensure -- is always a comment */
operator	(({special}|"-"{special})+"-"?)

%%
\n		++yylineno;
[ \t\r\f]	;
--.*	;
	/* not available as operators */
[[\](){}:;.,?!]		{ return yytoken=*yytext; }
	/* we have to handle '=' and '-' specially: otherwise matched as ops */
"-"/[^\-]	{return yytoken='-'; }
"="/[^\-]	{return yytoken='='; }
"$"/[^\-]	{return yytoken='$';}
"->"/[^\-]	{return yytoken=ARROW; }
":="	{return yytoken=COLONEQ;}
":>"	{return yytoken=COLONGR;}
".."	{return yytoken=DOTDOT;}
"::"	{return yytoken=DOUBLECOLON;}
"..."	{return yytoken=DOTDOTDOT;}
":::"	{return yytoken=TRIPLECOLON; }
":?"	{return yytoken=MATCH; }
{operator}/[^\-]	   { yylval.symbol = intern_special(yytext);
			     yytoken = get_infix(yylval.symbol);
			     if (yytoken == 0) yytoken = OPERATOR;
			     return yytoken; }
"'"(\\(.|\n)|[^\n\\'])*"'" { yylval.string = make_saved_string(yytext); 
			     return yytoken=CHARACTER_CONSTANT; }
"'"(\\(.|\n)|[^\n\\'])*    { yylval.string = make_saved_string(yytext); 
			     yytoken = LEX_ERROR;
			     yyerror("unterminated character constant");
			     return yytoken=CHARACTER_CONSTANT; }
\"(\\(.|\n)|[^\n\\\"])*\"  { yylval.string = make_saved_string(yytext); 
			     return yytoken=STRING_CONSTANT; }
\"(\\(.|\n)|[^\n\\\"])*    { yylval.string = make_saved_string(yytext); 
			     yytoken = LEX_ERROR;
			     yyerror("unterminated string constant");
			     return yytoken=STRING_CONSTANT; }
[A-Za-z][A-Za-z0-9_]*	{ yylval.symbol = intern_symbol(yytext);
			  return yytoken=symbol_token(yylval.symbol); }
"_"			{
                          static int unique=0;
			  static char buf[10];
			  sprintf(buf,"_%d",++unique);
			  yylval.symbol = intern_symbol(buf);
			  return yytoken=symbol_token(yylval.symbol); }
[0-9][A-Za-z0-9_]*	{ yylval.string = make_saved_string(yytext);
			  return yytoken=INTEGER; }
[0-9][A-Za-z0-9_]*"."[0-9][A-Za-z0-9]* { yylval.string = make_saved_string(yytext);
				    return yytoken=REAL; }
"_"[A-Za-z0-9_]*	{ if (!warned_about_underline) {
                            yytoken = LEX_ERROR;
			    yyerror("identifiers may not start with _");
			    warned_about_underline = TRUE;
			  }
			  yylval.symbol = intern_symbol(yytext);
			  return yytoken=symbol_token(yylval.symbol); }
"`"[A-Za-z_][A-Za-z0-9_]*"`" { yylval.symbol = intern_symbol_np(yytext);
			       yytoken = get_infix(yylval.symbol);
			       if (yytoken == 0) yytoken = OPERATOR;
			       return yytoken; }
<<EOF>>			{ return yytoken = 0; }			 
.	{ char buf[100];
	  sprintf(buf,"character ignored: '%c' (0x%x)",*yytext,*yytext);
	  yytoken = LEX_ERROR;
	  yyerror(buf);
	  /* don't return */}
%%


char *keyword_names[] = {
"and",
"attribute",
/* "attribution", */
"begin",
"case",
"circular",
"class",
"collection",
"constant",	/* currently unused */
"constructor",
"elscase",
"else",
"elsif",
"end",
"endif",
"extends",
"for",
/* "foreign", */
"function",
/* "grammar", */
/* "group", */
"if",
"in",
"infix",
"infixl",
"infixr",
"inherit",
/* "inherited", */
"input",
/* "is", */
/* "language", */
/* "local", */
"match",
"module",
"not",
"on",
"or",
"pattern",
"phylum",
"pragma",
"private",
"procedure",
"public",
"remote",
/* "return", */
/* "set", */
"signature",
/* "synthesized", */
"then",
"type",
/* "use", */
"var",
"with",
""
};

int keyword_tokens[] = {
keyAND,
keyATTRIBUTE,
/* keyATTRIBUTION, */
keyBEGIN,
keyCASE,
keyCIRCULAR,
keyCLASS,
keyCOLLECTION,
keyCONSTANT,
keyCONSTRUCTOR,
keyELSCASE,
keyELSE,
keyELSIF,
keyEND,
keyENDIF,
keyEXTENDS,
keyFOR,
/* keyFOREIGN, */
keyFUNCTION,
/* keyGRAMMAR, */
/* keyGROUP, */
keyIF,
keyIN,
keyINFIX,
keyINFIXL,
keyINFIXR,
keyINHERIT,
/* keyINHERITED, */
keyINPUT,
/* keyIS, */
/* keyLANGUAGE, */
/* keyLOCAL, */
keyMATCH,
keyMODULE,
keyNOT,
keyON,
keyOR,
keyPATTERN,
keyPHYLUM,
keyPRAGMA,
keyPRIVATE,
keyPROCEDURE,
keyPUBLIC,
keyREMOTE,
/* keyRETURN, */
/* keySET, */
keySIGNATURE,
/* keySYNTHESIZED, */
keyTHEN,
keyTYPE,
/* keyUSE, */
keyVAR,
keyWITH,
0
};

#define FIRST_ID ((sizeof(keyword_names)/sizeof(char *))-1)

int symbol_token(SYMBOL s) {
  int i = symbol_id(s);
  if (i < FIRST_ID) {
    if (!streq(symbol_name(s),keyword_names[i])) {
      fprintf(stderr,"panic: expected \"%s\", got \"%s\"\n",
	      keyword_names[i],symbol_name(s));
      exit(1);
    }
    return keyword_tokens[i];
  }
  return IDENTIFIER;
}

void set_code_name(Symbol sym, String s) {
  symbol_info(sym)->code_name = s;
}

String get_code_name(Symbol sym) {
  return symbol_info(sym)->code_name;
}

void set_infix(Symbol sym, int kind) {
  symbol_info(sym)->infix_info = kind;
  /* printf("infix[%s] := %d\n",symbol_name(sym),kind); */
}

int get_infix(Symbol sym) {
  /* printf("infix[%s] = %d\n",symbol_name(sym),symbol_info(sym)->infix_info);
   */
  return symbol_info(sym)->infix_info;
}

static char translate[256]; /* ASCII */
static void make_code_name(SYMBOL sym, int is_op) {
  static char special_buf[20] = {'_', 'o', 'p', '_', 0};
  int i = 4;
  unsigned char *s;
  for (s = (unsigned char *)symbol_name(sym); *s != '\0'; ++s) {
    special_buf[i] = translate[*s];
    ++i;
  }
  special_buf[i] = '\0';
  set_code_name(sym,make_saved_string(is_op ? special_buf : (special_buf+3)));
}
  
static SYMBOL intern_special_np(char *text) {
  text[strlen(text)-1] = '\0'; /* cut off final paren */
  return intern_special(text+1);
}

static SYMBOL intern_special(char *text) {
  SYMBOL sym;

  sym = intern_symbol(text);
  make_code_name(sym,1);

#ifdef DEBUGLEX
  printf("interned: %s (codename = %s, infix = %d)\n",
  	 text,get_code_name(sym),get_infix(sym));
#endif

  return sym;
}

static SYMBOL intern_symbol_np(char *text) {
  text[strlen(text)-1] = '\0'; /* cut off final backquote */
  return intern_symbol(text+1);
}

static char *impl_reserved_names[] = {
  /* how strange I can't find a file of C keywords */
  "auto",
  "case",
  "char",
  "const",
  "default",
  "do",
  "double",
  "entry",
  "extern",
  "float",
  "for",
  "int",
  "long",
  "sizeof",
  "static",
  "struct",
  "switch",
  "typedef",
  "union",
  "virtual",
  "void",
  "while",
  /* names we need to reserve, (that don't start with _) */
  NULL,
  };

static void init_lexer_tables()
{
  int i,j;
  int code = 0;
  for (i=0; i < 256; ++i) {
    if ('a' <= i && i <= 'z' ||
	'A' <= i && i <= 'Z' ||
	'0' <= i && i <= '9') {
      translate[i] = i; /* leave as is */
    } else {
      code = (code+1) % 62;
      if (code < 26) {
	translate[i] = code + 'A';
      } else if (code < 52) {
	translate[i] = code + 'a' - 26;
      } else if (code < 62) {
	translate[i] = code + '0' - 52;
      }
    }
  }
  init_symbols();
  for (i=0; i < FIRST_ID; ++i) {
    if ((j=symbol_id(intern_symbol(keyword_names[i]))) != i) {
      fprintf(stderr,"panic: interned \"%s\", got id = %d\n",
	      keyword_names[i],j);
    }
  }
  /* we now use prefixes and do not need to do the following */
#ifdef NO_PREFIX
  /* must do this *after* interning all APS keywords */
  for (i=0; impl_reserved_names[i] != 0; ++i) {
    make_code_name(intern_symbol(impl_reserved_names[i]),0);
  }
#endif
  intern_special("-");
  intern_special("=");
  intern_special("{}");
  intern_special("..");
}

void init_lexer(FILE *f) {
  static int inited = FALSE;
  yyrestart(f);
  yylineno = 1;
  if (!inited) {
    init_lexer_tables();
    inited = TRUE;
  }
  init_parser();
}

