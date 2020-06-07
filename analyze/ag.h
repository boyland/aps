/* Attribute grammar definitions.
 */
#ifndef AG_H
#define AG_H

typedef struct symbol SYMBOL;

typedef struct nonterminal NT;
typedef struct terminal TERM;
typedef struct keyword KEYWORD;

typedef struct function FUNCTION;

typedef struct value VALUE;
typedef struct attribute ATTR;
typedef struct local LOCAL;
typedef struct field FIELD;
typedef struct parameter PARAM;
typedef struct result RESULT;

typedef struct production PROD;

typedef struct scope SCOPE;
typedef struct rule RULE;
typedef struct condition COND;

typedef struct expression EXPR;

struct symbol {
  char *name;
  short kind;
  short flags;
};

struct nonterminal {
  SYMBOL super;
#define GRAM_NT 1
  PROD *prods[];
  ATTR *attrs[];
  void *io_graph;
};

struct terminal {
  SYMBOL super;
#define GRAM_TERM 2
};

struct keyword {
  SYMBOL super;
#define GRAM_KEYWORD 3
};

struct function {
  SYMBOL super;
#define SYM_FUNCTION 4
  PARAM *params[];
  RESULT *result;
  SCOPE *body;
  void *ad_graph;
};

struct primitive {
  SYMBOL super;
#define SYM_PRIMITIVE 5
};

struct value {
  SYMBOL super;
#define VALUE_ATTR 6
#define VALUE_LOCAL 7
#define VALUE_FIELD 8
#define VALUE_PARAM 9
#define VALUE_RESULT 10
  void *fibers;
};
  
struct attribute {
  VALUE super;
#define AT_SYN 16
};

struct local {
  VALUE super;
  SCOPE *local_scope;
#define LOC_OBJ 32
};

struct field {
  VALUE super;
#define FLD_COL 64
};

struct parameter {
  VALUE super;
};

struct result {
  VALUE super;
};

struct production {
  NT *nt;
  SYMBOL *symbols[];
  SCOPE *body;
  void *ad_graph;
};

struct scope {
  COND *cond;
  LOCAL *locals[];
  RULE *rules[];
};

struct rule {
  SCOPE *rule_scope;
  int rule_kind;
#define RULE_ASSIGN 13
#define RULE_COLLECT 14
#define RULE_IF 15
};

struct assign_rule {
  RULE super;
  EXPR *lhs, *rhs;
};

struct if_rule {
  RULE super;
  EXPR *cond;
  SCOPE *then_block, *else_block;
  int index; /* used for sorting */
};

struct cond {
  struct if_rule *condition;
  BOOL branch; /* TRUE or FALSE */
  COND *next;
};

struct expr {
  int expr_kind;
#define EXPR_USE 19
#define EXPR_REF 20
#define EXPR_FREF 21
#define EXPR_OBJ 22
#define EXPR_ID 23
#define EXPR_CALL 24
};

struct use_expr {
  EXPR super;
  SYMBOL *used;
}

struct ref_expr {
  EXPR super;
}

#endif
