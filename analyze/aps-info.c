/* APS-INFO
 * Access slots inside APS nodes.
 * This file must be recompiled each time
 * we need a new slot.
 */
#include <stdio.h>
#include "aps-ag.h"

struct info {
  void *parent;
  union {
    struct Program_info program_info;
    struct Unit_info unit_info;
    struct Use_info use_info;
    struct Declaration_info declaration_info;
    struct Match_info match_info;
    struct Expression_info expression_info;
    struct Pattern_info pattern_info;
    struct Type_info type_info;
    struct Signature_info signature_info;
    struct Def_info def_info;
  } var;
};

int info_size=sizeof(struct info);

extern struct info *tnode_info(void *);

void *tnode_parent(void *tnode) {
  return tnode_info(tnode)->parent;
}

static void *set_parent(void *parent, void *tnode) {
#ifdef UNDEF
  if (ABSTRACT_APS_tnode_phylum(tnode) == KEYDeclaration) {
    switch (Declaration_KEY((Declaration)tnode)) {
    case KEYdeclaration:
      printf("Setting parent for %s\n",
	     symbol_name(def_name(declaration_def((Declaration)tnode))));
      break;
    }
  }
#endif
  tnode_info(tnode)->parent = parent;
  return tnode;
}
void set_tnode_parent(Program p) {
#ifdef UNDEF
  printf("Setting parents for %s\n",program_name(p));
#endif
  traverse_Program(set_parent,/*anything non null*/p,p);
  tnode_info(p)->parent = NULL; /* fix parent of root */
}

struct Program_info *Program_info(Program _node) {
  return &(tnode_info(_node)->var.program_info);
}

struct Unit_info *Unit_info(Unit _node) {
  return &(tnode_info(_node)->var.unit_info);
}

struct Use_info *Use_info(Use _node) {
  return &(tnode_info(_node)->var.use_info);
}

struct Declaration_info *Declaration_info(Declaration _node) {
  return &(tnode_info(_node)->var.declaration_info);
}

struct Match_info *Match_info(Match _node) {
  return &(tnode_info(_node)->var.match_info);
}

struct Expression_info *Expression_info(Expression _node) {
  return &(tnode_info(_node)->var.expression_info);
}

struct Pattern_info *Pattern_info(Pattern _node) {
  return &(tnode_info(_node)->var.pattern_info);
}

struct Type_info *Type_info(Type _node) {
  return &(tnode_info(_node)->var.type_info);
}

struct Signature_info *Signature_info(Signature _node) {
  return &(tnode_info(_node)->var.signature_info);
}

struct Def_info *Def_info(Def _node) {
  return &(tnode_info(_node)->var.def_info);
}


/**
 * Returns the count of number of actuals
 */
int compute_type_contour_size(TypeActuals type_actuals, Declarations type_formals) {
  int count = 0;
  if (type_actuals != NULL) {
    Type ta;
    for (ta = first_TypeActual(type_actuals); ta != NULL; ta = TYPE_NEXT(ta)) {
      count++;
    }
  } else if (type_formals != NULL) {
    Declaration tf;
    for (tf = first_Declaration(type_formals); tf != NULL; tf = DECL_NEXT(tf)) {
      count++;
    }
  }

  return count;
}
