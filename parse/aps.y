%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "aps-tree.h"
#include "aps-lex.h"
#include "jbb-symbol.h"
#include "jbb-string.h"

/* kludges */
#define no_stmt() no_decl()
#define no_block() block(nil_Declarations())
#define no_direction() direction(FALSE,FALSE,FALSE)

#define value_name(s) value_use(use(s))
#define type_name(s) type_use(use(s))
#define pattern_name(s) pattern_use(use(s))
#define sig_name(s) sig_use(use(s))

#define local_def(s) def(s,TRUE,FALSE)

extern void yyerror(char*);
extern void yylexerror(char*);

extern Program the_tree;

static Symbol underscore_symbol, eq_symbol, seq_symbol;
static int implicit_name_number = 0;
static char *filebase = "<unknown>";

void init_parser() {
  static int inited = FALSE;
  if (!inited) {
    underscore_symbol = intern_symbol("_");
    eq_symbol = intern_symbol("=");
    seq_symbol = intern_symbol("{}");
    inited = TRUE;
  }
  {
    char *root_start = strrchr(yyfilename,'/');
    char *root_end;
    if (root_start == NULL) {
      root_start = yyfilename;
    } else {
      ++root_start;
    }
    root_end = strrchr(root_start,'.');
    if (root_end == NULL) {
      root_end = root_start + strlen(root_start);
    }
    if ((filebase = (char *)malloc(root_end-root_start+1)) == NULL) {
      filebase = "unknown";
    } else {
      strncpy(filebase,root_start,root_end-root_start);
      filebase[root_end-root_start] = '\0';
    }
  }
  implicit_name_number = 0;
}

static Symbol make_implicit_name() {
  char *name = (char *)malloc(strlen(filebase)+20);
  Symbol res;
  char *t;
  if (name == NULL) return gensym();
  sprintf(name,"_%s_%d",filebase,++implicit_name_number);
  for (t=name; *t; ++t) {
    if (!isalnum(*t)) *t = '_';
  }
  /*printf("Generating implicit name %s\n",name);*/
  res = intern_symbol(name);
  free(name);
  return res;
}

int public = FALSE; /* toggled by "private" and "public" */
int constant = TRUE; /* cleared by "var" */

static Expression sbinop(Symbol sym, Expression e1, Expression e2) {
  return funcall(value_name(sym),
		 xcons_Actuals(list_Actuals(e1),e2));
}

static Expression binop(char *name, Expression e1, Expression e2) {
  return sbinop(intern_symbol(name),e1,e2);
}

static Expression unop(char *name, Expression e) {
  /* fprintf(stderr,"creating unop for \"%s\"\n",name); */
  return funcall(value_name(intern_symbol(name)),
		 list_Actuals(e));
}

static Expression sunop(Symbol sym, Expression e1) {
  const char *s = symbol_name(sym);
  char *us = (char *)malloc(strlen(s)+2);
  Expression result;
  if (us == NULL) {
    yylexerror("out of memory");
    us = "#+";
  } else {
    us[0] = '#';
    strcpy(us+1,s);
  }
  result = unop(us,e1);
  free(us);
  return result;
}
			        
/* we still have to re make the dump stuff */
static Pattern match_expression(Expression e) {
  Symbol s = gensym();
  return and_pattern(pattern_var(normal_formal(local_def(s),no_type())),
		     condition(binop("=",value_name(s),e)));
}

static Expression make_sequence(Actuals as, Type ty) {
  if (ty == NULL) 
    return funcall(value_use(use(seq_symbol)),as);
  else
    return funcall(value_use(qual_use(ty,seq_symbol)),as);

}

static Pattern make_sequence_pattern(PatternActuals ps, Type ty) {
  if (ty == NULL) 
    return pattern_call(pattern_use(use(seq_symbol)),ps);
  else
    return pattern_call(pattern_use(qual_use(ty,seq_symbol)),ps);
}

static Declaration first_Declaration(Declarations l) {
  switch (Declarations_KEY(l)) {
  case KEYlist_Declarations:
    return list_Declarations_elem(l);
  case KEYappend_Declarations:
    return first_Declaration(append_Declarations_l1(l));
  default:
    fatal_error("first_Declaration got empty list");
    return NULL;
  }
}

#define cons_Actuals(x,l) \
	append_Actuals(list_Actuals(x),l)
#define cons_PatternActuals(x,l) \
	append_PatternActuals(list_PatternActuals(x),l)

#define return_decl(name,type) \
	value_decl(def((name)!=0?(name):underscore_symbol,TRUE,FALSE), \
		   type, \
		   direction(FALSE,FALSE,FALSE), \
		   no_default())
#define attr_return_decl(type,dir,deft) \
        value_decl(def(underscore_symbol,TRUE,FALSE), \
		   type, copy_Direction(dir), copy_Default(deft))

static int infix_table[10][2] = {
  {OPL0, OPR0},
  {OPL1, OPR1},
  {OPL2, OPR2},
  {OPL3, OPR3},
  {OPL4, OPR4},
  {OPL5, OPR5},
  {OPL6, OPR6},
  {OPL7, OPR7},
  {OPL8, OPR8},
  {OPL9, OPR9}
};

%}

%union { /* two elements for each abstract non-terminal, exceptions noted*/
  Signature signature;
  Type type;
  Expression expression;
  Program program;
  Unit unit;
  Declaration declaration;
  Block block;
  Pattern pattern;
  Match match;
  Direction direction;
  Default default_tag;
  Def def;
  Use use;
  Units units;
  Declarations declarations;
  Actuals actuals;
  PatternActuals patternActuals;
  Types types;
  Expressions expressions;
  Patterns patterns;
  Matches matches;
  TypeActuals typeActuals;
  Formals formals;
  Formal formal;
  TypeFormals typeFormals;

  String string;
  Symbol symbol;
  int integer;
  int boolean;
}

%token <symbol> keyAND
%token <symbol> keyATTRIBUTE
/* %token <symbol> keyATTRIBUTION */
%token <symbol> keyBEGIN
%token <symbol> keyCASE
%token <symbol> keyCIRCULAR
%token <symbol> keyCLASS
%token <symbol> keyCOLLECTION
%token <symbol> keyCONSTANT
%token <symbol> keyCONSTRUCTOR
%token <symbol> keyELSCASE
%token <symbol> keyELSE
%token <symbol> keyELSIF
%token <symbol> keyEND
%token <symbol> keyENDIF
%token <symbol> keyEXTENDS
%token <symbol> keyFOR
/* %token <symbol> keyFOREIGN */
%token <symbol> keyFUNCTION
/* %token <symbol> keyGRAMMAR */
/* %token <symbol> keyGROUP */
%token <symbol> keyIF
%token <symbol> keyIN
%token <symbol> keyINFIX
%token <symbol> keyINFIXL
%token <symbol> keyINFIXR
%token <symbol> keyINHERIT
/* %token <symbol> keyINHERITED */
%token <symbol> keyINPUT
/* %token <symbol> keyIS */
/* %token <symbol> keyLANGUAGE */
/* %token <symbol> keyLOCAL */
%token <symbol> keyMATCH
%token <symbol> keyMODULE
%token <symbol> keyNOT
%token <symbol> keyON
%token <symbol> keyOR
%token <symbol> keyPATTERN
%token <symbol> keyPHYLUM
%token <symbol> keyPRAGMA
%token <symbol> keyPRIVATE
%token <symbol> keyPROCEDURE
%token <symbol> keyPUBLIC
%token <symbol> keyREMOTE
/* %token <symbol> keyRETURN */
/* %token <symbol> keySET */
%token <symbol> keySIGNATURE
/* %token <symbol> keySYNTHESIZED */
%token <symbol> keyTHEN
%token <symbol> keyTYPE
/* %token <symbol> keyUSE */
%token <symbol> keyVAR
%token <symbol> keyWITH

%token DOTDOT
%token DOUBLECOLON
%token DOTDOTDOT
%token TRIPLECOLON
%token ARROW

%token IDENTIFIER
%token OPERATOR
%token INTEGER
%token REAL
%token STRING_CONSTANT
%token CHARACTER_CONSTANT

%token MATCH

%nonassoc COLONEQ COLONGR
%left ','
%left keyIF keyFOR ':'
%left keyOR
%left keyAND
%nonassoc keyNOT keyREMOTE /* keyFOREIGN */ keyIN

%left DOTDOTDOT
%left OPL0 DOTDOT
%right OPR0
%left OPL1
%right OPR1
%left OPL2
%right OPR2
%left OPL3
%right OPR3
%left OPL4 '='
%right OPR4
%left OPL5
%right OPR5
%left OPL6
%right OPR6 UNARY '-'
%left OPL7
%right OPR7
%left OPL8
%right OPR8
%left OPL9 OPERATOR
%right OPR9

%type <symbol> OPL0 OPR0 OPL1 OPR1 OPL2 OPR2 OPL3 OPR3 OPL4 OPR4
%type <symbol> OPL5 OPR5 OPL6 OPR6 OPL7 OPR7 OPL8 OPR8 OPL9 OPR9

%type <units> unit_list
%type <unit> unit with
%type <declarations> decl_list 
%type <block> module_body decl_block
%type <declaration> decl inherit var_decl function_decl type_decl 
%type <declaration> phy_decl con_decl pat_decl class_decl signature_decl
%type <declaration> attribute_decl top_level_match pragma module_decl
%type <declaration> replacement
%type <direction> opt_direction direction
%type <default_tag> default
%type <typeFormals> polymorphism type_formals
%type <typeFormals> phylum_formal type_formal
%type <formals> formals formal formal_list
%type <formal> opt_formal
%type <type> type opt_typing simple_type type_prefix
%type <type> opt_extension
%type <pattern> pattern pattern_body simple_pattern Pattern pattern_arg
%type <patternActuals> simple_pattern_args non_empty_pattern_args
%type <patterns> choices 
%type <block> stmts opt_body stmt_elsif_part stmt_match_default
%type <declaration> stmt
%type <declaration> result_value single_result_value single_result_type
%type <declarations> stmt_list multi_result_values multi_result_value
%type <declarations> multi_result_typing multi_result_types
%type <matches> stmt_match_list
%type <match> match
%type <expression> atomic_expr simple_expr expr Expr
%type <expression> lhs rhs actual sequence_actual pragma_actual
%type <expressions> pragma_actuals
%type <actuals> sequence_actuals non_empty_sequence_actuals
%type <actuals> actuals non_empty_actuals
%type <pattern> atomic_pattern pattern_name
%type <symbol> IDENTIFIER OPERATOR opt_id id id_or_Result id_or_result
%type <def> implicit_name
%type <string> INTEGER REAL STRING_CONSTANT CHARACTER_CONSTANT
%type <integer> optional_integer
%type <boolean> opt_PHYLUM PHYLUM
%type <typeActuals> type_actuals type_parameters is_type_parameters
%type <types> types non_empty_types
%type <signature> signature opt_signatures
%type <signature> simple_signature
%type <use> qual_use

%%

program : { public=TRUE; } unit_list
  		{ the_tree = program(make_saved_string(filebase),$2); }
	;

unit_list
 	: /* nothing */
		{ $$ = nil_Units(); }
	| unit_list unit
		{ $$ = xcons_Units($1,$2); }
	| unit_list fixity ';'
		{ $$ = $1; }
	| unit_list keyPRIVATE ';'
		{ $$ = $1; public = FALSE; }
	| unit_list keyPUBLIC ';'
		{ $$ = $1; public = TRUE; }
/*	| unit_list ';'
		{ $$ = $1; } */
	;

unit 	: with
		{ $$ = $1; }
	| decl
		{ $$ = decl_unit($1); }
	;

fixity	: keyINFIX optional_integer OPERATOR
		{ /* printf("infix[%s] := %d ?\n",symbol_name($3),$2); */
		  set_infix($3,infix_table[$2][0]); }
	| keyINFIXL optional_integer OPERATOR
		{ set_infix($3,infix_table[$2][0]); }
	| keyINFIXR optional_integer OPERATOR
		{ set_infix($3,infix_table[$2][1]); }
	;

optional_integer
	: /* EMPTY */
		{ $$ = 9; }
	| INTEGER
		{ char *s = (char *)$1;
		  if (s[1] != '\0') {
		    yyerror("fixity levels must be single digits:");
		  }
		  $$ = s[0]-'0';
		}
	;

with
	: keyWITH STRING_CONSTANT ';'
		{ $$ = with_unit($2); }
	;

decl_list
	:	{ $$ = nil_Declarations(); }
/*	| decl_list ';'
		{ $$ = $1; } */
	| decl_list keyPRIVATE ';'
		{ $$ = $1; public = FALSE; }
	| decl_list keyPUBLIC ';'
		{ $$ = $1; public = TRUE; }
	| decl_list keyVAR ';'
		{ $$ = $1; constant = FALSE; }
	| decl_list decl
		{ $$ = xcons_Declarations($1,$2); }
	;


decl	: class_decl
	| module_decl
	| signature_decl
	| type_decl
	| function_decl
	| var_decl
	| phy_decl
	| con_decl
	| pat_decl
	| attribute_decl
	| pragma
	| inherit
	| replacement
	| top_level_match
	| implicit_name polymorphism decl 
		{ $$ = polymorphic($1,$2,block(list_Declarations($3))); }
	| implicit_name polymorphism decl_block
		{ $$ = polymorphic($1,$2,$3); }
	| keyPRIVATE { $<integer>$ = public; public = FALSE; } decl
		{ $$ = $3; public = $<integer>2; }
	| keyPUBLIC { $<integer>$ = public; public = TRUE; } decl
		{ $$ = $3; public = $<integer>2; }
/*	| keyCONSTANT decl 
		{ $$ = $2; } */
	| keyVAR { $<integer>$ = constant; constant = FALSE; } decl
		{ $$ = $3; constant = $<integer>2; }
	| error
		{ $$ = no_decl(); }
	;

class_decl
	: keyCLASS id polymorphism
              id_or_Result opt_signatures
          module_body
		{ $$ = class_decl(def($2,constant,public),
				  $3,
				  type_formal(local_def($4),no_sig()),
				  $5,$6); }
	| keyCLASS id '=' qual_use ';'
		{ $$ = class_renaming(def($2,constant,public),
				      class_use($4)); }
	;

signature_decl
	: keySIGNATURE id COLONEQ signature ';'
		{ $$ = signature_decl(def($2,constant,public),$4); }
	| keySIGNATURE id '=' qual_use ';'
		{ $$ = signature_renaming(def($2,constant,public),
					  sig_use($4));}
	| keySIGNATURE id '='
          '{' {yylexerror("'=' used for ':='");} types is_close_brace ';'
		{ $$ = signature_decl(def($2,constant,public),fixed_sig($6)); }
	;

/* module decl cannot be factored very much because
 * if the formals are missing the '(' starting an identifier
 * could look like the start of a formals list.
 */
module_decl
	: keyMODULE id polymorphism formals opt_signatures
              opt_PHYLUM id_or_Result opt_extension
          module_body
		{ $$ = module_decl(def($2,constant,public),$3,$4,
				   $6 ? phylum_decl(local_def($7),no_sig(),$8)
				      : type_decl(local_def($7),no_sig(),$8),
				   $5,$9); }
	| keyMODULE id polymorphism /* no formals */ isDOUBLECOLON signature
              opt_PHYLUM id_or_Result opt_extension
	  module_body
		{ $$ = module_decl(def($2,constant,public),$3,nil_Formals(),
				   $6 ? phylum_decl(local_def($7),no_sig(),$8)
				      : type_decl(local_def($7),no_sig(),$8),
				   $5,$9); }
	| keyMODULE id polymorphism /* no formals */ /* no signatures */
              PHYLUM id_or_Result opt_extension
	  module_body
		{ $$ = module_decl(def($2,constant,public),$3,nil_Formals(),
				   $4 ? phylum_decl(local_def($5),no_sig(),$6)
				      : type_decl(local_def($5),no_sig(),$6),
				   no_sig(),$7); }
	| keyMODULE id polymorphism /* no formals */ /* no signatures */
              /* no PHYLUM */ id_or_Result opt_extension
	  module_body
		{ $$ = module_decl(def($2,constant,public),$3,nil_Formals(),
				   type_decl(local_def($4),no_sig(),$5),
				   no_sig(),$6); }
	| keyMODULE id '=' qual_use ';'
		{ $$ = module_renaming(def($2,constant,public),
				       module_use($4)); }
	;

opt_PHYLUM
	: /* empty */
		{ $$ = FALSE; }
	| PHYLUM
		{ $$ = $1; }
	;

PHYLUM	: keyTYPE
		{ $$ = FALSE; }
	| keyPHYLUM
		{ $$ = TRUE; }
	;

id_or_Result	
	: /* empty */
		{ $$ = intern_symbol("Result"); }
	| id
		{ $$ = $1; }
	;

id_or_result
	: /* empty */
		{ $$ = intern_symbol("result"); }
	| id
		{ $$ = $1; }
	;

opt_extension
	: /* NULL */
		{ $$ = no_type(); }
	| '=' type
		{ yylexerror("used '=' for ':='"); $$ = $2; }
	| COLONEQ type
		{ $$ = $2; }
	| keyEXTENDS type
		{ $$ = $2; }
	;

module_body
	: /* NULL */ ';'
		{ $$ = no_block(); }
	| keyBEGIN { $<integer>$ = public; public = TRUE; }
	    decl_list 
	  iskeyEND is_semi
		{ public = $<integer>2;
                  $$ = block($3); }
	;

decl_block
	: keyBEGIN { $<integer>$ = public; }
	    decl_list 
	  iskeyEND is_semi
		{ public = $<integer>2;
                  $$ = block($3); }
	;

pragma
	: keyPRAGMA id ';'
		{ $$ = pragma_call($2,nil_Expressions()); }
	| keyPRAGMA id '(' ')' ';'
		{ $$ = pragma_call($2,nil_Expressions()); }
	| keyPRAGMA id '(' pragma_actuals is_close_paren ';'
		{ $$ = pragma_call($2,$4); }
	;

pragma_actuals
	: pragma_actual
		{ $$ = list_Expressions($1); }
	| pragma_actuals ',' pragma_actual
		{ $$ = append_Expressions($1,list_Expressions($3)); }
	;

pragma_actual
	: expr
		{ $$ = $1; }
	| keyPATTERN qual_use
		{ $$ = pattern_value(pattern_use($2)); }
	| keyTYPE qual_use
		{ $$ = type_value(type_use($2)); }
	| keyMODULE qual_use
		{ $$ = module_value(module_use($2)); }
	| keySIGNATURE qual_use
	  	{ $$ = signature_value(sig_use($2)); }
	| keyCLASS qual_use
	  	{ $$ = class_value(class_use($2)); }
	;

inherit
	: implicit_name keyINHERIT type decl_block
		{ $$ = inheritance($1,$3,$4); }
	;

replacement
	: id ARROW expr ';'
		{ $$ = value_replacement(value_name($1),$3); }
	| keyPATTERN id ARROW atomic_pattern ';'
		{ $$ = pattern_replacement(pattern_name($2),$4); }
	| keyTYPE id ARROW type ';'
		{ $$ = type_replacement(type_name($2),$4); }
	| keySIGNATURE id ARROW signature ';'
		{ $$ = signature_replacement(sig_name($2),$4); }
	;

phy_decl
	: keyPHYLUM id opt_signatures ';'
		{ $$ = phylum_decl(def($2,constant,public),
				   $3,
				   no_type()); }
	| keyPHYLUM id opt_signatures COLONEQ type ';'
		{ $$ = phylum_decl(def($2,constant,public), $3, $5); }
	| keyPHYLUM id opt_signatures '=' type ';'
		{ yylexerror("used '=' for ':='");
		  $$ = phylum_decl(def($2,constant,public), $3, $5); }
	;

con_decl
	: keyCONSTRUCTOR id formals result_value ';'
		{ $$ = constructor_decl
		    (def($2,constant,public),
		     function_type($3,list_Declarations($4))); }
	;

pat_decl
	: keyPATTERN id formals opt_id opt_typing pattern_body ';'
		{ $$ = pattern_decl
		    (def($2,constant,public),
		     function_type($3,list_Declarations(return_decl($4,$5))),
		     $6); }
	| keyPATTERN id ':' type pattern_body ';'
		{ $$ = pattern_decl(def($2,constant,public),$4,$5); }
	| keyPATTERN id '=' Pattern ';'
		{ $$ = pattern_renaming(def($2,constant,public),$4); }
	;

pattern_body
	: /* Empty */
		{ $$ = no_pattern(); }
	| COLONEQ choices
		{ $$ = choice_pattern($2); }
	| COLONEQ
		{ $$ = choice_pattern(nil_Patterns()); }
	| '=' choices
		{ yylexerror("used '=' for ':='");
		  $$ = choice_pattern($2); }
	| '='
		{ yylexerror("used '=' for ':='");
		  $$ = choice_pattern(nil_Patterns()); }
	;

attribute_decl /* trying out several different syntaxes
	: opt_direction keyATTRIBUTE id opt_formal ':' type default ';'
	  	{ $$ = attribute_decl
		    (def($3,FALSE,public),
		     function_type(list_Formals($4),
		                   list_Declarations(attr_return_decl($6,$1,$7))),
		     $1,$7); } */
	: opt_direction keyATTRIBUTE 
          opt_formal '.' id ':' type default ';'
		{ $$ = attribute_decl
		    (def($5,FALSE,public),
		     function_type(list_Formals($3),
				   list_Declarations(attr_return_decl($7,$1,$8))),
		     $1,$8); }
	;

opt_direction
	: /* Empty */
		{ $$ = direction(FALSE,FALSE,FALSE); }
	| direction
	;

direction
	: keyINPUT
		{ $$ = direction(TRUE,FALSE,FALSE); }
	| keyINPUT keyCOLLECTION
		{ $$ = direction(TRUE,TRUE,FALSE); }
	| keyINPUT keyCIRCULAR
		{ $$ = direction(TRUE,FALSE,TRUE); }
	| keyINPUT keyCIRCULAR keyCOLLECTION
		{ $$ = direction(TRUE,TRUE,TRUE); }
	| keyCOLLECTION
		{ $$ = direction(FALSE,TRUE,FALSE); }
	| keyCIRCULAR
		{ $$ = direction(FALSE,FALSE,TRUE); }
	| keyCIRCULAR keyCOLLECTION
		{ $$ = direction(FALSE,TRUE,TRUE); }
	;

opt_formal
	: type
		{ $$ = normal_formal(local_def(underscore_symbol),$1); }
	| '(' id ':' type is_close_paren
		{ $$ = normal_formal(local_def($2),$4); }
	;

default
	: /* Empty */
		{ $$ = no_default(); }
	| COLONEQ expr
		{ $$ = simple($2); }
	| '=' expr
		{ yylexerror("= used for :=");
		  $$ = simple($2); }
	| COLONGR expr
		{ $$ = composite($2,no_expr()); }
	| COLONEQ expr ',' expr
		{ yylexerror(":= used for :>");
		  $$ = composite($2,$4); }
	| COLONGR expr ',' expr
		{ $$ = composite($2,$4); }
	;

top_level_match
	: match
		{ $$ = top_level_match($1); }
	;

type_decl
	: keyTYPE id opt_signatures ';'
		{ $$ = type_decl(def($2,constant,public), $3, no_type()); }
	| keyTYPE id opt_signatures COLONEQ type ';'
		{ $$ = type_decl(def($2,constant,public), $3, $5); }
	| keyTYPE id isDOUBLECOLON signature '=' type ';'
		{ yylexerror("used '=' for ':='");
		  $$ = type_decl(def($2,constant,public), $4, $6); }
	| keyTYPE id '=' qual_use ';'
		{ $$ = type_renaming(def($2,constant,public),
				     type_use($4)); }
	;

var_decl
	: direction id ':' type default ';'
		{ $$ = value_decl(def($2,constant,public),$4,$1,$5); }
	| id ':' type default ';'
		{ $$ = value_decl(def($1,constant,public),$3,
				  direction(FALSE,FALSE,FALSE),$4); }
	| id '=' expr ';'
		{ $$ = value_renaming(def($1,constant,public),$3); }
	;

function_decl
	: keyFUNCTION id formals multi_result_value opt_body ';'
		{ $$ = function_decl(def($2,constant,public),
				     function_type($3,$4),
				     $5); }
	| keyPROCEDURE id formals multi_result_value opt_body ';'
		{ $$ = procedure_decl(def($2,constant,public),
				      function_type($3,$4),
				      $5); }
	;

multi_result_value
	: /* EMPTY */
		{ $$ = nil_Declarations() }
	| direction id_or_result ':' type  default
		{ $$ = list_Declarations(value_decl(def($2,FALSE,FALSE),
						    $4,$1,$5)); }
	| id_or_result ':' type  default
		{ $$ = list_Declarations(value_decl(def($1,FALSE,FALSE),
						    $3,no_direction(),$4)); }
	| '(' multi_result_values ')'
		{ $$ = $2; }
	;

multi_result_values
	: single_result_value
		{ $$ = list_Declarations($1); }
	| multi_result_values ';' single_result_value
		{ $$ = xcons_Declarations($1,$3); }
	;

single_result_value
	: opt_direction id ':' type default
		{ $$ = value_decl(def($2,FALSE,FALSE),$4,$1,$5); }

result_value
	: opt_direction id_or_result opt_typing default
		{ $$ = value_decl(def($2,FALSE,FALSE),$3,$1,$4); }
	;

multi_result_typing
	: /* EMPTY */
		{ $$ = nil_Declarations() }
	| id_or_result ':' type
		{ $$ = list_Declarations(value_decl(def($1,FALSE,FALSE),
						    $3,no_direction(),
						    no_default())); }
	| '(' multi_result_types ')'
		{ $$ = $2; }
	;

multi_result_types
	: single_result_type
		{ $$ = list_Declarations($1); }
	| multi_result_types ';' single_result_type
		{ $$ = xcons_Declarations($1,$3); }
	;

single_result_type
	: id ':' type
		{ $$ = value_decl(def($1,FALSE,FALSE),$3,
				  no_direction(),no_default()); }

opt_typing
	: /* EMPTY */
		{ $$ = no_type(); }
	| ':' type
		{ $$ = $2; }
	;

opt_body
	: /* EMPTY */
		{ $$ = no_block(); }
	| keyBEGIN stmts iskeyEND
		{ $$ = $2; }
	;

/*
opt_polymorphism
	: \* EMPTY *\
		{ $$ = nil_TypeFormals(); }
	| polymorphism
		{ $$ = $1; }
	;
*/

polymorphism
	: '[' type_formals is_close_bracket
		{ $$ = $2; }
	| '[' is_close_bracket
		{ $$ = nil_TypeFormals(); }
	;

type_formals
	: type_formal
		{ $$ = $1; }
	| keyTYPE type_formal
		{ $$ = $2; }
	| keyPHYLUM phylum_formal
		{ $$ = $2; }
	| type_formals ';' type_formal
		{ $$ = append_TypeFormals($1,$3); }
	| type_formals ';' keyTYPE type_formal
		{ $$ = append_TypeFormals($1,$4); }
	| type_formals ';' keyPHYLUM phylum_formal
		{ $$ = append_TypeFormals($1,$4); }
	;

type_formal
	: id opt_signatures
		{ $$ = list_TypeFormals(type_formal(local_def($1),$2)); }
	| id ',' type_formal
		{ Signature sig = type_formal_sig(first_Declaration($3));
		  $$ =
		    append_TypeFormals(
		      list_TypeFormals(type_formal(local_def($1),sig)), $3); }
	;

phylum_formal
	: id opt_signatures
		{ $$ = list_TypeFormals(phylum_formal(local_def($1),$2)); }
	| id ',' phylum_formal
		{ Signature sig = phylum_formal_sig(first_Declaration($3));
		  $$ =
		    append_TypeFormals(
		      list_TypeFormals(phylum_formal(local_def($1),sig)), $3);}
	;

opt_signatures
	: /* EMPTY */
		{ $$ = no_sig(); }
	| isDOUBLECOLON signature
		{ $$ = $2; }
	;

isDOUBLECOLON
	: DOUBLECOLON {}
	| ':'	{ yylexerror("used ':' for '::'"); }
/*	| COLONEQ { yylexerror("used ':=' for '::'"); } */
	;

signature
	: signature ',' simple_signature
		{ $$ = mult_sig($1,$3); }
	| simple_signature
		{ $$ = $1; }
	;

simple_signature
	: qual_use
		{ $$ = sig_use($1); }
	| keyINPUT keyVAR qual_use is_type_parameters
		{ $$ = sig_inst(TRUE,TRUE,class_use($3),$4); }
	| keyVAR keyINPUT qual_use is_type_parameters
		{ $$ = sig_inst(TRUE,TRUE,class_use($3),$4); }
	| keyINPUT qual_use is_type_parameters
		{ $$ = sig_inst(TRUE,FALSE,class_use($2),$3); }
	| keyVAR qual_use is_type_parameters
		{ $$ = sig_inst(FALSE,TRUE,class_use($2),$3); }
	| qual_use type_parameters
		{ $$ = sig_inst(FALSE,FALSE,class_use($1),$2); }
	| '{' types is_close_brace
		{ $$ = fixed_sig($2); }
	| '(' signature is_close_paren
		{ $$ = $2; }
	;

is_type_parameters
	: /* EMPTY */
		{ yyerror("missing instantiation");
		  $$ = nil_TypeActuals(); }
	| type_parameters
		{ $$ = $1; }
	;

type_parameters
	: '[' is_close_bracket
		{ $$ = nil_TypeActuals(); }
	| '[' type_actuals is_close_bracket
		{ $$ = $2; }
	;

type_actuals
	: type
		{ $$ = list_TypeActuals($1); }
	| type_actuals ',' type
		{ $$ = xcons_TypeActuals($1,$3); }
	;

types
	: /* EMPTY */
		{ $$ = nil_Types(); }
	| non_empty_types
		{ $$ = $1; }
	;

non_empty_types
	: type
		{ $$ = list_Types($1); }
	| non_empty_types ',' type
		{ $$ = xcons_Types($1,$3); }
	;

formals
	: '(' formal_list is_close_paren
		{ $$ = $2; }
	| '(' is_close_paren
		{ $$ = nil_Formals(); }
	;

formal_list
	: formal
		{ $$ = $1; }
	| id ':' type DOTDOTDOT
		{ $$ = list_Formals(seq_formal(local_def($1),$3)); }
	| formal ';' formal_list
		{ $$ = append_Formals($1,$3); }
	;

formal	: id ':' type
		{ $$ = list_Formals(normal_formal(local_def($1),$3)); }
	| id ',' formal
		{ Type ty =
		    copy_Type(normal_formal_type(first_Declaration($3)));
		  $$ = append_Formals
		    (list_Formals(normal_formal(local_def($1),ty)), $3); }
	;

type	: simple_type
		{ $$ = $1; }
	| keyPRIVATE type
		{ $$ = private_type($2); }
	| keyREMOTE type
		{ $$ = remote_type($2); }
/*	| keyFOREIGN type
		{ $$ = foreign_type($2); } */
/*	| keyFUNCTION formals opt_typing
		{ $$ = function_type
		    ($2,
		     list_Declarations(value_decl(def(underscore_symbol,
						      FALSE,FALSE),
						  $3,
						  direction(FALSE,FALSE,FALSE),
						  no_default()))); } */
	| keyFUNCTION formals multi_result_typing
		{ $$ = function_type($2,$3); }
	;

/* simple types are not recursive */
simple_type
	: qual_use
		{ $$ = type_use($1); }
	| qual_use type_parameters
		{ $$ = type_inst(module_use($1),$2,nil_Actuals()); }
	| qual_use type_parameters actuals
		{ $$ = type_inst(module_use($1),$2,$3); }
	| '(' type is_close_paren
		{ $$ = $2; }
	;

qual_use
	: id
		{ $$ = use($1); }
	| type_prefix '$' id
		{ $$ = qual_use($1,$3); }
	;

/* type_prefix is a type that comes before a $ and can be parsed in an
 * expression context without conflicts.
 */
type_prefix
	: qual_use
		{ $$ = type_use($1); }
	;

Pattern : pattern
	| Pattern keyIF expr
		{ $$ = and_pattern($1,condition($3)); }
	| error
		{ $$ = no_pattern(); }
	;

choices
	: Pattern
		{ $$ = list_Patterns($1); }
	| choices ',' Pattern
		{ $$ = xcons_Patterns($1,$3); }
	;

pattern	: simple_pattern
	| pattern keyAND simple_pattern
		{ $$ = and_pattern($1,$3); }
	| pattern '=' simple_pattern
		{ $$ = and_pattern($1,$3); }
	| pattern MATCH type
		{ $$ = match_pattern($1,$3); }
	;

simple_pattern
	: pattern_name simple_pattern_args
		{ $$ = pattern_call($1,$2); }
	| '{' non_empty_pattern_args is_close_brace
		{ $$ = make_sequence_pattern($2,NULL); }
	| '{' non_empty_pattern_args is_close_brace ':' type
		{ $$ = make_sequence_pattern($2,$5); }
	| type_prefix '$' '{' non_empty_pattern_args is_close_brace
		{ $$ = make_sequence_pattern($4,$1); }
	| '{' is_close_brace
		{ $$ = make_sequence_pattern(nil_PatternActuals(),NULL); }
	| '{' is_close_brace ':' type
		{ $$ = make_sequence_pattern(nil_PatternActuals(),$4); }
	| type_prefix '$' '{' is_close_brace
		{ $$ = make_sequence_pattern(nil_PatternActuals(),$1); }
	| '?' opt_id opt_typing
		{ $$ = pattern_var(normal_formal(local_def($2),$3)); }
	| '!' simple_expr
		{ $$ = match_expression($2); }
/* 
	| simple_pattern DOTDOTDOT simple_pattern
		{ $$ = append_pattern($1,$3); }
 */
	| '(' Pattern is_close_paren
		{ $$ = $2; }
	| atomic_pattern
		{ $$ = $1; }
	;

atomic_pattern	
	: pattern_name 
		{ $$ = $1; }
	| pattern_name ':' type
                { $$ = typed_pattern($1,$3); }
	| INTEGER		
		{ $$ = match_expression(integer_const($1)); }
	| REAL		
		{ $$ = match_expression(real_const($1)); }
	| STRING_CONSTANT	
		{ $$ = match_expression(string_const($1)); }
	;

pattern_name
	: qual_use
		{ $$ = pattern_use($1); }
	;

simple_pattern_args
	: '(' is_close_paren
		{ $$ = nil_PatternActuals(); }
	| '(' non_empty_pattern_args is_close_paren
		{ $$ = $2; }
	;

non_empty_pattern_args
	: pattern_arg
		{ $$ = list_PatternActuals($1); }
	| non_empty_pattern_args ',' pattern_arg
		{ $$ = xcons_PatternActuals($1,$3); }
	;

pattern_arg
	: Pattern
		{ $$ = $1; }
	| id COLONEQ Pattern
		{ $$ = pattern_actual($3,value_name($1)); }
	| DOTDOTDOT
		{ $$ = rest_pattern(no_pattern()); }
	| DOTDOTDOT keyAND simple_pattern
		{ $$ = rest_pattern($3); }
	;

stmts	: { $<integer>$ = public; public = FALSE; } stmt_list
		{ public = $<integer>1; $$ = block($2); }
	;

stmt_list
	: 	{ $$ = nil_Declarations(); }
	| stmt_list stmt
		{ $$ = xcons_Declarations($1,$2); }
	;

stmt	: error
		{ $$ = no_stmt(); }
/* 	| ';'	{ $$ = no_stmt(); } */
/*	| expr ';'
		{ $$ = effect($1); } */
	| simple_expr actuals ';'
		{ $$ = multi_call($1,$2,nil_Actuals()); }
	| lhs COLONEQ rhs ';'
		{ $$ = normal_assign($1,$3); }
	| lhs COLONGR rhs ';'
		{ $$ = collect_assign($1,$3); }
	| keyIF expr iskeyTHEN stmts stmt_elsif_part iskeyENDIF is_semi
		{ $$ = if_stmt($2,$4,$5); }
	| keyFOR id opt_typing keyIN expr keyBEGIN stmts iskeyEND is_semi
		{ $$ = for_in_stmt(normal_formal(local_def($2),$3),$5,$7); }
	/* we have to inline opt_direction in order to avoid S/R conflicts */
	| direction id ':' type default ';'
		{ $$ = value_decl(def($2,FALSE,FALSE),$4,$1,$5); }
	| id ':' type default ';'
		{ $$ = value_decl(def($1,FALSE,FALSE),
				  $3,direction(FALSE,FALSE,FALSE),$4); }
	| keyBEGIN stmts iskeyEND is_semi
		{ $$ = block_stmt($2); }
	| keyCASE expr keyBEGIN stmt_match_list stmt_match_default 
          iskeyEND is_semi
		{ $$ = case_stmt($2,$4,$5); }
	| keyFOR simple_expr keyBEGIN stmt_match_list iskeyEND is_semi
		{ $$ = for_stmt($2,$4); }
	| implicit_name polymorphism stmt
		{ $$ = polymorphic($1,$2,block(list_Declarations($3))); }
	| match { yylexerror("missing enclosing 'case' or 'for'");
		  $$ = no_stmt(); }
/* some things from top-level decls */
	| function_decl
	| pat_decl
	| pragma
	;

lhs	: expr
		{ $$ = $1; }
	| expr ',' expr
		{ $$ = append($1,$3); }
	;

rhs	: expr
		{ $$ = $1; }
	| expr ',' expr
		{ $$ = append($1,$3); }
	;

stmt_elsif_part /* right recursion does the association right */
	: 	{ $$ = no_block(); }
	| keyELSE stmts
		{ $$ = $2; }
	| keyELSIF expr keyTHEN stmts stmt_elsif_part
		{ $$ = block(list_Declarations(if_stmt($2,$4,$5))); }
	;

stmt_match_list
	:	{ $$ = nil_Matches(); }
	| stmt_match_list match
		{ $$ = xcons_Matches($1,$2); }
	;

match	: keyMATCH Pattern keyBEGIN stmts iskeyEND is_semi
		{ $$ = matcher($2,$4); }
	;

stmt_match_default
	:	{ $$ = no_block(); }
	| keyELSE stmts
		{ $$ = $2; }
	/* testing new syntax: I'm not sure I like the extra BEGIN */
	| keyELSCASE expr keyBEGIN stmt_match_list stmt_match_default
		{ $$ = block(list_Declarations(case_stmt($2,$4,$5))); }
	;

Expr	: expr
	| Expr ',' expr		{ $$ = append($1,$3); }
	| expr ':' type		{ $$ = typed_value($1,$3); }
	;

expr	: simple_expr
	| expr DOTDOTDOT	{ $$ = repeat($1); }
	| expr DOTDOT expr	{ $$ = binop("..",$1,$3); }
	| expr OPL0 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR0 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL1 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR1 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL2 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR2 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL3 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR3 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL4 expr	{ $$ = sbinop($2,$1,$3); }
	| expr '=' expr		{ $$ = binop("=",$1,$3); }
	| expr OPR4 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL5 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR5 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL6 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR6 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL7 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR7 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL8 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR8 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPL9 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPR9 expr	{ $$ = sbinop($2,$1,$3); }
	| expr OPERATOR expr	{ $$ = sbinop($2,$1,$3); }
	| expr '-' expr		{ $$ = binop("-",$1,$3); }
	| '-' expr %prec UNARY	{ $$ = unop("#-",$2); }
	| '=' expr %prec UNARY	{ $$ = unop("#=",$2); }
	| OPL0 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR0 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL1 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR1 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL2 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR2 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL3 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR3 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL4 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR4 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL5 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR5 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL6 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR6 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL7 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR7 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL8 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR8 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPL9 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPR9 expr %prec UNARY { $$ = sunop($1,$2); }
	| OPERATOR expr %prec UNARY { $$ = sunop($1,$2); }
	| expr keyAND expr	{ $$ = binop("and",$1,$3); }
	| expr keyOR expr	{ $$ = binop("or",$1,$3); }
	| keyNOT expr		{ $$ = unop("not",$2); }
	| expr keyIN expr	{ $$ = binop("in",$1,$3); }
	| expr keyNOT keyIN expr	{ $$ = unop("not",binop("in",$1,$4)); }
/* 	| '#' expr %prec UNARY	{ $$ = unop("#",$2); } */
	| expr keyIF expr
		{ $$ = guarded($1,$3); }
	| expr keyFOR id opt_typing keyIN expr %prec keyFOR
		{ $$ = controlled($1,normal_formal(local_def($3),$4),$6); }
	;


/* only left recursion */
simple_expr
	: atomic_expr
/*	| simple_expr type_parameters
 *		{ $$ = value_inst($1,$2); }
 */
	| simple_expr actuals
		{ $$ = funcall($1,$2); }
	| simple_expr '.' atomic_expr
		{ $$ = funcall($3,list_Actuals($1)); }
	;


/* no recursion to the left or right */
atomic_expr
	: qual_use		{ $$ = value_use($1); }
	| INTEGER		{ $$ = integer_const($1); }
	| REAL			{ $$ = real_const($1); }
	| STRING_CONSTANT	{ $$ = string_const($1); }
  	| CHARACTER_CONSTANT	{ $$ = char_const($1);}
	| '(' Expr is_close_paren		{ $$ = $2; }
	| '{' sequence_actuals is_close_brace	
				{ $$ = make_sequence($2,NULL); }
	| type_prefix '$' '{' sequence_actuals is_close_brace	
				{ $$ = make_sequence($4,$1); }
	| '(' error ')'		{ $$ = no_expr(); }
	;

actuals	: '(' is_close_paren
		{ $$ = nil_Actuals(); }
	| '(' non_empty_actuals is_close_paren
		{ $$ = $2; }
	;

non_empty_actuals
	: actual
		{ $$ = list_Actuals($1); }
	| non_empty_actuals ',' actual
		{ $$ = xcons_Actuals($1,$3); }
	;

actual	: expr
		{ $$ = $1; }
	;

sequence_actuals
	: /* EMPTY */
		{ $$ = nil_Actuals(); }
	| non_empty_sequence_actuals
		{ $$ = $1; }
	;

non_empty_sequence_actuals
	: sequence_actual
		{ $$ = list_Actuals($1); }
	| sequence_actuals ',' sequence_actual
		{ $$ = xcons_Actuals($1,$3); }
	;

sequence_actual
	: expr
/*	| sequence_actual keyIF expr
		{ $$ = guarded($1,$3); }
	| sequence_actual keyFOR id opt_typing keyIN expr
		{ $$ = controlled($1,normal_formal(local_def($3),$4),$6); } */
	;

implicit_name
	: /* EMPTY */	{ $$ = def(make_implicit_name(),TRUE,public); }
	| id		{ $$ = def($1,TRUE,public); }
	;

opt_id	: /* EMPTY */	{ $$ = underscore_symbol; }
	| id		{ $$ = $1; }
	;

id	: IDENTIFIER
		{ $$ = $1; }
	| '(' keyAND is_close_paren
		{ $$ = $2; }
	| '(' keyATTRIBUTE is_close_paren
		{ $$ = $2; }
/*	| '(' keyATTRIBUTION is_close_paren
		{ $$ = $2; } */
	| '(' keyBEGIN is_close_paren
		{ $$ = $2; }
	| '(' keyCASE is_close_paren
		{ $$ = $2; }
	| '(' keyCIRCULAR is_close_paren
		{ $$ = $2; }
	| '(' keyCLASS is_close_paren
		{ $$ = $2; }
	| '(' keyCOLLECTION is_close_paren
		{ $$ = $2; }
	| '(' keyCONSTANT is_close_paren
		{ $$ = $2; }
	| '(' keyCONSTRUCTOR is_close_paren
		{ $$ = $2; }
	| '(' keyELSCASE is_close_paren
		{ $$ = $2; }
	| '(' keyELSE is_close_paren
		{ $$ = $2; }
	| '(' keyELSIF is_close_paren
		{ $$ = $2; }
	| '(' keyEND is_close_paren
		{ $$ = $2; }
	| '(' keyENDIF is_close_paren
		{ $$ = $2; }
	| '(' keyEXTENDS is_close_paren
		{ $$ = $2; }
	| '(' keyFOR is_close_paren
		{ $$ = $2; }
/*	| '(' keyFOREIGN is_close_paren
		{ $$ = $2; } */
	| '(' keyFUNCTION is_close_paren
		{ $$ = $2; }
/*	| '(' keyGRAMMAR is_close_paren
		{ $$ = $2; } */
/*	| '(' keyGROUP is_close_paren
		{ $$ = $2; } */
	| '(' keyIF is_close_paren
		{ $$ = $2; }
	| '(' keyIN is_close_paren
		{ $$ = $2; }
	| '(' keyINFIX is_close_paren
		{ $$ = $2; }
	| '(' keyINFIXL is_close_paren
		{ $$ = $2; }
	| '(' keyINFIXR is_close_paren
		{ $$ = $2; }
	| '(' keyINHERIT is_close_paren
		{ $$ = $2; }
	| '(' keyINPUT is_close_paren
		{ $$ = $2; }
/*	| '(' keyIS is_close_paren
		{ $$ = $2; } */
/*	| '(' keyLANGUAGE is_close_paren
		{ $$ = $2; } */
/*	| '(' keyLOCAL is_close_paren
		{ $$ = $2; } */
	| '(' keyMATCH is_close_paren
		{ $$ = $2; }
	| '(' keyMODULE is_close_paren
		{ $$ = $2; }
	| '(' keyNOT is_close_paren
		{ $$ = $2; }
	| '(' keyON is_close_paren
		{ $$ = $2; }
	| '(' keyOR is_close_paren
		{ $$ = $2; }
	| '(' keyPATTERN is_close_paren
		{ $$ = $2; }
	| '(' keyPHYLUM is_close_paren
		{ $$ = $2; }
	| '(' keyPRAGMA is_close_paren
		{ $$ = $2; }
	| '(' keyPRIVATE is_close_paren
		{ $$ = $2; }
	| '(' keyPROCEDURE is_close_paren
		{ $$ = $2; }
	| '(' keyPUBLIC is_close_paren
		{ $$ = $2; }
	| '(' keyREMOTE is_close_paren
		{ $$ = $2; }
/*	| '(' keyRETURN is_close_paren
		{ $$ = $2; } */
/* 	| '(' keySET is_close_paren
		{ $$ = $2; } */
	| '(' keySIGNATURE is_close_paren
		{ $$ = $2; }
/*	| '(' keySYNTHESIZED is_close_paren
		{ $$ = $2; } */
	| '(' keyTHEN is_close_paren
		{ $$ = $2; }
	| '(' keyTYPE is_close_paren
		{ $$ = $2; }
/*	| '(' keyUSE is_close_paren
		{ $$ = $2; } */
	| '(' keyVAR is_close_paren
		{ $$ = $2; }
	| '(' keyWITH is_close_paren
		{ $$ = $2; }
	| '(' OPL0 is_close_paren
		{ $$ = $2; }
	| '(' OPR0 is_close_paren
		{ $$ = $2; }
	| '(' OPL1 is_close_paren
		{ $$ = $2; }
	| '(' OPR1 is_close_paren
		{ $$ = $2; }
	| '(' OPL2 is_close_paren
		{ $$ = $2; }
	| '(' OPR2 is_close_paren
		{ $$ = $2; }
	| '(' OPL3 is_close_paren
		{ $$ = $2; }
	| '(' OPR3 is_close_paren
		{ $$ = $2; }
	| '(' OPL4 is_close_paren
		{ $$ = $2; }
	| '(' OPR4 is_close_paren
		{ $$ = $2; }
	| '(' OPL5 is_close_paren
		{ $$ = $2; }
	| '(' OPR5 is_close_paren
		{ $$ = $2; }
	| '(' OPL6 is_close_paren
		{ $$ = $2; }
	| '(' OPR6 is_close_paren
		{ $$ = $2; }
	| '(' OPL7 is_close_paren
		{ $$ = $2; }
	| '(' OPR7 is_close_paren
		{ $$ = $2; }
	| '(' OPL8 is_close_paren
		{ $$ = $2; }
	| '(' OPR8 is_close_paren
		{ $$ = $2; }
	| '(' OPL9 is_close_paren
		{ $$ = $2; }
	| '(' OPR9 is_close_paren
		{ $$ = $2; }
	| '(' '=' is_close_paren
		{ $$ = intern_symbol("="); }
	| '(' '-' is_close_paren
		{ $$ = intern_symbol("-"); }
	| '(' OPERATOR is_close_paren
		{ $$ = $2; }
	| '{' '.' is_close_brace
		{ $$ = seq_symbol; }
/*	| '(' is_close_paren
 *		{ $$ = intern_symbol(""); }
 */
	| '(' '.' is_close_paren 
		{ $$ = intern_symbol("."); }
	| '(' DOTDOT is_close_paren
		{ $$ = intern_symbol(".."); }
	| '(' DOTDOTDOT is_close_paren 
		{ $$ = intern_symbol("..."); }
	;

iskeyEND
	: keyEND {}
	| keyENDIF { yylexerror("'endif' used for 'end'"); }
	;

iskeyENDIF
	: keyENDIF {}
	| keyEND { yylexerror("'end' used for 'endif'"); }
	;

iskeyTHEN
	: keyTHEN {}
	| keyBEGIN { yylexerror("'begin' used for 'then'"); }

is_close_paren
	: ')' {}
	| ']' { yylexerror("']' used for ')'"); }
	| '}' { yylexerror("'}' used for ')'"); }
	;

is_close_bracket
	: ']' {}
	| ')' { yylexerror("')' used for ']'"); }
	| '}' { yylexerror("'}' used for ']'"); }
	;

is_close_brace
	: '}' {}
	| ')' { yylexerror("')' used for '}'"); }
	| ']' { yylexerror("']' used for '}'"); }
	;

is_semi
	: ';' {}
	/* the following rule causes several shift/reduce conflict */
	| /* empty */ { yyerror("missing ';'"); }
	;

%%

void yylexerror(char *s) {
  int saved = yytoken;
  yytoken = LEX_ERROR;
  yyerror(s);
  yytoken = saved;
}

void yyerror(char *s) {
  extern int aps_parse_error;
  aps_parse_error = 1;
  fprintf(stderr,"%s:%d: %s",yyfilename,yylineno,s);
  if (yytoken == 0) {
    fprintf(stderr," at end of file\n");
  } else if (yytoken == LEX_ERROR) { /* lex error, do nothing */
    fprintf(stderr,"\n");
  } else if (yytoken < 32) { /* why is this happening? */
    fprintf(stderr," before token 0x%d\n",yytoken);
  } else if (yytoken < 128) {
    fprintf(stderr," before token \"%c\"\n",yytoken);
  } else {
    fprintf(stderr," before ");
    switch (yytoken) {
    case DOTDOTDOT:
      fprintf(stderr,"token \"...\"");
      break;
    case DOTDOT:
      fprintf(stderr,"token \"..\"");
      break;
    case DOUBLECOLON:
      fprintf(stderr,"token \"::\"");
      break;
    case TRIPLECOLON:
      fprintf(stderr,"token \":::\"");
      break;
    case COLONEQ:
      fprintf(stderr,"token \":=\"");
      break;

    case IDENTIFIER:
      fprintf(stderr,"identifier \"%s\"",symbol_name(yylval.symbol));
      break;
    case OPERATOR:
    case OPL0: case OPR0:
    case OPL1: case OPR1:
    case OPL2: case OPR2:
    case OPL3: case OPR3:
    case OPL4: case OPR4:
    case OPL5: case OPR5:
    case OPL6: case OPR6:
    case OPL7: case OPR7:
    case OPL8: case OPR8:
    case OPL9: case OPR9:
      fprintf(stderr,"operator \"%s\"",symbol_name(yylval.symbol));
      break;
    case INTEGER:
    case REAL:
    case CHARACTER_CONSTANT:
    case STRING_CONSTANT:
      print_string(stderr,yylval.string);
      break;
      
    default:
      fprintf(stderr,"keyword \"%s\"",symbol_name(yylval.symbol));
      break;
    }
    fprintf(stderr,"\n");
  }
}
