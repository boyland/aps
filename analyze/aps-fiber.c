#include <stdio.h>
#include <jbb.h>

#include "alloc.h"
#include "aps-ag.h"

#define ADD_FIBER 1
#define ALL_FIBERSETS 2
#define PUSH_FIBER 4
int fiber_debug = 0;

Def reverse_def(Def d) {
  char name[80];
  Symbol sym;
  sprintf(name,"%s!",symbol_name(def_name(d)));
  sym = intern_symbol(name);
  return def(sym,def_is_constant(d),def_is_public(d));
}

Declaration field_base(Declaration field) {
  if (FIELD_DECL_IS_REVERSE(field)) {
    return reverse_field(field);
  } else {
    return field;
  }
}

Declaration reverse_field(Declaration field) {
  return DUAL_DECL(field);
}

static struct fiber base_fiber_struct = {NULL,NULL,NULL};
FIBER base_fiber = &base_fiber_struct;
  
FIBER lengthen_fiber(Declaration field, FIBER xshorter) {
  FIBER shorter = (xshorter == NULL) ? base_fiber : xshorter;
  FIBER longer = (FIBER)assoc(field,shorter->longer);
  if (field == NULL) return shorter; /* makes easier */
  if (longer == NULL) {
    FIBER tmp;
    BOOL error = FALSE;
    /* check special instructions for the field */
    Declaration base = field_base(field);
    if (base == 0) {
      fatal_error("base of field %s[0%o] is NULL!\n",
		  decl_name(field),Declaration_info(field)->decl_flags);
    }
    if (FIELD_DECL_IS_UNTRACKED(base)) return shorter;
    if (FIELD_DECL_IS_CYCLIC(base) &&
	shorter->field == field) return shorter;
    /* check whether the field already occurs in the fiber */
    for (tmp=shorter; tmp != NULL; tmp=tmp->shorter) {
      if (tmp->field == field) { /* error: fiber cycle! */
	error = TRUE; /* allocate new fiber and then stop the show */
	break;
      }
    }
    longer = (FIBER)HALLOC(sizeof(struct fiber));
    longer->field = field;
    longer->shorter = shorter;
    longer->longer = NULL;
    if (error) {
      fprintf(stderr,"Cyclic fiber produced: ");
      print_fiber(longer,stderr);
      putc('\n',stderr);
      fatal_error("stop");
    }
    shorter->longer = acons(field,longer,shorter->longer);
  }
  return longer;
}

enum shortening { no_shorter, one_shorter, one_same, two_shorter };

enum shortening shorten(Declaration field, FIBER longer) {
  /* check special instructions for the field */
  Declaration base = field_base(field);
  if (FIELD_DECL_IS_UNTRACKED(base)) return one_same;
  if (FIELD_DECL_IS_CYCLIC(base)) {
    if (longer->field == field) return two_shorter;
    /* otherwise fall through to end */
  } else if (longer->field == field) {
    return one_shorter;
  }
  return no_shorter;
}

int fiberset_length(FIBERSET fs) {
  int size=0;
  while (fs != NULL) {
    ++size;
    fs=fs->rest;
  }
  return size;
}

static FIBERSET fiber_worklist_head;
static FIBERSET fiber_worklist_tail;

Declaration field_ref_p(Expression);
Declaration attr_ref_p(Expression);
Declaration local_call_p(Expression);

Expression field_ref_object(Expression); /* also good for attr_ref's */

Declaration object_decl_p(Declaration);

void add_to_fiberset(FIBER fiber, void *tnode, int fstype, FIBERSET *fsp) {
  if (fiber->field == NULL) {
    /* base fiber! */
    if (fstype & FIBERSET_TYPE_REVERSE)
      fatal_error("%d:base fiber got into reverse set!",
		  tnode_line_number(tnode));
    return;
  }
  while (*fsp != NULL) {
    if ((*fsp)->fiber == fiber) return; /* already there */
    fsp = &(*fsp)->rest;
  }
  { FIBERSET fs = (FIBERSET)HALLOC(sizeof(struct fiberset));
    fs->rest = NULL;
    fs->fiber = fiber;
    fs->tnode = tnode;
    fs->fiberset_type = fstype;
    fs->next_in_fiber_worklist = NULL;
    if (fiber_worklist_head == NULL) {
      fiber_worklist_head = fs;
    } else {
      fiber_worklist_tail->next_in_fiber_worklist = fs;
    }
    fiber_worklist_tail = fs;
    (*fsp) = fs;
    /* DEBUG */
    if (fiber_debug & ADD_FIBER) {
      printf("%d: Added ",tnode_line_number(tnode));
      print_fiberset_entry(fs,stdout);
      printf("\n");
    }
  }
}

FIBERSET get_next_fiber() {
  FIBERSET next = fiber_worklist_head;
  if (next != NULL) {
    fiber_worklist_head = next->next_in_fiber_worklist;
    if (next == fiber_worklist_tail) fiber_worklist_tail = NULL;
  }
  return next;
}

int member_fiberset(FIBER f, FIBERSET fs) {
  while (fs != NULL) {
    if (f == fs->fiber) return TRUE;
    fs = fs->rest;
  }
  return FALSE;
}

FIBERSET intersect_fiberset(FIBERSET fs1, FIBERSET fs2) {
  FIBERSET fs0 = NULL;
  while (fs1 != NULL) {
    if (member_fiberset(fs1->fiber,fs2)) {
      FIBERSET fs = (FIBERSET)HALLOC(sizeof(struct fiberset));
      fs->rest = fs0;
      fs->fiber = fs1->fiber;
      fs->tnode = fs1->tnode;
      fs->fiberset_type = fs1->fiberset_type;
      fs->next_in_fiber_worklist = NULL;
      fs0 = fs;
    }
    fs1 = fs1->rest;
  }
  return fs0;
}


/*** NODE PREDICATES ETC ***/

/* A field reference is a attribute call with a local attribute
 * marked as a field decl.
 */
Declaration field_ref_p(Expression expr) {
  switch (Expression_KEY(expr)) {
  case KEYfuncall:
    { Expression func = funcall_f(expr);
      switch (Expression_KEY(func)) {
      case KEYvalue_use:
	{ Declaration attr = USE_DECL(value_use_use(func));
	  if (attr == NULL) aps_error(func,"unbound function");
	  else if (DECL_IS_LOCAL(attr) && FIELD_DECL_P(attr)) return attr;
	}
      }
    }
  }
  return NULL;
}

Declaration attr_ref_p(Expression expr) {
  switch (Expression_KEY(expr)) {
  case KEYfuncall:
    { Expression func = funcall_f(expr);
      switch (Expression_KEY(func)) {
      case KEYvalue_use:
	{ Declaration attr = USE_DECL(value_use_use(func));
	  if (attr == NULL) aps_error(func,"unbound function");
	  else if (DECL_IS_LOCAL(attr) && !FIELD_DECL_P(attr)) {
	    switch (Declaration_KEY(attr)) {
	    case KEYattribute_decl:
	      return attr;
	    }
	  }
	}
      }
    }
  }
  return NULL;
}

Declaration constructor_call_p(Expression expr) {
  switch (Expression_KEY(expr)) {
  case KEYfuncall:
    { Expression func = funcall_f(expr);
      switch (Expression_KEY(func)) {
      case KEYvalue_use:
	{ Declaration decl = USE_DECL(value_use_use(func));
	  if (decl == NULL) aps_error(func,"unbound function");
	  else if (DECL_IS_LOCAL(decl) && !FIELD_DECL_P(decl)) {
	    switch (Declaration_KEY(decl)) {
	    case KEYconstructor_decl:
	      return decl;
	    }
	  }
	}
      }
    }
  }
  return NULL;
}

Expression field_ref_object(Expression expr) {
  /* assume a field_ref */
  return first_Actual(funcall_actuals(expr));
}

Declaration local_call_p(Expression expr) {
  switch (Expression_KEY(expr)) {
  case KEYfuncall:
    { Expression func = funcall_f(expr);
      switch (Expression_KEY(func)) {
      case KEYvalue_use:
	{ Declaration decl = USE_DECL(value_use_use(func));
	  if (decl != NULL && DECL_IS_LOCAL(decl)) {
	    switch (Declaration_KEY(decl)) {
	    case KEYprocedure_decl:
	    case KEYfunction_decl:
	      return decl;
	    }
	  }
	}
      }
    }
  }
  return NULL;
}

Declaration object_decl_p(Declaration decl) {
  switch (Declaration_KEY(decl)) {
  default: return NULL;
  case KEYvalue_decl:
    { Default def = value_decl_default(decl);
      switch (Default_KEY(def)) {
      default: return NULL;
      case KEYsimple:
	{ Expression expr = simple_value(def);
	  return constructor_call_p(expr);
	}
	break;
      }
    }
  }
  return NULL;
}

Declaration constructor_decl_phylum(Declaration decl) {
  Type ft = constructor_decl_type(decl);
  Declaration rd = first_Declaration(function_type_return_values(ft));
  Type rt = value_decl_type(rd);
  Use u = type_use_use(rt);
  return USE_DECL(u);
}

Declaration some_function_decl_result(Declaration func) {
  return first_Declaration
    (function_type_return_values(some_function_decl_type(func)));
}

Declaration result_decl_p(Declaration rdecl) {
  void *node = tnode_parent(rdecl);
  if (Declaration_KEY(rdecl) != KEYvalue_decl) return NULL;
  while (ABSTRACT_APS_tnode_phylum(node) == KEYDeclarations)
    node = tnode_parent(node);
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYType:
    switch (Type_KEY((Type)node)) {
    case KEYfunction_type:
      return (Declaration)tnode_parent(node);
    }
    break;
  }
  return NULL;
}

Declaration formal_function_decl(Declaration formal) {
  void *p = tnode_parent(formal);
  while (ABSTRACT_APS_tnode_phylum(p) == KEYDeclarations) {
    p = tnode_parent(p);
  }
  if (ABSTRACT_APS_tnode_phylum(p) != KEYType)
    fatal_error("%d:cannot find function_type",tnode_line_number(p));
  p = tnode_parent(p);
  if (ABSTRACT_APS_tnode_phylum(p) != KEYDeclaration)
    fatal_error("%d:cannot find function_decl",tnode_line_number(p));
  { Declaration decl = (Declaration)p;
    switch (Declaration_KEY(decl)) {
    default: fatal_error("%d: not a function decl",tnode_line_number(decl));
    case KEYfunction_decl:
    case KEYprocedure_decl:
    case KEYconstructor_decl:
    case KEYpattern_decl:
      return decl;
    }
  }
}

int function_formal_index(Declaration func, Declaration formal) {
  int i=0;
  Declaration f;
  Declarations formals = function_type_formals(some_function_decl_type(func));
  for (f = first_Declaration(formals);
       f != NULL && f != formal;
       f = DECL_NEXT(f), ++i) {
    /* nothing */
  }
  if (f == NULL) fatal_error("%d: cannot find formal",tnode_line_number(func));
  return i;
}

Expression nth_Actual(Actuals actuals, int n) {
  Expression expr;
  for (expr = first_Actual(actuals);
       expr != NULL && n > 0;
       expr = Expression_info(expr)->next_expr, --n) {
    /* nothing */
  }
  if (expr == NULL) 
    fatal_error("%d: cannot find actual",tnode_line_number(actuals));
  return expr;
}

Expression function_formal_actual(Declaration func,
				  Declaration formal,
				  Expression call) {
  Expression expr;
  Declaration f;
  Declarations formals = function_type_formals(some_function_decl_type(func));
  Actuals actuals = funcall_actuals(call);
  if (result_decl_p(formal)) return call;
  for (expr = first_Actual(actuals), f = first_Declaration(formals);
       expr != NULL && f != NULL && formal != f;
       expr = Expression_info(expr)->next_expr, f = DECL_NEXT(f)) {
    /* nothing */
  }
  if (expr == NULL || f == NULL)
    fatal_error("%d: cannot find actual",tnode_line_number(actuals));
  return expr;
}

Declaration function_actual_formal(Declaration func,
				   Expression actual,
				   Expression call) {
  Expression expr;
  Declaration f;
  Declarations formals = function_type_formals(some_function_decl_type(func));
  Actuals actuals = funcall_actuals(call);
  for (expr = first_Actual(actuals), f = first_Declaration(formals);
       expr != NULL && f != NULL && expr != actual;
       expr = Expression_info(expr)->next_expr, f = DECL_NEXT(f)) {
    /* nothing */
  }
  if (expr == NULL || f == NULL)
    fatal_error("%d: cannot find formal",tnode_line_number(actuals));
  return f;
}

Declaration phylum_shared_info_attribute(Declaration phylum, STATE *s) {
  Declaration instance =
    POLYMORPHIC_INSTANCES(MODULE_SHARED_INFO_POLYMORPHIC(s->module));
  if (phylum == NULL) {
    fatal_error("cannot get shared_info for null phylum");
  }
  while (instance != NULL) {
    TypeFormals tfs = polymorphic_type_formals(instance);
    Use u = type_use_use(type_renaming_old(list_Declarations_elem(tfs)));
    /* printf("Looking at shared info for %s\n",symbol_name(use_name(u))); */
    if (USE_DECL(u) == phylum) {
      return list_Declarations_elem(block_body(polymorphic_body(instance)));
    }
    instance = DECL_NEXT(instance);
  }
  fatal_error("could not find shared info for %s",decl_name(phylum));
  return NULL;
}

/** Return the declaration of the node at the root of the top-level-match
 * of function/procedure decl enclosing the given node.  If none, return NULL.
 */
Declaration responsible_node_declaration(void *node) {
  while (node != NULL) {
    switch (ABSTRACT_APS_tnode_phylum(node)) {
    case KEYDeclaration:
      { Declaration decl = (Declaration)node;
	switch (Declaration_KEY(decl)) {
	case KEYtop_level_match:
	  { Pattern pat=matcher_pat(top_level_match_m((Declaration)node));
	    switch (Pattern_KEY(pat)) {
	    case KEYand_pattern:
	      pat = and_pattern_p1(pat);
	      switch (Pattern_KEY(pat)) {
	      case KEYpattern_var:
		return pattern_var_formal(pat);
		break;
	      }
	      break;
	    }
	    fatal_error("%d: could not find responsible node in pattern",
			tnode_line_number(pat));
	    return NULL;
	  }
	  break;
	case KEYsome_function_decl:
	  return node;
	}
      }
    }
    node = tnode_parent(node);
  }
  return NULL;
}

/** Return true if a use of a shared value. */
Declaration shared_use_p(Expression expr) {
  /* if we're inside the shared area, the use is considered a normal use
   *! Needs to be modified for functions/procedures.
   */
  if (responsible_node_declaration(expr) == NULL) return NULL;
  switch (Expression_KEY(expr)) {
  case KEYvalue_use:
    { Declaration decl = USE_DECL(value_use_use(expr));
      if (decl == NULL || !DECL_IS_SHARED(decl)) return NULL;
      switch (Declaration_KEY(decl)) {
      case KEYvalue_decl:
	return decl;
      }
    }
    break;
  }
  return NULL;
}

/** Return the phylum of a pattern_var node.
 */
Declaration node_decl_phylum(Declaration decl) {
  switch (Declaration_KEY(decl)) {
  case KEYsome_function_decl:
    /* special case for function decls */
    return decl;
  case KEYnormal_formal:
    {
      Type ty = infer_formal_type(decl);
      switch (Type_KEY(ty)) {
      case KEYtype_use:
	{
	  Declaration phy = USE_DECL(type_use_use(ty));
	  if (phy != NULL) {
	    if (Declaration_KEY(phy) == KEYphylum_decl) return phy;
	    else return NULL;
	  }
	}
	break;
      default:
	break;
      }
    }
    break;
  default:
    break;
  }
  fatal_error("%d: cannot find phylum for %s",
	      tnode_line_number(decl),decl_name(decl));
  return NULL;
}

/** Return the declaration of the shared_info attribute
 * of the node responsible here.
 */
Declaration responsible_node_shared_info(void *node, STATE *s) {
  Declaration decl = responsible_node_declaration(node);
  Declaration phy = decl ? node_decl_phylum(decl) : 0;
  if (decl == NULL) return NULL;
  if (phy == NULL)
    fatal_error("%d: cannot find phylum for responsible node %s",
		tnode_line_number(node),decl_name(decl));
  return phylum_shared_info_attribute(phy,s);
}

Declaration formal_in_case_p(Declaration formal) {
  if (Declaration_KEY(formal) != KEYnormal_formal) return NULL;
  if (ABSTRACT_APS_tnode_phylum(tnode_parent(formal)) == KEYPattern) {
    /* move up to the case statement */
    void *parent = tnode_parent(formal);
    while (ABSTRACT_APS_tnode_phylum(parent) == KEYPattern ||
	   ABSTRACT_APS_tnode_phylum(parent) == KEYPatterns ||
	   ABSTRACT_APS_tnode_phylum(parent) == KEYPatternActuals) {
      parent = tnode_parent(parent);
    }
    if (ABSTRACT_APS_tnode_phylum(parent) != KEYMatch)
      fatal_error("%d: not a Match",tnode_line_number(parent));
    parent = tnode_parent(parent);
    while (ABSTRACT_APS_tnode_phylum(parent) == KEYMatches) {
      parent = tnode_parent(parent);
    }
    if (ABSTRACT_APS_tnode_phylum(parent) != KEYDeclaration) {
      fatal_error("%d: not a Declaration",tnode_line_number(parent));
    }
    switch (Declaration_KEY((Declaration)parent)) {
    default:
      fatal_error("%d: not a case",tnode_line_number(parent));
      break;
    case KEYcase_stmt:
      return (Declaration)parent;
    case KEYtop_level_match:
      return NULL;
    }
  } else {
    return NULL;
  }
}


/*** INITIALIZATION ***/

Declaration attribute_decl_phylum(Declaration attr) {
  Declaration f =
    first_Declaration(function_type_formals(attribute_decl_type(attr)));
  return USE_DECL(type_use_use(infer_formal_type(f)));
}

/* A field in an attribute on a phylum not from the extension.
 * If the phylum is not in the list for the state, we assume
 * it is local.
 */
void init_field_decls(Declaration module, STATE *s) {
  Declaration decl, shared_info_phylum, shared_info_polymorphic;
  int i;
  extern int aps_yylineno;

  /* set up the shared info attributes */
  aps_yylineno = tnode_line_number(module);
  MODULE_SHARED_INFO_PHYLUM(module) =
    shared_info_phylum =
      phylum_decl(def(intern_symbol("__GlobalInfo"),TRUE,TRUE),
		  no_sig(),no_type());

  { Declaration tf = type_formal(def(intern_symbol("P"),TRUE,FALSE),
				 no_sig()); /* actually a fixed_sig */
    Use u = use(find_symbol("P"));
    Declaration formal = normal_formal(def(intern_symbol("_"),TRUE,FALSE),
				       type_use(u));
    Use gipu = use(find_symbol("__GlobalInfo"));
    Declaration rd = value_decl(def(intern_symbol("_"),TRUE,FALSE),
				remote_type(type_use(gipu)),
				direction(FALSE,FALSE,FALSE),
				no_default());
    Declaration attr =
      attribute_decl(def(intern_symbol("shared_info"),FALSE,TRUE),
		     function_type(list_Formals(formal),
				   list_Declarations(rd)),
		     direction(FALSE,FALSE,FALSE),
		     no_default());
    Declaration shared_info_polymorphic;
    USE_DECL(u) = tf;
    USE_DECL(gipu) = shared_info_phylum;
    MODULE_SHARED_INFO_POLYMORPHIC(module) =
      shared_info_polymorphic =
	polymorphic(def(intern_symbol("G"),TRUE,TRUE),
		    list_Declarations(tf),
		    block(list_Declarations(attr)));
    for (i=0; i < s->phyla.length; ++i) {
      Declaration p = s->phyla.array[i];
      /* hand instantiate the shared info */
      Use pu = use(def_name(declaration_def(p)));
      Declaration tr = type_renaming(def(intern_symbol("P"),TRUE,FALSE),
				     type_use(pu));
      /* shortcut use, I don't think my code handles renamings well */
      Use u = use(def_name(declaration_def(p)));
      Declaration formal = normal_formal(def(intern_symbol("_"),TRUE,FALSE),
				       type_use(u));
      Use gipu = use(find_symbol("__GlobalInfo"));
      Declaration rd = value_decl(def(intern_symbol("_"),TRUE,FALSE),
				  remote_type(type_use(gipu)),
				  direction(FALSE,FALSE,FALSE),
				  no_default());
      char name[80];
      Declaration attr;
      Declaration shared_info_instance;
      Symbol new_sym;

      /* printf("Creating shared info for %s\n",decl_name(p)); */
      
      USE_DECL(pu) = p;
      USE_DECL(u) = p;
      USE_DECL(gipu) = shared_info_phylum;
      sprintf(name,"G[%s]'shared_info",decl_name(p));
      new_sym = intern_symbol(name);
      set_code_name(new_sym,make_string("__shared_info"));
      attr = 
	attribute_decl(def(new_sym,FALSE,TRUE),
		       function_type(list_Formals(formal),
				     list_Declarations(rd)),
		       direction(FALSE,FALSE,FALSE),
		       no_default());
      Declaration_info(attr)->decl_flags |=
	SHARED_INFO_FLAG | SHARED_DECL_FLAG | ATTR_DECL_INH_FLAG;

      shared_info_instance =
	polymorphic(def(intern_symbol("G"),TRUE,TRUE),
		    list_Declarations(tr),
		    block(list_Declarations(attr)));
      DECL_NEXT(shared_info_instance) =
	POLYMORPHIC_INSTANCES(shared_info_polymorphic);
      POLYMORPHIC_INSTANCES(shared_info_polymorphic) = shared_info_instance;
    }
  }
  

  for (decl = first_Declaration(block_body(module_decl_contents(module)));
       decl != NULL;
       decl = DECL_NEXT(decl)) {
    Declaration_info(decl)->decl_flags |= SHARED_DECL_FLAG;
    switch (Declaration_KEY(decl)) {
    case KEYvalue_decl:
      { Declaration reversed;
	if (fiber_debug & FIBER_INTRO) {
	  if (direction_is_collection(value_decl_direction(decl))) {
	    printf("%s is a shared collection\n",decl_name(decl));
	  } else {
	    printf("%s is a shared value\n",decl_name(decl));
	  }
	}
	NEXT_FIELD(decl) = NEXT_FIELD(shared_info_phylum);
	NEXT_FIELD(shared_info_phylum) = decl;
	aps_yylineno = tnode_line_number(decl);
	reversed =
	      attribute_decl(reverse_def(value_decl_def(decl)),
			     no_type(),
			     copy_Direction(value_decl_direction(decl)),
			     no_default());
	Declaration_info(reversed)->decl_flags =
	      Declaration_info(decl)->decl_flags | FIELD_DECL_REVERSE_FLAG;
	DUAL_DECL(decl) = reversed;
	DUAL_DECL(reversed) = decl;
      }
      break;
    case KEYattribute_decl:
      { Declaration phylum = attribute_decl_phylum(decl);
	for (i=0; i < s->phyla.length; ++i) {
	  if (phylum == s->phyla.array[i]) break;
	}
	if (i == s->phyla.length) {
	  if (fiber_debug & FIBER_INTRO)
	    printf("%s is a field (not an attribute)\n",decl_name(decl));
	  Declaration_info(decl)->decl_flags |= FIELD_DECL_FLAG;
	  NEXT_FIELD(decl) = NEXT_FIELD(phylum);
	  NEXT_FIELD(phylum) = decl;
	  { Declaration reversed;
	    aps_yylineno = tnode_line_number(decl);
	    reversed =
	      attribute_decl(reverse_def(attribute_decl_def(decl)),
			     no_type(),
			     copy_Direction(attribute_decl_direction(decl)),
			     no_default());
	    Declaration_info(reversed)->decl_flags =
	      Declaration_info(decl)->decl_flags | FIELD_DECL_REVERSE_FLAG;
	    DUAL_DECL(decl) = reversed;
	    DUAL_DECL(reversed) = decl;
	  }
	}
      }
      break;
    }
  }
}

static void *lhs_key = &lhs_key;

/* for now, lhs and rhs cover everything */
void *init_rhs_lhs(void *key, void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      if (key == lhs_key) {
	Expression_info(expr)->expr_flags |= EXPR_LHS_FLAG;
	/*?? 9/20/97: why not say that the rest is rhs ? */
	return node;
      } else {
	Expression_info(expr)->expr_flags |= EXPR_RHS_FLAG;
      }
    }
    break;
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
      case KEYpragma_call:
	return NULL;
      case KEYassign:
	traverse_Expression(init_rhs_lhs,lhs_key,assign_lhs(decl));
	traverse_Expression(init_rhs_lhs,key,assign_rhs(decl));
	return NULL;
      }
    }
    break;
  }
  return key;
}

/* ensure all Declarations have clean fiber sets */
void *preinitialize_fibersets(void *statep, void *node)
{
  FIBERSETS *fss = 0;
  int i;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    fss = &fibersets_for((Declaration)node);
    break;
  case KEYExpression:
    fss = &expr_fibersets_for((Expression)node);
    break;
  }
  if (fss)
    for (i=0; i < 6; ++i)
      fss->set[i] = 0;
  return statep;
}

/* find all places where fibers spring into existence */
void *initialize_fibersets(void *statep, void *node) {
  STATE *state = (STATE *)statep;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      Declaration cdecl;
      switch (Declaration_KEY(decl)) {
      default: break;
      case KEYvalue_decl:
	if (DECL_IS_SHARED(decl)) {
	  Declaration sattr =
	    phylum_shared_info_attribute(state->start_phylum,state);
	  if (fiber_debug & FIBER_INTRO)
	    printf("%d: found shared decl %s\n",tnode_line_number(decl),
		   decl_name(decl));
	  add_to_fiberset(lengthen_fiber(decl,base_fiber),
			  sattr,FIBERSET_NORMAL_PROVIDE,
			  &fiberset_for(sattr,FIBERSET_NORMAL_PROVIDE));
	  if (direction_is_collection(value_decl_direction(decl))) {
	    /* we want the value.
	     *? (why is this necessary?)
	     */
	    /*? Why not backward_base_fiber? */
	    add_to_fiberset(lengthen_fiber(decl,base_fiber),
			    sattr,FIBERSET_REVERSE_REQUIRE,
			    &fiberset_for(sattr,FIBERSET_REVERSE_REQUIRE));
	  }
	}
	if ((cdecl = object_decl_p(decl)) != NULL) {
	  Declaration pdecl = constructor_decl_phylum(cdecl);
	  Declaration fdecl;
	  if (fiber_debug & FIBER_INTRO)
	    printf("%d: found object decl %s\n",tnode_line_number(decl),
		   decl_name(decl));
	  for (fdecl = NEXT_FIELD(pdecl);
	       fdecl != NULL;
	       fdecl = NEXT_FIELD(fdecl)) {
	    if (!direction_is_collection(attribute_decl_direction(fdecl))) {
	      add_to_fiberset(lengthen_fiber(fdecl,base_fiber),
			      decl,FIBERSET_NORMAL_PROVIDE,
			      &fiberset_for(decl,FIBERSET_NORMAL_PROVIDE));
	    }
	  }
	}
      }
    }
    break;
  case KEYExpression:
    { Expression expr = (Expression)node;
      Declaration field = field_ref_p(expr);
      Declaration cdecl = constructor_call_p(expr);
      Declaration gdecl = shared_use_p(expr);
      if (field != NULL && !FIELD_DECL_IS_UNTRACKED(field)) {
	Expression object = field_ref_object(expr);
	if (EXPR_IS_RHS(expr)) {
	  if (fiber_debug & FIBER_INTRO) 
	    printf("%d: found field ref of %s\n",tnode_line_number(expr),
		   decl_name(field));
	  add_to_fiberset(lengthen_fiber(field,base_fiber),
			  object,
			  FIBERSET_NORMAL_REQUIRE,
			  &expr_fiberset_for(object,FIBERSET_NORMAL_REQUIRE));
	} else if (direction_is_collection(attribute_decl_direction(field))) {
	  if (fiber_debug & FIBER_INTRO) 
	    printf("%d: found field collect of %s\n",tnode_line_number(expr),
		   decl_name(field));
	  add_to_fiberset(lengthen_fiber(field,base_fiber),
			  object,
			  FIBERSET_REVERSE_PROVIDE,
			  &expr_fiberset_for(object,FIBERSET_REVERSE_PROVIDE));
	} else {
	  if (fiber_debug & FIBER_INTRO) 
	    printf("%d: found field assign of %s\n",tnode_line_number(expr),
		   decl_name(field));
	  /* force field to be required
	   * otherwise, instance often cannot be found.
	   */
	  add_to_fiberset(lengthen_fiber(field,base_fiber),
			  object,
			  FIBERSET_NORMAL_REQUIRE,
			  &expr_fiberset_for(object,FIBERSET_NORMAL_REQUIRE));
	}
      } else if (gdecl != NULL) {
	/* A shared use is implicitly of the form:
	 *     (X.shared_info).gdecl
	 * and so we combine the rules of using an attribute
	 * with the rule above for field refs.
	 */
	Declaration attr = responsible_node_shared_info(expr,state);
	/* If attr is NULL, it means we are at the shared level ourselves,
	 * and so we don't need to use the shared_info attribute.
	 */
	if (attr != NULL) {
	  if (EXPR_IS_RHS(expr)) {
	    if (fiber_debug & FIBER_INTRO) 
	      printf("%d: found shared use of %s\n",
		     tnode_line_number(expr),decl_name(gdecl));
	    add_to_fiberset(lengthen_fiber(gdecl,base_fiber),
			    attr, FIBERSET_NORMAL_REQUIRE,
			    &fiberset_for(attr,FIBERSET_NORMAL_REQUIRE));
	  } else {
	    if (fiber_debug & FIBER_INTRO) 
	      printf("%d: found shared (collection?) assign of %s\n",
		     tnode_line_number(expr),decl_name(gdecl));
	    add_to_fiberset(lengthen_fiber(gdecl,base_fiber),
			    attr, FIBERSET_REVERSE_PROVIDE,
			    &fiberset_for(attr,FIBERSET_REVERSE_PROVIDE));
	  }
	}
      } else if (cdecl != NULL) {
	Declaration pdecl = constructor_decl_phylum(cdecl);
	Declaration field;
	if (fiber_debug & FIBER_INTRO) 
	  printf("%d: found object construction of %s\n",
		 tnode_line_number(expr),decl_name(cdecl));
	for (field = NEXT_FIELD(pdecl);
	     field != NULL;
	     field = NEXT_FIELD(field)) {
	  FIBER f = lengthen_fiber(field,base_fiber);
	  if (direction_is_collection(attribute_decl_direction(field))) {
	    add_to_fiberset(f,expr,FIBERSET_NORMAL_PROVIDE,
			    &expr_fiberset_for(expr,FIBERSET_NORMAL_PROVIDE));
	    add_to_fiberset(f,expr,FIBERSET_REVERSE_REQUIRE,
			    &expr_fiberset_for(expr,FIBERSET_REVERSE_REQUIRE));
	  }
	}
      }
    }
    break;
  default:
    break;
  }
  return statep;
}

void print_fibersets_for_decl(Declaration decl) {
  FIBERSET *fss = fibersets_for(decl).set;
  int i=0;
  if ((fiber_debug & ALL_FIBERSETS) == 0) i += 4;
  for (; i < 6; ++i) {
    FIBERSET fs = fss[i];
    if (fs != NULL) {
      if (i&FIBERSET_TYPE_REVERSE) printf("reverse ");
      switch (i>>1) {
      default: printf("unknown "); break;
      case 0: printf("provide "); break;
      case 1: printf("require "); break;
      case 2: printf("final "); break;
      }
      printf("fibers for %s\n",decl_name(decl));
      print_fiberset(fs,stdout);
    }
  }
}

/* print out sets of fibers for each Declaration */
void *print_fibersets(void *statep, void *node) {
  /* UNUSED: STATE *state = (STATE)statep; */
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    print_fibersets_for_decl((Declaration)node);
    break;
  default:
    break;
  }
  return statep;
}

void print_shared_info_fibersets(STATE *state) {
  int i;
  for (i=0; i < state->phyla.length; ++i) {
    Declaration phy = state->phyla.array[i];
    print_fibersets_for_decl(phylum_shared_info_attribute(phy,state));
  }
}

void *finalize_fibersets_for_decl(Declaration decl) {
  switch (Declaration_KEY(decl)) {
  case KEYdeclaration:
    {
      fiberset_for(decl,FIBERSET_REVERSE_FINAL) =
	intersect_fiberset(fiberset_for(decl,FIBERSET_REVERSE_PROVIDE),
			   fiberset_for(decl,FIBERSET_REVERSE_REQUIRE));
      fiberset_for(decl,FIBERSET_NORMAL_FINAL) =
	intersect_fiberset(fiberset_for(decl,FIBERSET_NORMAL_PROVIDE),
			   fiberset_for(decl,FIBERSET_NORMAL_REQUIRE));
    }
  }
}

/* compute the final sets of fibers for each Declaration */
void *finalize_fibersets(void *statep, void *node) {
  /* UNUSED: STATE *state = (STATE *)statep; */
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    finalize_fibersets_for_decl((Declaration)node);
    break;
  default:
    break;
  }
  return statep;
}

/* compute final fibersets for shared info */
void finalize_shared_info_fibersets(STATE *state) {
  int i;
  for (i=0; i < state->phyla.length; ++i) {
    Declaration phy = state->phyla.array[i];
    finalize_fibersets_for_decl(phylum_shared_info_attribute(phy,state));
  }  
}

/*** FIBER ANALYSIS ***/

static void *add_fiberset_to_value_uses(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      if (!EXPR_IS_RHS(expr)) return fsp; /* don't look at LHS */
      switch (Expression_KEY(expr)) {
      case KEYvalue_use:
	/* if we have a use, and the use is unshared
	 * (i.e. the decl is not a shared value decl or
	 *  the use is at the top-level),
	 * we add the fiber to this expression.
	 */
	if (fs->tnode == USE_DECL(value_use_use(expr)) &&
	    (!DECL_IS_SHARED((Declaration)fs->tnode)||
	     shared_use_p(expr) == NULL))
	  add_to_fiberset(fs->fiber,expr,fstype,&expr_fiberset_for(expr,fstype));
	break;
      }
    }
    break;
  }
  return fsp;
}

/** Push fibers to definitions of direct fields. */
static void *add_fiberset_to_direct_defs(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      Declaration field;
      Expression object;
      if (!EXPR_IS_LHS(expr)) return fsp; /* only look at LHS */
      if ((field=field_ref_p(expr)) == NULL || /* not a field ref */
	  direction_is_collection(attribute_decl_direction(field)))/* not direct */
	  return fsp; /* not a field ref */
      object = field_ref_object(expr);
      switch (Expression_KEY(object)) {
      case KEYvalue_use:
	/* if we have a direct field assignment, and the use is unshared
	 * (i.e. the decl is not a shared value decl or
	 *  the use is at the top-level),
	 * we add the fiber to this expression
	 * 
	 */
	if (fs->tnode == USE_DECL(value_use_use(object)) &&
	    (!DECL_IS_SHARED((Declaration)fs->tnode)||
	     shared_use_p(expr) == NULL)) {
	  int fallthrough = FALSE;
	  switch (fstype) {
	  default:
	    fatal_error("Bad fstype in pushing to direct field assignment");
	  case FIBERSET_NORMAL_REQUIRE: break;
	  case FIBERSET_REVERSE_PROVIDE:
	    field = reverse_field(field);
	    break;
	  }
	  switch (shorten(field,fs->fiber)) {
	  case no_shorter: break;
	  case two_shorter:
	    fallthrough = TRUE;
	  case one_same:
	    add_to_fiberset(fs->fiber,expr,fstype,
			    &expr_fiberset_for(expr,fstype));
	    if (!fallthrough) break;
	  case one_shorter:
	    add_to_fiberset(fs->fiber->shorter,expr,fstype,
			    &expr_fiberset_for(expr,fstype));
	    break;
	  }
	}
	break;
      }
    }
    break;
  }
  return fsp;
}

static void *add_fiberset_to_value_defs(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      if (!EXPR_IS_LHS(expr)) return fsp; /* only look at LHS */
      switch (Expression_KEY(expr)) {
      case KEYvalue_use:
	/* if we have a use, and the use is unshared
	 * (i.e. the decl is not a shared value decl or
	 *  the use is at the top-level),
	 * we add the fiber to this expression.
	 */
	if (fs->tnode == USE_DECL(value_use_use(expr)) &&
	    (!DECL_IS_SHARED((Declaration)fs->tnode)||
	     shared_use_p(expr) == NULL))
	  add_to_fiberset(fs->fiber,expr,fstype,&expr_fiberset_for(expr,fstype));
	break;
      }
    }
    break;
  }
  return fsp;
}

static STATE *shared_info_state; /* needed to provide context */

/** Find calls to the function named in the fiber and push a fiber back to
 * the responsible node's shared info.
 */
static void *add_fiberset_to_function_use_shared_info(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      switch (Expression_KEY(expr)) {
      case KEYfuncall:
	if (attribute_decl_phylum(fs->tnode) == local_call_p(expr)) {
	  Declaration attr =
	    responsible_node_shared_info(node,shared_info_state);
	  if (attr != NULL) {
	    add_to_fiberset(fs->fiber,attr,fstype,&fiberset_for(attr,fstype));
	  }
	}
	break;
      }
    }
    break;
  }
  return fsp;
}

/** Find calls of functions and push a fiber onto each one's shared info. */
static void *add_fiberset_to_function_call_shared_info(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      switch (Expression_KEY(expr)) {
      case KEYfuncall:
	{ Declaration decl = local_call_p(expr);
	  if (decl != NULL) {
	    Declaration attr =
	      phylum_shared_info_attribute(decl,shared_info_state);
	    add_to_fiberset(fs->fiber,attr,fstype,&fiberset_for(attr,fstype));
	  }
	}
	break;
      }
    }
    break;
  }
  return fsp;
}

/** Push fiberset from a shared_info attribute to
 * other shared info sets for each production in which it is involved.
 */
static void *add_fiberset_to_shared_info(void *fsp, void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration tdecl = (Declaration)node;
      switch (Declaration_KEY(tdecl)) {
      default:
	/*! will have to be changed when we nest tlm's in polymorphics etc. */
	return NULL; /* don't bother looking inside (for now) */
      case KEYmodule_decl:
	return fsp;
      case KEYtop_level_match:
	{ FIBERSET fs = (FIBERSET)fsp;
	  Declaration shared_info = (Declaration)fs->tnode;
	  Declaration phylum = attribute_decl_phylum(shared_info);
	  int fstype = fs->fiberset_type;
	  int forward_p = FORWARD_FLOW_P(fstype);
	  Declaration lhs_decl = top_level_match_lhs_decl(tdecl);
	  Declaration rhs_decl = top_level_match_first_rhs_decl(tdecl);
	  if (forward_p) {
	    /* move from lhs to rhs */
	    /* printf("pushing shared info fiber forward\n"); */
	    if (node_decl_phylum(lhs_decl) == phylum) {
	      for (; rhs_decl != NULL; rhs_decl = next_rhs_decl(rhs_decl)) {
		Declaration phy = node_decl_phylum(rhs_decl);
		if (phy != NULL) {
		  Declaration attr =
		    phylum_shared_info_attribute(phy,shared_info_state);
		  add_to_fiberset(fs->fiber,attr,
				  fstype,&fiberset_for(attr,fstype));
		}
	      }
	      traverse_Declaration(add_fiberset_to_function_call_shared_info,
				   fsp,tdecl);
	    }
	  } else {
	    for (; rhs_decl != NULL; rhs_decl = next_rhs_decl(rhs_decl)) {
	      if (phylum == node_decl_phylum(rhs_decl)) {
		Declaration phy = node_decl_phylum(lhs_decl);
		Declaration attr =
		  phylum_shared_info_attribute(phy,shared_info_state);
		add_to_fiberset(fs->fiber,attr,
				fstype,&fiberset_for(attr,fstype));
		break;
	      }
	    }
	  }
	}
	return NULL;
	break;
      }
    }
    break;
  }
  return fsp;	
}

static void *add_fiberset_to_shared_use(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      switch (Expression_KEY(expr)) {
      case KEYvalue_use:
	{ Declaration sdecl = USE_DECL(value_use_use(expr));
	  /* if we have a shared use, that use is equivalent to
	   *   shared_info.sdecl
	   * and the direction is correct,
	   * then we apply the rules for field acess.
	   */
	  if (DECL_IS_SHARED(sdecl) &&
	      Declaration_KEY(sdecl) == KEYvalue_decl &&
	      (!FIELD_DECL_IS_UNTRACKED(sdecl) ||
	       !DECL_IS_SHARED(fs->fiber->field))) {
	    Declaration field = /* but see below */
	      EXPR_IS_LHS(expr) ? reverse_field(sdecl) : sdecl;
	    int fstype2 = EXPR_IS_LHS(expr) ?
	      (fstype ^ FIBERSET_TYPE_REVERSE) : fstype;

	    int fallthrough = FALSE;

	    if (fstype & FIBERSET_TYPE_REVERSE)
	      field = reverse_field(field);

	    if (fiber_debug & PUSH_FIBER) {
	      printf("%d:adding ",tnode_line_number(expr));
	      print_fiberset_entry(fs,stdout);
	      printf(" (field %s) to shared %s use of %s\n",
		     decl_name(field),
		     EXPR_IS_LHS(expr) ? "lhs" : "rhs",
		     decl_name(sdecl));
	    }

	    switch (shorten(field,fs->fiber)) {
	    case no_shorter: break;
	    case two_shorter:
	      fallthrough = TRUE;
	    case one_same:
	      add_to_fiberset(fs->fiber,expr,fstype2,
			      &expr_fiberset_for(expr,fstype2));
	      if (!fallthrough) break;
	    case one_shorter:
	      add_to_fiberset(fs->fiber->shorter,expr,fstype2,
			      &expr_fiberset_for(expr,fstype2));
	      break;
	    }
	  }
	}
	break;
      }
    }
    break;
  }
  return fsp;
}
	
static void *add_fiberset_to_attr_use(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      switch (Expression_KEY(expr)) {
      case KEYfuncall:
	if (fs->tnode == attr_ref_p(expr))
	  add_to_fiberset(fs->fiber,expr,fstype,&expr_fiberset_for(expr,fstype));
	break;
      }
    }
    break;
  }
  return fsp;
}

static void *add_fiberset_to_function_call(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  Declaration formal=(Declaration)fs->tnode;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYExpression:
    { Expression expr = (Expression)node;
      switch (Expression_KEY(expr)) {
      case KEYfuncall:
	{ Declaration func = formal_function_decl(formal);
	  if (local_call_p(expr) == func) {
	    Expression actual = function_formal_actual(func,formal,expr);
	    add_to_fiberset(fs->fiber,actual,fstype,&expr_fiberset_for(actual,fstype));
	  }
	}
      }
    }
  }
  return fsp;
}

static void *add_fiberset_to_pattern_vars(void *fsp, void *node) {
  FIBERSET fs = (FIBERSET)fsp;
  int fstype = fs->fiberset_type;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  default: return NULL;
  case KEYMatches:
  case KEYMatch:
  case KEYPatterns:
  case KEYPatternActuals:
    break;
  case KEYPattern:
    { Pattern pat = (Pattern)node;
      switch (Pattern_KEY(node)) {
      case KEYpattern_var:
	{ Declaration pvar = pattern_var_formal(pat);
	  add_to_fiberset(fs->fiber,pvar,fstype,&fiberset_for(pvar,fstype));
	}
      }
    }
    break;
  }
  return fsp;
}

static void push_fiber(FIBERSET fs, Declaration module, STATE *state) {
  int fstype = fs->fiberset_type;
  if (fiber_debug & PUSH_FIBER) {
    printf("Pushing ");
    print_fiberset_entry(fs,stdout);
    putc('\n',stdout);
  }
  switch (ABSTRACT_APS_tnode_phylum(fs->tnode)) {
  default: fatal_error("bad fiberset owner");
  case KEYDeclaration:
    { Declaration decl = (Declaration)fs->tnode;
      /* add to all uses.
       * It would be *much* faster to keep a list of uses
       * and then traverse the lists.
       */
      switch (Declaration_KEY(decl)) {
      default: fatal_error("bad fiberset owner declaration");
      case KEYvalue_decl:
	if (BACKWARD_FLOW_P(fstype)) {
	  Default def = value_decl_default(decl);
	  if (DECL_IS_SHARED(decl) &&
	      direction_is_collection(value_decl_direction(decl))) {
	    /* push fiber onto the shared info for this decl */
	    int new_fstype = fstype ^ FIBERSET_TYPE_REVERSE;
	    Declaration field = decl;
	    Declaration sattr =
	      phylum_shared_info_attribute(state->start_phylum,state);
	    if (fstype == FIBERSET_REVERSE_PROVIDE)
	      field = reverse_field(field);
	    add_to_fiberset(lengthen_fiber(field,fs->fiber),
			    sattr,new_fstype,&fiberset_for(sattr,new_fstype));
	  } else if (object_decl_p(decl) != NULL) {
	    traverse_Declaration(add_fiberset_to_direct_defs,fs,module);
	  } else {
	    /* broadcast to assignments (currently all unshared)
	     *! fix for top-level assignments to shared declarations
	     *! (priority: low)
	     */
	    traverse_Declaration(add_fiberset_to_value_defs,fs,module);
	  }
	  /* no matter what, we broadcast to the default value: */
	  switch (Default_KEY(def)) {
	  default: break;
	  case KEYcomposite:
	    { Expression value = composite_initial(def);
	      add_to_fiberset(fs->fiber,value,fstype,
			      &expr_fiberset_for(value,fstype));
	    }
	    break;
	  case KEYsimple:
	    { Expression value = simple_value(def);
	      add_to_fiberset(fs->fiber,value,fstype,
			      &expr_fiberset_for(value,fstype));
	    }
	    break;
	  }
	} else {
	  if (DECL_IS_SHARED(decl)) {
	    /* push fiber onto the use fiber for shared info */
	    Declaration field = decl;
	    Declaration sattr =
	      phylum_shared_info_attribute(state->start_phylum,state);
	    if (fstype == FIBERSET_REVERSE_REQUIRE)
	      field = reverse_field(field);
	    add_to_fiberset(lengthen_fiber(field,fs->fiber),sattr,fstype,
			    &fiberset_for(sattr,fstype));
	  }
	  /* no matter what we add fiber to non-shared uses */
	  traverse_Declaration(add_fiberset_to_value_uses,fs,module);
	  /* and if the result, we move to call sites */
	  if (result_decl_p(decl) != NULL) {
	    traverse_Declaration(add_fiberset_to_function_call,fs,module);
	  }
	}
	break;
      case KEYattribute_decl:
	if (ATTR_DECL_IS_SHARED_INFO(decl)) {
	  if (decl == phylum_shared_info_attribute(state->start_phylum,state)
	      && BACKWARD_FLOW_P(fstype)) {
	    /* rules for shared decls */
	    Declaration tdecl;
	    for (tdecl = NEXT_FIELD(MODULE_SHARED_INFO_PHYLUM(state->module));
		 tdecl != NULL;
		 tdecl = NEXT_FIELD(tdecl)) {
	      Declaration field = tdecl;
	      int fallthrough = FALSE;
	      if (fstype == FIBERSET_REVERSE_PROVIDE)
		field = reverse_field(tdecl);
	      switch (shorten(field,fs->fiber)) {
	      case no_shorter: break;
	      case two_shorter:
		fallthrough = TRUE;
	      case one_same:
		add_to_fiberset(fs->fiber,tdecl,fstype,
				&fiberset_for(tdecl,fstype));
		if (!fallthrough) break;
	      case one_shorter:
		add_to_fiberset(fs->fiber->shorter,tdecl,fstype,
				&fiberset_for(tdecl,fstype));
		break;
	      }
	      if (direction_is_collection(value_decl_direction(tdecl))) {
		int fstype2 = fstype ^ FIBERSET_TYPE_REVERSE;
		field = tdecl;
		if (fstype == FIBERSET_NORMAL_REQUIRE)
		  field = reverse_field(tdecl);
		switch (shorten(field,fs->fiber)) {
		case no_shorter: break;
		case two_shorter:
		  fallthrough = TRUE;
		case one_same:
		  add_to_fiberset(fs->fiber,tdecl,fstype2,
				  &fiberset_for(tdecl,fstype2));
		  if (!fallthrough) break;
		case one_shorter:
		  add_to_fiberset(fs->fiber->shorter,tdecl,fstype2,
				  &fiberset_for(tdecl,fstype2));
		  break;
		}
	      }
	    }
	  } else {
	    int i;
	    Declaration phy = attribute_decl_phylum(decl);
	    shared_info_state = state;
	    switch (Declaration_KEY(phy)) {
	    case KEYphylum_decl:
	      /* printf("shared info for phylum %s\n",decl_name(phy)); */
	      traverse_Declaration(add_fiberset_to_shared_info,fs,module);
	      break;
	    case KEYsome_function_decl:
	      /* printf("shared info for function %s\n",decl_name(phy)); */
	      if (BACKWARD_FLOW_P(fstype)) {
		traverse_Declaration(add_fiberset_to_function_use_shared_info,
				     fs,module);
	      } else {
		traverse_Declaration(add_fiberset_to_function_call_shared_info,
				     fs,phy);
	      }
	      break;
	    }
	    /*
	     * for forward flow:
	     *   with field: interested in RHS shared uses.
	     *   with reverse field: interested in LHS shared uses.
	     * for backward flow: uninterested
	     */
	    if (FORWARD_FLOW_P(fstype)) {
	      switch (Declaration_KEY(phy)) {
	      case KEYphylum_decl:
		/* find shared uses in the top level matches for this phylum */
		for (i=0; i < state->match_rules.length; ++i) {
		  Declaration tlm = state->match_rules.array[i];
		  if (Declaration_KEY(tlm) == KEYtop_level_match) {
		    Declaration node = responsible_node_declaration(tlm);
		    if (node_decl_phylum(node) == phy)
		      traverse_Declaration(add_fiberset_to_shared_use,fs,tlm);
		  }
		}
		break;
	      case KEYsome_function_decl:
		traverse_Declaration(add_fiberset_to_shared_use,fs,phy);
		break;
	      }
	    }
	  }
	} else {
	  traverse_Declaration(add_fiberset_to_attr_use,fs,module);
	}
	break;
      case KEYformal:
	if (BACKWARD_FLOW_P(fstype)) {
	  Declaration case_stmt;
	  if ((case_stmt = formal_in_case_p(decl)) != NULL) {
	    Expression expr = case_stmt_expr(case_stmt);
	    add_to_fiberset(fs->fiber,expr,fstype,
			    &expr_fiberset_for(expr,fstype));
	  } else if (ABSTRACT_APS_tnode_phylum(tnode_parent(decl))
		     == KEYPattern) {
	    /* a match variable not in a case, drop */
	  } else {
	    traverse_Declaration(add_fiberset_to_function_call,fs,module);
	  }
	} else {
	  traverse_Declaration(add_fiberset_to_value_uses,fs,module);
	}
	break;
      }
    }
    break;
  case KEYExpression:
    { Expression expr = (Expression)fs->tnode;
      int lhs_p = EXPR_IS_LHS(expr);
      int forward_p = FORWARD_FLOW_P(fstype);
      if ((lhs_p && !forward_p) || (!lhs_p && forward_p)) {
	/* look up */
	/* four situations:
	 * RHS && NORMAL && PROVIDE
	 * RHS && REVERSE && REQUIRE
	 * LHS && NORMAL && REQUIRE
	 * LHS && REVERSE && PROVIDE
	 */
	void *parent = tnode_parent(expr);
	switch (ABSTRACT_APS_tnode_phylum(parent)) {
	default: fatal_error("%d:unexpected parent phylum:%d",
			     tnode_line_number(parent),
			     ABSTRACT_APS_tnode_phylum(parent));
	case KEYExpression:
	  { Expression expr = (Expression)parent;
	    switch (Expression_KEY(expr)) {
	    default:
	      fatal_error("%d:unexpected parent expression:%d",
			  tnode_line_number(parent),
			  Expression_KEY(expr));
	      break;
	    case KEYrepeat:
	      add_to_fiberset(fs->fiber,expr,fstype,
			      &expr_fiberset_for(expr,fstype));
	      break;
	    }
	  }
	  break;
	case KEYPattern:
	  /*!! for now */
	  break;
	case KEYDefault:
	  { Declaration decl = (Declaration)tnode_parent(parent);
	    add_to_fiberset(fs->fiber,decl,fstype,&fiberset_for(decl,fstype));
	  }
	  break;
	case KEYDeclaration:
	  { Declaration stmt = (Declaration)parent;
	    switch (Declaration_KEY(stmt)) {
	    default: fatal_error("%d:unexpected parent declaration",
				 tnode_line_number(parent));
	    case KEYif_stmt:
	      /* drop on floor */
	      break;
	    case KEYcase_stmt:
	      /* must be forward flow */
	      /* We just push the fiber onto all pattern variables
	       * in the match statement, possibly too conservative.
	       */
	      traverse_Matches(add_fiberset_to_pattern_vars,
			       fs,case_stmt_matchers(stmt));
	      break;
	    case KEYassign:
	      if (lhs_p) {
		/* backward flow */
		Expression rhs = assign_rhs(stmt);
		add_to_fiberset(fs->fiber,rhs,fstype,&expr_fiberset_for(rhs,fstype));
	      } else {
		/* forward flow */
		Expression lhs = assign_lhs(stmt);
		add_to_fiberset(fs->fiber,lhs,fstype,&expr_fiberset_for(lhs,fstype));
	      }
	    }
	  }
	  break;
	case KEYActuals:
	  /* move up to call node */
	  do {
	    parent = tnode_parent(parent);
	  } while (ABSTRACT_APS_tnode_phylum(parent) == KEYActuals);
	  { Expression pexpr = (Expression)parent;
	    int plhs_p = EXPR_IS_LHS(pexpr);
	    switch (Expression_KEY(pexpr)) {
	    default: fatal_error("%d:unanticipated parent expression:",
				 tnode_line_number(pexpr));
	    case KEYfuncall:
	      { Declaration decl;
		if (lhs_p) fatal_error("object with field ref cannot be lhs");
		if ((decl = field_ref_p(pexpr)) != NULL) {
		  Declaration field = decl;
		  int fstype2 = fstype; /* The type for the object */
		  int fallthrough = FALSE;
		  if (plhs_p) {
		    /* for lhs field assignments, reverse the fiberset type */
		    fstype2 ^= FIBERSET_TYPE_REVERSE;
		  }
		  if (fstype2 & FIBERSET_TYPE_REVERSE) {
		    /* for opposite direction sets, we use f' instead of f */
		    field = reverse_field(field);
		    if (field == 0) {
		      fprintf(stderr,"flags for %s are 0%o\n",
			      decl_name(decl),Declaration_info(decl)->decl_flags);
		      fatal_error("No reverse field for %s\n",decl_name(decl));
		    }
		  }
		  switch (shorten(field,fs->fiber)) {
		  case no_shorter: break;
		  case two_shorter:
		    fallthrough = TRUE;
		  case one_same:
		    add_to_fiberset(fs->fiber,pexpr,fstype2,
				    &expr_fiberset_for(pexpr,fstype2));
		    if (!fallthrough) break;
		  case one_shorter:
		    add_to_fiberset(fs->fiber->shorter,pexpr,fstype2,
				    &expr_fiberset_for(pexpr,fstype2));
		    break;
		  }
		} else if ((decl = local_call_p(pexpr)) != NULL) {
		  switch (Declaration_KEY(decl)) {
		  default: fatal_error("%d:unknown function decl",
				       tnode_line_number(decl));
		  case KEYsome_function_decl:
		    { Declaration formal =
			function_actual_formal(decl,expr,pexpr);
		      add_to_fiberset(fs->fiber,formal,fstype,
				      &fiberset_for(formal,fstype));
		    }
		  }
		} else {
		  /* other call : push to caller: */
		  add_to_fiberset(fs->fiber,pexpr,fstype,
				  &expr_fiberset_for(pexpr,fstype));
		}
	      }
	      break;
	    }
	  }
	  break;
	}
      } else {
	/* move down */
	/* four situations
	 * LHS && NORMAL && PROVIDE
	 * LHS && REVERSE && REQUIRE
	 * RHS && NORMAL && REQUIRE
	 * RHS && REVERSE && PROVIDE
	 */
	switch (Expression_KEY(expr)) {
	default: fatal_error("%d:unexpected expr for fibering",
			     tnode_line_number(expr));
	case KEYinteger_const:
	case KEYreal_const:
	case KEYstring_const:
	case KEYchar_const:
	case KEYundefined:
	case KEYno_expr:
	  break;
	case KEYvalue_use:
	  { Declaration decl = USE_DECL(value_use_use(expr));
	    if (decl != NULL && DECL_IS_LOCAL(decl)) {
	      if (DECL_IS_SHARED(decl) && shared_use_p(expr) != NULL) {
		Declaration sattr = responsible_node_shared_info(expr,state);
		Declaration field = decl;
		int fstype2 = fstype; /* The type for the object */
		if (fiberset_reverse_p(fs)) {
		  /* for opposite direction sets, we use f' instead of f */
		  field = reverse_field(field);
		}
		if (lhs_p) {
		  /* for lhs field assignments, reverse the fiberset type */
		  fstype2 ^= FIBERSET_TYPE_REVERSE;
		}
		add_to_fiberset(lengthen_fiber(field,fs->fiber),
				sattr, fstype2,
				&fiberset_for(sattr,fstype2));
	      } else {
		add_to_fiberset(fs->fiber,decl,fstype,
				&fiberset_for(decl,fstype));
	      }
	    }
	  }
	  break;
	case KEYfuncall:
	  { Declaration decl;
	    if ((decl = field_ref_p(expr)) != NULL) {
	      Expression object = field_ref_object(expr);
	      Declaration field = decl;
	      int fstype2 = fstype; /* The type for the object */
	      if (fiberset_reverse_p(fs)) {
		/* for opposite direction sets, we use f' instead of f */
		field = reverse_field(field);
	      }
	      if (lhs_p) {
		/* for lhs field assignments, reverse the fiberset type */
		fstype2 ^= FIBERSET_TYPE_REVERSE;
	      }
	      if (direction_is_collection(attribute_decl_direction(decl))) {
		add_to_fiberset(lengthen_fiber(field,fs->fiber),
				object, fstype2,
				&expr_fiberset_for(object,fstype2));
	      } else { /* direct field */
		/* Direct field accesses are handled specially.
		 * Ideally, they would simply be a simplification of
		 * collect assigns, (in which case we would reverse the 
		 * fiberset type and then move back to the creation point
		 * where the fiberset would be reversed again.
		 * But they aren't.  
		 * Thus we need to move the information directly to
		 * the declaration.  I think it is OK that that
		 * this information doesn't land on the constructor, but I'm
		 * not sure (2002/5/24)
		 */
		Declaration odecl;
		switch (Expression_KEY(object)) {
		default: fatal_error("%d: too complex direct field access",
				     tnode_line_number(object));
		case KEYvalue_use:
		  odecl = USE_DECL(value_use_use(object));
		  break;
		case KEYfuncall:
		  odecl = attr_ref_p(object);
		  if (odecl == NULL) odecl = field_ref_p(object);
		  break;
		}
		if (odecl != NULL) {
		  add_to_fiberset(lengthen_fiber(field,fs->fiber),
				  odecl,fstype, /* not fstype2 !! */
				  &fiberset_for(odecl,fstype));
		} else {
		  fatal_error("%d: unknown direct field access",
			      tnode_line_number(object));		  
		}
	      }
	    } else if ((decl = attr_ref_p(expr)) != NULL) {
	      add_to_fiberset(fs->fiber,decl,fstype,&fiberset_for(decl,fstype));
	    } else if ((decl = local_call_p(expr)) != NULL) {
	      switch (Declaration_KEY(decl)) {
	      default: fatal_error("%d:unknown function decl",
				   tnode_line_number(decl));
	      case KEYfunction_decl:
	      case KEYprocedure_decl:
		{ Declaration result = some_function_decl_result(decl);
		  add_to_fiberset(fs->fiber,result,fstype,&fiberset_for(result,fstype));
		}
	      }
	    } else if ((decl = constructor_call_p(expr)) != NULL) {
	      Declaration pdecl = constructor_decl_phylum(decl);
	      Declaration fdecl = NULL;
	      int fstype2 = fstype ^ FIBERSET_TYPE_REVERSE;
	      /* All constructor arguments are strict and so have trivial
	       * fiber rules:
	       */
	      Expression arg;
	      for (arg = first_Actual(funcall_actuals(expr));
		   arg != NULL;
		   arg = Expression_info(arg)->next_expr) {
		add_to_fiberset(fs->fiber,arg,fstype,
				&expr_fiberset_for(arg,fstype));
	      }
	      /* Use all attributes "fields" for phylum */
	      for (fdecl = NEXT_FIELD(pdecl);
		   fdecl != NULL;
		   fdecl = NEXT_FIELD(fdecl)) {
		if (direction_is_collection(attribute_decl_direction(fdecl))) {
		  Declaration reversed = reverse_field(fdecl);
		  /* first:
		   * F'(o) >= f . ({e} U f-1 . F(o))
		   * (ignore the {e} here.  Handled in initialization)
		   */
		  switch (shorten(fdecl,fs->fiber)) {
		  case no_shorter:
		    /* nothing to do */
		    break; 
		  case one_shorter:
		  case two_shorter:
		  case one_same:
		    /* result is same as we had before */
		    add_to_fiberset(fs->fiber,expr,fstype2,
				    &expr_fiberset_for(expr,fstype2));
		    break;
		  }
		  /* now:
		   * F'(o) >= f' . f'-1 . F(o)
		   */
		  switch (shorten(reversed,fs->fiber)) {
		  case no_shorter:
		    /* nothing to do */
		    break; 
		  case one_shorter:
		  case two_shorter:
		  case one_same:
		    /* result is same as we had before */
		    add_to_fiberset(fs->fiber,expr,fstype2,
				    &expr_fiberset_for(expr,fstype2));
		    break;
		  }
		}
	      }
	    } else { /* a primitive, a non-local function */
	      /* copy to each argument */
	      Expression arg;
	      for (arg = first_Actual(funcall_actuals(expr));
		   arg != NULL;
		   arg = Expression_info(arg)->next_expr) {
		add_to_fiberset(fs->fiber,arg,fstype,&expr_fiberset_for(arg,fstype));
	      }
	    }
	  }
	  break;
	}
      }
    }
    break;
  }
}

void fiber_module(Declaration module, STATE *s) {
  /* initialise FIELD_DECL_P and DECL_IS_SHARED */
  init_field_decls(module,s);
  /* initialize EXPR_IS_RHS and EXPR_IS_LHS */
  traverse_Declaration(init_rhs_lhs,s,module);
  /* set up initial fiber sets */
  
  /* entry point of callsite AI */
  if (fiber_debug & CALLSITE_INFO) {
    callset_AI(module, s);
  }

  traverse_Declaration(preinitialize_fibersets,s,module);
  traverse_Declaration(initialize_fibersets,s,module);
  /* go through fiber worklist */
  { FIBERSET fs;
    for (;;) {
      fs = get_next_fiber();
      if (fs == NULL) break;
      push_fiber(fs,module,s);
    }
  }
  /* DONE! */
  traverse_Declaration(finalize_fibersets,s,module);
  finalize_shared_info_fibersets(s);
  if (fiber_debug & FIBER_FINAL) {
    traverse_Declaration(print_fibersets,s,module);
    print_shared_info_fibersets(s);
  }
  fflush(stdout);
}

/*** DEBUGGING OUTPUT ***/

void print_fiber(FIBER fiber, FILE *stream) {
  while (fiber != NULL && fiber->field != NULL) {
    fputs(decl_name(fiber->field),stream);
    fiber = fiber->shorter;
    if (fiber->field != NULL) fputc('.',stream);
  }
}

void print_fiberset(FIBERSET fs, FILE *stream) {
  while (fs != NULL) {
    fputs("  ",stream); print_fiber(fs->fiber,stream); fputc('\n',stream);
    fs = fs->rest;
  }
}

void print_fiberset_entry(FIBERSET fs, FILE *stream) {
  void *tnode = fs->tnode;
  int fstype = fs->fiberset_type;
  switch (fstype) {
  case FIBERSET_NORMAL_PROVIDE: printf("provide "); break;
  case FIBERSET_REVERSE_PROVIDE: printf("provide' "); break;
  case FIBERSET_NORMAL_REQUIRE: printf("require "); break;
  case FIBERSET_REVERSE_REQUIRE: printf("require' "); break;
  default: printf("%d ",fstype); break;
  }
  print_fiber(fs->fiber,stdout);
  switch (ABSTRACT_APS_tnode_phylum(tnode)) {
  case KEYDeclaration:
    printf(" of %s",decl_name((Declaration)tnode));
    break;
  case KEYExpression:
    { Expression expr = (Expression)tnode;
      switch (Expression_KEY(expr)) {
      case KEYvalue_use:
	printf(" of use(%s)",
	       symbol_name(use_name(value_use_use(expr))));
	break;
      case KEYfuncall:
	{ Declaration decl;
	  if ((decl = field_ref_p(expr)) != NULL ||
	      (decl = attr_ref_p(expr)) != NULL) {
	    Expression object = field_ref_object(expr);
	    switch (Expression_KEY(object)) {
	    default:
	      printf(" of ?.%s",decl_name(decl));
	      break;
	    case KEYvalue_use:
	      printf(" of %s.%s",
		     symbol_name(use_name(value_use_use(object))),
		     decl_name(decl));
	      break;
	    }
	  } else {
	    Expression func = funcall_f(expr);
	    switch (Expression_KEY(func)) {
	    case KEYvalue_use:
	      {
		Use u = value_use_use(func);
		switch (Use_KEY(u)) {
		case KEYuse:
		  printf(" of %s(...)",symbol_name(use_name(u)));
		  break;
		case KEYqual_use:
		  printf(" of ...$%s(...)",symbol_name(qual_use_name(u)));
		  break;
		}
	      }
	      break;
	    }
	  }
	}
	break;
      }
    }
    break;
  }
}


