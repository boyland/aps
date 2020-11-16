#include <stdio.h>
#include <stdlib.h>
#include "jbb.h"
#include "jbb-alloc.h"
#include "aps-ag.h"

#define ADD_FIBER 1
#define ALL_FIBERSETS 2
#define PUSH_FIBER 4
int fiber_debug = 0;

typedef struct edges {
  struct edges *rest;   // rest of esges
  Declaration  edge;    // f or f. or empty(null).
  int from; // FSA from
} *EDGES;           // list of edges

typedef struct nodeset {
  struct nodeset *rest;   // the rest nodes
  int            node;    // node's index number [1,enough large]
} *NODESET;

typedef struct   node_in_tree {
  struct fiber as_fiber;
  NODESET ns;       // a set of original nodes from FSA.
  int     dir;      // BACKWARD or FOREWARD
  int     index;    // the index number in the tree: from 1
  int hash;
} *NODE_IN_TREE;    // the node in the result tree

typedef struct worklist {
  struct worklist *rest;
  NODE_IN_TREE    tree_node;
} *WORKLIST;

#define EMPTY_NODESET (NODESET)NULL

// static data declaration
// those data are needed to process oset/uset and at last to
//   compute DFA.

#define MAX_FSA_NUMBER 1800
#define MAX_DFA_NUMBER 1000

// 1-D arary: a list of edges to any node
EDGES FSA_graph[MAX_FSA_NUMBER];    //! need to be created dynamically

// record the DFA nodes: the index number of DFA node is set from 1.
NODE_IN_TREE DFA[MAX_DFA_NUMBER];

// DFA is a tree with edge f in F b/w nodes
Declaration DFA_tree[MAX_DFA_NUMBER][MAX_DFA_NUMBER]; // edge from j to i for [j][i]

NODESET DFA_result_set;         // nodeset we are interested in in DFA.
                                // Those states will lead to the final state.
WORKLIST wl= NULL;
int BACKWARD_ = 1;
int FORWARD_ = 2;
int DFA_node_number = 1;		// there is always 1 init node. (final states)

/////////////  DATA definition end ////////////////////////////////


BOOL fiber_is_reverse(FIBER f)
{
  NODE_IN_TREE n = (NODE_IN_TREE)f;
  if (n == 0) return FALSE;
  return n->dir != FORWARD_;
}

/* The following function will not be necessary
 * Once the DFA graph is explicit encoded using fibers and not with
 * DFA_tree.
 */
void print_fibers(STATE *s)
{
  int i, n;
  n = s->fibers.length;
  for (i=1; i < n; ++i) {
    FIBER f = s->fibers.array[i];
    ALIST l = f->longer;
    printf("Fiber #%d: ",i);
    print_fiber(f,stdout);
    if (fiber_is_reverse(f))
      puts(" BACKWARD");
    else
      puts("");
    while (l != NULL) {
      printf("  %s . this = ", decl_name((Declaration)alist_key(l)));
      print_fiber((FIBER)alist_value(l),stdout);
      printf("\n");
      l = alist_next(l);
    }
  }
}

/* The following function should be done as part of fiber module.
 * Then we don't need to call this here.
 *
 * We shouldn't use static arrays at all, and better still would be
 * avoid global things as well, and put everything in the "STATE".
 */

void add_fibers_to_state(STATE *s)
{
  int n = DFA_node_number+1;
  int i;
  VECTORALLOC(s->fibers,FIBER,n);
  s->fibers.array[0] = base_fiber;
  for (i = 1; i < n; ++i) {
    s->fibers.array[i] = &DFA[i]->as_fiber;
  }
}


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

static struct node_in_tree base_DFA_node = { {NULL,NULL,NULL}, NULL, 2, 1, 0};
FIBER base_fiber = &base_DFA_node.as_fiber;
  
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
BOOL local_type_p(Type);

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
  if (ABSTRACT_APS_tnode_phylum(tnode) == KEYExpression) {
    Expression e = (Expression)tnode;
    Type ty = infer_expr_type(e);
    if (!local_type_p(ty)) {
      if (fiber_debug & ADD_FIBER) {
	struct fiberset fss;
	fss.rest = 0;
	fss.fiber = fiber;
	fss.tnode = tnode;
	fss.fiberset_type = fstype;
	printf("%d: Not adding ",tnode_line_number(tnode));
	print_fiberset_entry(&fss,stdout);
	printf(" (not of local type)\n");
      }
      return; /* should not have fibers */
    }
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

BOOL local_type_p(Type ty) { /* could this type carry an object? */
  switch (Type_KEY(ty)) {
  default:
    return TRUE;
  case KEYtype_use:
    {
      Declaration d = USE_DECL(type_use_use(ty));
      return (d != NULL && DECL_IS_LOCAL(d));
    }
    break;
  case KEYremote_type:
    return local_type_p(remote_type_nodetype(ty));
  case KEYfunction_type:
  case KEYno_type:
    /* non-carrying */
    return FALSE;
  }
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

// return true if this pattern variable is controlled by a case
// statement and if so what the case statement is.
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

Declaration constructor_pcall_p(Pattern p)
{
  switch (Pattern_KEY(p)) {
  default: break;
  case KEYpattern_call:
    {
      Pattern func = pattern_call_func(p);
      switch (Pattern_KEY(func)) {
      default: break;
      case KEYpattern_use:
	{ Declaration decl = USE_DECL(pattern_use_use(func));
	  if (decl == NULL) aps_error(func,"unbound function");
	  switch (Declaration_KEY(decl)) {
	  default: break;
	  case KEYconstructor_decl:
	    return decl;
	  }
	}
      }
    }
  }
  return NULL;
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
      Expression_info(expr)->expr_flags |= EXPR_RHS_FLAG;
    }
    break;
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
      case KEYpragma_call:
	return NULL;
      case KEYassign:
	Expression_info(assign_lhs(decl))->expr_flags |= EXPR_LHS_FLAG;
	traverse_Expression_skip(init_rhs_lhs,key,assign_lhs(decl));
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
  case KEYDeclaration: {
		Declaration decl = (Declaration)node;
    fss = &fibersets_for((Declaration)node);

    /* init oset and uset */
    Declaration_info(decl)->oset = EMPTY_OSET;
    Declaration_info(decl)->uset = EMPTY_USET;

    // init the index of node on decl.
    Declaration_info(decl)->index = 0;
    switch (Declaration_KEY(decl)) {
      default: break;
      case KEYvalue_decl: {
	Declaration cdecl;
	if ((cdecl = object_decl_p(decl)) != NULL) {
	  
				// init oset of object to {object}
	  OSET oset = (OSET)malloc(sizeof(struct oset));
	  oset->rest = NULL;
	  oset->o = decl;
	  Declaration_info(decl)->oset = oset;
	}; // if 
	break;
      }	// value_decl
    }// switch
  }// Delaration
  break; 
  case KEYExpression: {
    Expression expr = (Expression)node;
    fss = &expr_fibersets_for(expr);
    
    // init index of node on expr.
    Expression_info(expr)->index = 0;
  } // Expression
  break;
  } // switch
  
  if (fss)
    for (i=0; i < 6; ++i)
      fss->set[i] = 0;
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

STATE* mystate;

Declaration uitem_field(Expression e) 
{
  Declaration fdecl;
  fdecl = shared_use_p(e);
  if (fdecl == NULL) {
    fdecl = field_ref_p(e);
  }
  return fdecl;
}

// DFA algorithm start here.
void print_oset(OSET oset){
	OSET p= oset;
	while (p != NULL) {
		printf("    %s:%d\n", decl_name(p->o), tnode_line_number(p->o));
		p = p->rest;
	}
}

void print_uset(USET uset){
	USET p= uset;
	while (p != NULL) {
		printf("    (%d(0x%p),%s%s:%d)\n", 
		       tnode_line_number(p->u),
		       p->u,
		       EXPR_IS_LHS(p->u) ? "dot " : "",
		       decl_name(uitem_field(p->u)), tnode_line_number(p->u));
		p = p->rest;
	}
}

USET get_uset(Declaration decl)
{
	return Declaration_info(decl)->uset;
}

OSET get_oset(Declaration decl)
{
	return Declaration_info(decl)->oset;
}

static BOOL done = FALSE;

USET uset_union(USET,USET);
OSET oset_union(OSET,OSET);

USET add_to_uset(Declaration decl, USET uset)
{
	USET *head = &Declaration_info(decl)->uset;
	//printf("Before add_to_uset(%s), set = \n",decl_name(decl));
	//print_uset(*head);
	USET new_uset = uset_union(*head, uset);
	if (*head != new_uset) {
	  done = FALSE;
	  if (fiber_debug & ADD_FIBER) {
	    printf("Added (%d,%s%s) to uset of ",
			   tnode_line_number(new_uset->u),
			   EXPR_IS_LHS(new_uset->u) ? "dot " : "",
		   decl_name(uitem_field(new_uset->u)));
	    switch (Declaration_KEY(decl)) {
	    case KEYassign:
	      printf("assignment:%d\n",tnode_line_number(decl));
	      break;
	    default:
	      printf("%s\n",decl_name(decl));
	      break;
	    }
	  }
	  *head = new_uset;
	}
	return new_uset;
}

OSET add_to_oset(Declaration decl, OSET oset)
{
	OSET *head = &Declaration_info(decl)->oset;
	//printf("Before add_to_set, set = \n");
	//print_oset(*head);
	OSET new_oset = oset_union(*head,oset);
	//printf("After unioning with\n");
	//print_oset(oset);
	//printf("After add_to_oset: The result is\n");
	//print_oset(new_oset);
	if (new_oset != *head) {
	  done = FALSE;
	  if (fiber_debug & ADD_FIBER) {
	    printf("Added %s to oset for ",
		   		decl_name(new_oset->o));
	    switch (Declaration_KEY(decl)) {
	    case KEYassign:
	      printf("assignment:%d\n",tnode_line_number(decl));
	      break;
	    default:
	      printf("%s\n",decl_name(decl));
	      break;
	    }
	  }
	  *head = new_oset;
	}
	return new_oset;
}

// add os2 into os1 without modifying either.
// The result may include parts of either argument.
OSET oset_union(OSET os1, OSET os2)
{
	OSET p, q;	

	if (os1 == EMPTY_OSET) 
		return os2;
	else if (os2 == EMPTY_OSET)
		return os1;
	
	for (q = os2; q != NULL; q = q->rest) {
		for (p = os1; p!=NULL; p=p->rest){
			if (p->o == q->o)
				break;
		} // goto next q
		if (p == NULL) { // not exist
		  p = (OSET)malloc(sizeof(struct oset));
		  p->o = q->o;
		  p->rest = os1;
		  os1 = p;
		}
	}
	return os1;	
}

// add us2 into us1 without modifying either.
// The result may include parts of either argument.
USET uset_union(USET us1, USET us2)
{
	USET p, q;	

	if (us1 == EMPTY_USET) 
		return us2;
	else if (us2 == EMPTY_USET)
		return us1;
	
	for (q = us2 ; q != NULL; q = q->rest) {
		for (p = us1; p!=NULL; p=p->rest){
			if (p->u == q->u)
				break;		// goto next q
		}
		if (p == NULL) { // not exist
		  p = (USET)malloc(sizeof(struct uset));
		  p->u = q->u;
		  p->rest = us1;
		  us1 = p;
		}
	}
	return us1;	
}

OSET doOU(Expression e, USET uset);

static int recursion_level = 0;
#define ENTER ++recursion_level
#define EXIT  --recursion_level
#define RETURN EXIT; return

// calculating USET of e given oset.
USET doUO(Expression e, OSET oset) {
  if (!local_type_p(infer_expr_type(e))) oset = EMPTY_OSET;
  ENTER;
//  printf("%d: doUO starting on line %d.\n", recursion_level,
//				 tnode_line_number(e));
  if (e==NULL) { RETURN NULL; }
  switch (Expression_KEY(e)) {
  default : aps_error(e, "not a proper expression to compute USET.\n");
    RETURN EMPTY_USET;
    break;

 	case KEYvalue_use:	
		{	Declaration sdecl = USE_DECL(value_use_use(e));
		// Several cases
		// * 1> a use of a local "shared" global variable
		// *    perhaps a global collection
		// * 2> a use of a local (attribute)
	//		 
//		printf("DEBUG: in KEYvalue.\n");
		if (!DECL_IS_LOCAL(sdecl)) {
//			printf("DEBUG: not local.\n");
		  RETURN EMPTY_USET;
		} else if (DECL_IS_SHARED(sdecl)) {
//			printf("DEBUG: shared.\n");
		  USET p = (USET)malloc(sizeof(struct uset));
		  p->u = e;
		  p->rest = NULL;
		  add_to_uset(responsible_node_shared_info(e, mystate),p);
		  add_to_oset(sdecl,oset);
		  RETURN get_uset(sdecl);
		} else if (!DECL_IS_SYNTAX(sdecl)) {
//			printf("DEBUG: not syntax.\n");
		  add_to_oset(sdecl,oset);
		  RETURN get_uset(sdecl);
	  } else {
		  aps_error(e,"assigning a syntax decl");
		}
		break;
		} // case KEYvalue_use
		case KEYfuncall:
		// * Several cases
		// * 1> X.a (attr_ref)
		// * 2> w.f (field_ref)
		// 
		{	Declaration fdecl;

		// attr ref: X.a 
		if ((fdecl = attr_ref_p(e)) != NULL) {
		  add_to_oset(fdecl,oset);
		  RETURN get_uset(fdecl);
		} else if ((fdecl = field_ref_p(e))!= NULL) {
		// field ref: w.f
		  Expression object = field_ref_object(e);

		  USET newuset = (USET)malloc(sizeof(struct uset));
		  OSET o_w; 
		  newuset->rest = NULL;
		  newuset->u = e;
		  o_w = doOU(object, newuset);
			RETURN EMPTY_USET;
		}
		}  // case funcall
	} // switch
}

// doUOp: calculate Uset of pattern given an OSet.
USET doUOp(Pattern pat, OSET oset){
  if (!local_type_p(infer_pattern_type(pat))) oset = EMPTY_OSET;
  switch (Pattern_KEY(pat)){
  default: {
    aps_error(pat, "unknown case for doUOp.");
    return EMPTY_USET;
    break;}
  
  case KEYno_pattern: {
    //  			if (fiber_debug & ALL_FIBERSETS) 
    //  					printf("doUOp (no_pattern)starting on line %d.\n", tnode_line_number(pat));
    return EMPTY_USET;
    break;}
  
  case KEYand_pattern: {
    // 			if (fiber_debug & ALL_FIBERSETS) 
    //  				printf("doUOp (and_pattern) starting on line %d.\n", tnode_line_number(pat));
    USET u1 = doUOp(and_pattern_p1(pat), oset);
    USET u2 = doUOp(and_pattern_p2(pat), oset);
    return uset_union(u1, u2);
    break; 	}
  
  case KEYpattern_var: {
    //  			if (fiber_debug & ALL_FIBERSETS) 
    //  				printf("doUOp (pattern_var) starting on line %d.\n", tnode_line_number(pat));
    Declaration decl = pattern_var_formal(pat);
    add_to_oset(decl, oset);
    return get_uset(decl);
    break; }
  
  case KEYpattern_call: {
    //  			if (fiber_debug & ALL_FIBERSETS) 
    //  				printf("doUOp (pattern_call) starting on line %d.\n", tnode_line_number(pat));
    USET uset = EMPTY_USET;
    Pattern p;
    // assume primitive
    for (p = first_PatternActual(pattern_call_actuals(pat)); p; p = PAT_NEXT(p)) 
      uset = uset_union(uset, doUOp(p, oset));
    return uset;
    break;}
  
  case KEYrest_pattern:{
    //  			if (fiber_debug & ALL_FIBERSETS) 
    //  				printf("doUOp (rest_pattern) starting on line %d.\n", tnode_line_number(pat));
    return doUOp(rest_pattern_constraint(pat), oset);
    break;}
  
  case KEYcondition:{
    //  			if (fiber_debug & ALL_FIBERSETS) 
    //  				printf("doUOp (condition) starting on line %d.\n", tnode_line_number(pat));
    return EMPTY_USET;
    break;}
  }  // switch 
}


int same_field(Expression e1, Expression e2) {
  return uitem_field(e1) == uitem_field(e2);
}


// get Oset of e given a uset.
OSET doOU(Expression e, USET uset)
{
  if (!local_type_p(infer_expr_type(e))) uset = EMPTY_USET;
  ENTER;
//  printf("%d: doOU starting on line %d.\n", recursion_level, tnode_line_number(e));
  if (e==NULL) { RETURN EMPTY_OSET; }
  switch (Expression_KEY(e)) {
	default: aps_error(e, "not a proper expression to compute OSET.\n");
	RETURN EMPTY_OSET;
	break;
	case KEYinteger_const:
	case KEYreal_const:
	case KEYchar_const:
	case KEYstring_const:
		RETURN EMPTY_OSET ;
//		break;

        case KEYrepeat:
          RETURN doOU(repeat_expr(e),uset);

 	case KEYvalue_use:
		{	Declaration sdecl = USE_DECL(value_use_use(e));
		// Several cases
		// * 1> a use of a non-local constant, like 'nil'
		// *    or MAX_INT or something else immutable and external.
		// * 2> a use of a local "shared" global variable
		// *    perhaps a global collection
		// * 3> a use of an object
		// * 4> a use of a local (attribute)
		// * 5> a use of a syntax-tree node.
		// *    (an error, for now)
		//		 
		if (!DECL_IS_LOCAL(sdecl)) {
//			printf("	%d: decl is local.\n", tnode_line_number(e));
		  RETURN EMPTY_OSET;
		} else if (DECL_IS_SHARED(sdecl)) {
//			printf("	%d: decl is shared.\n", tnode_line_number(e));
		  Declaration rsi = responsible_node_shared_info(e, mystate);
		  if (rsi != NULL) {
		    // uset of shared decl is added to the responsible node.
		    //? add_to_uset(rsi, get_uset(sdecl));  
		    USET p = (USET)malloc(sizeof(struct uset));
		    p->u = e;
		    p->rest = NULL;
		    add_to_uset(rsi,p);

		  } else {
		    // we are in the top-level area, and
		    // uses aren't remote
		  }
		  add_to_uset(sdecl,uset);
		  RETURN get_oset(sdecl);
		} else if (DECL_IS_OBJECT(sdecl)) {
//			printf("	%d: decl is OBJECT.\n", tnode_line_number(e));
		  add_to_uset(sdecl,uset);
		  RETURN get_oset(sdecl); // initialized to {sdecl}
		} else if (!DECL_IS_SYNTAX(sdecl)) {
//			printf("	%d: doOU: decl is NOT syntax.\n", tnode_line_number(e));
		  // in principle:
		//	sdecl.uset += uset;
		//	return sdecl.oset;
		  add_to_uset(sdecl,uset);
		  RETURN get_oset(sdecl);
		} else {
		  aps_warning(e,"using a syntax decl");
		  add_to_uset(sdecl,uset);
		  RETURN get_oset(sdecl);
		}
//		break;
	} // case KEYvalue_use
	case KEYfuncall:
		// * Several cases
		// * 1> X.a (attr_ref)
		// * 2> w.f (field_ref)
		// * 3> local function
		// * 4> primitive function
		// 
	{
	  Declaration fdecl;
	  OSET oset = EMPTY_OSET;
	  
//		printf("	%d: decl is funcall.\n", tnode_line_number(e));
	  // attr ref: X.a 
	  if ((fdecl = attr_ref_p(e)) != NULL) {
	    add_to_uset(fdecl,uset);
	    RETURN get_oset(fdecl);
	  } else if ((fdecl = field_ref_p(e))!= NULL) {
	    // field ref: w.f
	    Expression object = field_ref_object(e);
	    
	    USET newuset = (USET)malloc(sizeof(struct uset));
	    OSET o_w; 
	    newuset->rest = NULL;
	    newuset->u = e;
	    o_w = doOU(object, newuset);

	    /*
	    if (fiber_debug & ALL_FIBERSETS) {
	      printf("Found w.%s with O(W) = ",decl_name(fdecl));
	      print_oset(o_w);
	    }
	    */

          {
	    OSET p; 
	    oset = EMPTY_OSET;
	    for (p = o_w; p != NULL; p = p->rest){
	      Declaration o = p->o;
	      USET u_o = get_uset(o);
	      
	      USET q;	
	      for  (q = u_o; q != NULL; q= q->rest) {
		if ( (EXPR_IS_LHS(q->u)) && same_field(q->u ,newuset->u)) {
		  // f. belongs to u_o
		  Declaration assign = (Declaration)tnode_parent(q->u);
		  add_to_uset(assign,uset);
		  oset = oset_union(oset, get_oset(assign));
		} // if
	      } // for q
	    } // for p
	    RETURN oset;	
	   } 
	  } else if ((fdecl = local_call_p(e)) != NULL) {
	    // local function

	    // shared info stuff: local_call is treated as a child of X0, so
			//   shared_info will go through it.
			// function shared info:
      Declaration fsi = phylum_shared_info_attribute(fdecl, mystate);
			// responsible shared info: shared_info on X0.
      Declaration rsi = responsible_node_shared_info(e, mystate);

      if (rsi) {

			// rsi pass down oset to fsi.
			add_to_oset(fsi, get_oset(rsi));
			// fsi pass up uset to rsi
			add_to_uset(rsi, get_uset(fsi));
      }

      // normal stuff
 	   {
	    Expression arg;
	    Declaration result = some_function_decl_result(fdecl);
	    
	    for (arg = first_Actual(funcall_actuals(e));
					 arg != NULL;
		 			 arg = Expression_info(arg)->next_expr){
				// get decl of formals?
	      
	      Declaration f = function_actual_formal(fdecl,arg,e);
	      add_to_oset(f,doOU(arg,get_uset(f)));
	    }
	    add_to_uset(result,uset);
	    RETURN get_oset(result);
           }
	    
	  } else	{	// primiive: how to get decl? don't need.
	    OSET oset = EMPTY_OSET;
	    Expression arg = first_Actual(funcall_actuals(e));
	    for (; arg!= NULL; arg = Expression_info(arg)->next_expr){
	      oset = oset_union(oset,doOU(arg,uset));
	    }
	    RETURN oset;
	  }
	} 	// case KEYfuncall
  } // switch
}

void *print_all_ou(void *statep, void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration: {
    Declaration decl = (Declaration)node;
    switch (Declaration_KEY(decl))
    {
      case KEYassign:
        return statep;
    }
    if (Declaration_info(decl)->oset != NULL) {
      printf("OSET of node: %s:%d\n", decl_name(decl), tnode_line_number(decl));
      print_oset(Declaration_info(decl)->oset);
    }
    if (Declaration_info(decl)->uset != NULL) {
      printf("USET of node: %s:%d\n", decl_name(decl), tnode_line_number(decl));
      print_uset(Declaration_info(decl)->uset);
    }
    break; 
  }
  
  default:
    break;
  }
  return statep;
}

Declaration first_decl;
static int first=0;
void *test_add_oset(void *statep, void *node){
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration: {
		Declaration decl = (Declaration)node;
		OSET p = (OSET)malloc(sizeof(struct oset));
		p->o = decl;
		p->rest = NULL;

		if (first==0) {
			first_decl = decl;
			first++;
		}
		add_to_oset(first_decl, p);
		printf("the oset after the node: %s\n", decl_name(decl));
		print_oset(Declaration_info(first_decl)->oset);

		break; }

	default: break;
	}
	return statep;
}

// Nodes 0 and 1 are reserved for error detection.
static int FSA_next_node_index = 4;
static const int FSA_default_node = 2;
static NODESET default_node_set = 0;
static NODESET omega = NULL;			// the set of Omiga.
NODESET add_to_nodeset(NODESET, int );

// node's index is from 1.
// identify the decl node
int id_decl_node(Declaration decl) {
  if (Declaration_info(decl)->index > 0)
    return Declaration_info(decl)->index;

 {
  int index = FSA_next_node_index;
  FSA_next_node_index += 2;

  if (fiber_debug & ALL_FIBERSETS) {
    switch (Declaration_KEY(decl))
    {
      case KEYassign:
<<<<<<< HEAD
=======
    printf("%d: index for assign is %d\n",tnode_line_number(decl),
	   index);
>>>>>>> boyland-master
        break;
      default:
    printf("%d: index for %s is %d\n",tnode_line_number(decl),
	   decl_name(decl),index);
<<<<<<< HEAD
     break;
=======
        break;
>>>>>>> boyland-master
    }
  }
  Declaration_info(decl)->index = index;
  omega = add_to_nodeset(omega, index);
  omega = add_to_nodeset(omega, index+1);
  return index;
 }
}

// identify the expr node
int id_expr_node(Expression expr) {
  if (Expression_info(expr)->index > 0)
    return Expression_info(expr)->index;
		// expr node has been identified: 

 {
  int index = FSA_next_node_index;
  FSA_next_node_index += 2;

  if (fiber_debug & ALL_FIBERSETS) {
    printf("%d: index for expression(0x%p) is %d\n",
	   tnode_line_number(expr),
	   expr,
	   index);
    switch (Expression_KEY(expr))
    {
    case KEYfuncall:
      if (attr_ref_p(expr)) printf("a: %s\n", decl_name(attr_ref_p(expr)));
      if (field_ref_p(expr)) printf("f: %s\n", decl_name(field_ref_p(expr)));
      break;
    }
  }
  Expression_info(expr)->index = index;
  return index;
 }
}

EDGES all_fields_list = NULL;

int new_field(Declaration decl, EDGES fields_list){
	EDGES p = fields_list;
	for ( ; p != NULL; p = p->rest) {
		if (p->edge == decl) return 0;
	}
	return 1;
}

// get the all fields of a  phylum to the fields_list
EDGES get_fields(EDGES fields_list, Declaration decl){
  Declaration pdecl = constructor_decl_phylum(decl);
  Declaration fdecl;
  
  printf("DEBUG: phylum: %s; decl: %s\n", decl_name(pdecl),decl_name(decl));
  for (fdecl = NEXT_FIELD(pdecl); fdecl != NULL; 
       fdecl = NEXT_FIELD(fdecl)) {
    // if fdecl is first time met, add it to fields_list.
    
    if (new_field(fdecl, fields_list)) {
      EDGES p = (EDGES)malloc(sizeof(struct edges));
      p->edge = fdecl;
      p->rest = fields_list;
      fields_list = p;
    }
  }
  return fields_list;
}

void print_fields(EDGES fields_list){
	EDGES p;
	for (p = fields_list; p != NULL; p = p->rest)
		printf("	field: %s\n", decl_name(p->edge));
		;
}

// acount the number of nodes needed for FSA and
//  identify the nodes with index number. 
void *count_node(void *u, void *node)
{
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    {
      Declaration decl = (Declaration)node;
      if (get_oset(decl) != NULL || get_uset(decl) != NULL) {
	(void)id_decl_node(decl);
      }

      switch (Declaration_KEY(decl)) {
      default: break;
      case KEYattribute_decl:
	if (FIELD_DECL_P(decl)) {
	  (void)id_decl_node(decl);
	  if (new_field(decl, all_fields_list)) {
	    EDGES p = (EDGES)malloc(sizeof(struct edges));
	    p->edge = decl;
	    p->rest = all_fields_list;
	    all_fields_list = p;
	  }
	}
	break;
      case KEYvalue_decl: 
	if (DECL_IS_SHARED(decl)) {
	  (void)id_decl_node(decl);
	  if (new_field(decl,all_fields_list)) {
	    EDGES p = (EDGES)malloc(sizeof(struct edges));
	    p->edge = decl;
	    p->rest = all_fields_list;
	    all_fields_list = p;
	  }
	}
	break;
      }; // switch Declaration_KEY(decl)
      break;
    }	// case KEYDecl
    
  case KEYExpression:
    {
      Expression e = (Expression)node;
      switch (Expression_KEY(e)) {
      default:  break;
      case KEYvalue_use: {
	Declaration sdecl = USE_DECL(value_use_use(e));
	if (DECL_IS_SHARED(sdecl)) {
	  if (Declaration_KEY(sdecl) == KEYvalue_decl) {
	    Declaration rsi = responsible_node_shared_info(e, mystate);
	    if (rsi != NULL) {
	      // node Qu, Qu-
	      (void)id_expr_node(e);
	    }
	  }
	}
      }; break;
      case KEYfuncall: {
	Declaration fdecl;
	if ((fdecl = field_ref_p(e)) != NULL) { // field ref:w.f
	  // node Qu, Qu-
	  (void)id_expr_node(e);
	}
	break;
      } // case funcall
      } // switch expr
    }	// case KEYExpression
  } 	// switch node
  return u;
}

//
void *compute_OU(void *u, void *node)
{
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    {
      Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
      case KEYtop_level_match: {
	// shared_info
	Declaration lhs = top_level_match_lhs_decl(decl);
	Declaration sattr_l = phylum_shared_info_attribute(node_decl_phylum(lhs),
							   mystate);
	
	Declaration rhs = top_level_match_first_rhs_decl(decl);
	//				printf("after get rhs.\n");
	for(; rhs!= NULL; rhs = next_rhs_decl(rhs)) {
	  Declaration phy = node_decl_phylum(rhs);
	  // phy is null for semantic (terminal) children
	  if (phy) {
	    Declaration sattr_r = phylum_shared_info_attribute(phy, mystate);
	    add_to_oset(sattr_r, get_oset(sattr_l));	// pass down oset
	    add_to_uset(sattr_l, get_uset(sattr_r));	// pass up uset
	  }
	}	// for
	break;
      }	// case top_level_match
      
      // value_decl: includes initilization precess
      case KEYvalue_decl: {
	Default def = value_decl_default(decl) ;
	switch (Default_KEY(def)){
	case KEYno_default: 
	  break;
	case KEYsimple: {
	  Expression expr = simple_value(def);
	  USET uset = get_uset(decl);
	  OSET oset;

	  oset = doOU(expr, uset);
	  add_to_oset(decl, oset);
	  
	  break;}
	case KEYcomposite: {
	  Expression expr = composite_initial(def);
	  OSET oset = doOU(expr, get_uset(decl));
	  add_to_oset(decl, oset);
	  break;}
	}
	break;
      } // value_decl
	
      case KEYassign: {
	// printf("ASSIGN: %d \n", tnode_line_number(node));
	Expression lhs = assign_lhs(decl);
	Expression rhs = assign_rhs(decl);

	add_to_oset(decl, doOU(rhs, get_uset(decl)));
	add_to_uset(decl, doUO(lhs, get_oset(decl)));

	return NULL;
	break;
      }
      case KEYcase_stmt: {
	Match m;
	Expression expr = case_stmt_expr(decl);
	OSET oset = doOU(expr, EMPTY_USET);
	for (m= first_Match(case_stmt_matchers(decl)); m; m = MATCH_NEXT(m)){
	  USET u = doUOp(matcher_pat(m), oset);
	  doOU(expr, u);
	}
	
	//				printf("OSET for case at %d\n",
	//				       tnode_line_number(expr));
	// print_oset(oset);
	break; }
      }; // switch Declaration_KEY(decl)
      break;
    }	// case KEYDecl
    
  case KEYExpression:
    {
      Expression expr = (Expression)node;

      doOU(expr,EMPTY_USET);
      return NULL;
      break;
    }	//KEYExpression
  } 	// switch node
  return u;
}

NODESET nodeset_union(NODESET, NODESET);
NODESET set_of_node(int);

// link_expr_lhs_p: for pattern
NODESET link_expr_lhs_p(Pattern pat, NODESET nodeset){
  if (!local_type_p(infer_pattern_type(pat))) nodeset = default_node_set;
  switch (Pattern_KEY(pat)){
    default: {
        aps_error(pat, "unknown case for doUOp.");
        return EMPTY_NODESET;
        break;}
        
    case KEYno_pattern: {
        return EMPTY_NODESET;
          break;}
      
    case KEYand_pattern: {
        NODESET u1 = link_expr_lhs_p(and_pattern_p1(pat), nodeset);
        NODESET u2 = link_expr_lhs_p(and_pattern_p2(pat), nodeset);
        return nodeset_union(u1, u2);
        break;  }

    case KEYpattern_var: {
      Declaration decl = pattern_var_formal(pat);
      int index = get_node_decl(decl);
      if (index == 0) return EMPTY_NODESET;
      return set_of_node(index+1);  // Qd(-)
      break; }

    case KEYpattern_call: {
        NODESET nset = EMPTY_NODESET;
        Pattern p;
        // assume primitive
        for (p = first_PatternActual(pattern_call_actuals(pat)); p; p = PAT_NEXT(p))
              nset = nodeset_union(nset, link_expr_lhs_p(p, nodeset));
        return nset;
        break;}

    case KEYrest_pattern:{
        return link_expr_lhs_p(rest_pattern_constraint(pat), nodeset);
        break;}

    case KEYcondition:{
        return EMPTY_NODESET;
        break;}
  }  // switch 
}

// build FDA 
NODESET link_expr_rhs(Expression e, NODESET ns);
NODESET link_expr_lhs(Expression e, NODESET ns);

void add_FSA_edge(int, int, Declaration);

// add edges: Qx ---> Qo for o in oset(x)
void add_edges_oset(Declaration decl, Declaration edge){
  int from = Declaration_info(decl)->index;
  OSET oset = Declaration_info(decl)->oset;
  OSET p;
  /// QQQ oset is always empty. ???
  if (oset == NULL) printf("oset for %s is empty.\n", decl_name(decl));
  if (from == 0) fatal_error("%d: No index for %s.",
			     tnode_line_number(decl), decl_name(decl));
  for (p = oset; p!= NULL; p = p->rest){
    add_FSA_edge(from, Declaration_info(p->o)->index, edge);
  }
}

// Qx(-) ----->Qu(-)
void add_edges_uset(Declaration decl, Declaration edge) {
  int from = Declaration_info(decl)->index;
  USET uset = Declaration_info(decl)->uset;
  USET p;
  for (p = uset; p!= NULL; p = p->rest){
    add_FSA_edge(from+1, Expression_info(p->u)->index+1, edge);
  }
}

void *build_FSA(void *vstate, void *node)
{
  STATE *state = (STATE*)vstate;
  if (default_node_set == 0) {
    default_node_set = set_of_node(FSA_default_node);
    omega = add_to_nodeset(omega,FSA_default_node);
    omega = add_to_nodeset(omega,FSA_default_node+1);
  }
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    {   
      Declaration decl = (Declaration)node;
      OSET oset = get_oset(decl);
      USET uset = get_uset(decl);

      /*
      printf("\nChecking:\n");
      dump_lisp_Declaration(decl);
      switch (Declaration_KEY(decl)) {
      case KEYmulti_call:
	aps_error(decl,"Procedure calls not supported.\n");
	return NULL;
      }
      */

      if (oset != NULL) add_edges_oset(decl,NULL);	// Qx-->Qo
      if (uset != NULL) add_edges_uset(decl,NULL);	// Qx(-)-->Qu(-)
      	
      switch (Declaration_KEY(decl)) {
      case KEYmodule_decl:
	if (uset != NULL) {
	  USET q;
	  for (q = uset; q != NULL; q = q->rest) {
	    add_FSA_edge(Declaration_info(decl)->index, //Qo
			 Expression_info(q->u)->index,   //Qu
			 NULL );
	  }
	}
	break;
      case KEYtop_level_match: {
	// shared_info
	Declaration lhs = top_level_match_lhs_decl(decl);
	Declaration sattr_l = phylum_shared_info_attribute(node_decl_phylum(lhs),
							   state);
	
	Declaration rhs = top_level_match_first_rhs_decl(decl);
	//				printf("after get rhs.\n");
	for(; rhs!= NULL; rhs = next_rhs_decl(rhs)) {
	  Declaration phy = node_decl_phylum(rhs);
	  // phy is null for semantic (terminal) children
	  if (phy) {
	    Declaration sattr_r = phylum_shared_info_attribute(phy, mystate);
	    //? NOTHING TO DO ?
	  }
	}	// for
	break;
      }	// case top_level_match
      case KEYvalue_decl: {
	Declaration cdecl, pdecl, fdecl;
	if ((cdecl = object_decl_p(decl)) != NULL) {	// Qo--f->Qf ;  o is in B
	  pdecl = constructor_decl_phylum(cdecl);
	  for (fdecl = NEXT_FIELD(pdecl); fdecl != NULL; fdecl = NEXT_FIELD(fdecl)) {
	    add_FSA_edge(Declaration_info(decl)->index, //Qo
			 Declaration_info(fdecl)->index, //Qf 
			 fdecl);
	  }

	 {
	  USET uset = Declaration_info(decl)->uset;
	  USET q;
	  for (q = uset; q != NULL; q = q->rest) {
	    add_FSA_edge(Declaration_info(decl)->index, //Qo
			 Expression_info(q->u)->index,   //Qu
			 NULL );
	  }
	 }
	} else { // not object
	  Default def = value_decl_default(decl) ;
	  Expression expr = 0;
	  switch (Default_KEY(def)){
	  case KEYno_default: 
	    break;
	  case KEYsimple:
	    expr = simple_value(def);
	    break;
	  case KEYcomposite: 
	    expr = composite_initial(def);
	    break;
	  }
	  if (expr != 0) {
	    NODESET me = set_of_node(get_node_decl(decl)+1);  // Qd(-)
	    (void)link_expr_rhs(expr,me);
	  }
	} // end "else if not object"
	if (DECL_IS_SHARED(decl)) {
	  Declaration mod = state->module;
	  // this edge captures two paths:
	  // From Qo -> Qf and from Qo -> Qu -> Qv
	  // We elide the Qu ndoes for the assignment of globals
	  // at the global level.
	  add_FSA_edge(Declaration_info(mod)->index, // Qo
		       Declaration_info(decl)->index,   // Qf, Qu->Qv
		       decl);
	  // this edge captures the path:
	  // Qx(-) -> Qu(-) -> Qv
	  add_FSA_edge(Declaration_info(mod)->index+1, // Qo(-)=Qx(-)
		       Declaration_info(decl)->index,    // Qu(-)->Qv
		       reverse_field(decl));
	  // this edge forces the global to be computed:
	  // Qo -> Qu -(dot f)-> Qf
	  // (Done by eliding the Qu (fake use))
	  //
	  // But instead of Qf (which can be connected
	  // and cause other things to happen), we use
	  // the default node:
	  add_FSA_edge(Declaration_info(mod)->index, // Qo
		       FSA_default_node,   // Qdefault
		       reverse_field(decl));
	}
	break;
	return NULL;
      }
      
      case KEYassign: {
//      	printf("BUILD_FSA: ASSIGN: %d \n", tnode_line_number(node));
        Expression lhs = assign_lhs(decl);
        Expression rhs = assign_rhs(decl);
  
	if (fiber_debug & ADD_FSA_EDGE) {
		printf("%d: in assign\n",tnode_line_number(node));
	}

        NODESET M = link_expr_lhs(lhs, EMPTY_NODESET);
        NODESET N = link_expr_rhs(rhs, M);
        link_expr_lhs(lhs, N);
  
	if (fiber_debug & ADD_FSA_EDGE) {
		printf("  M=");
		print_nodeset(M);
		printf(", N=");
		print_nodeset(N);
		printf("\n");
	}

        return NULL;
      } // KEYassign
      case KEYcase_stmt: {
        Match m;
        Expression expr = case_stmt_expr(decl);
        NODESET M = link_expr_rhs(expr, EMPTY_NODESET);
        for (m= first_Match(case_stmt_matchers(decl)); m; m = MATCH_NEXT(m)){
          NODESET N = link_expr_lhs_p(matcher_pat(m), M);
          link_expr_rhs(expr, N);
        }
	break;
      }
      } // switch decl
    }	// case KEYDecl
    break;
  case KEYExpression: {
     Expression expr = (Expression)node;
     link_expr_rhs(expr, default_node_set);
     switch (Expression_KEY(expr)) {
     default: break;
     }
     return NULL;
  }
  }	// switch node
  return vstate;
}


//same edge?
int same_edge(Declaration edge1, Declaration edge2) {
	if (edge1 == edge2) 
		return 1;
	else
		return 0;
}

// edge is in the edgeset?
int edge_in_set(Declaration edge, EDGES edges){
	EDGES eds;
	if (edges == NULL) return 0;
	if (same_edge(edge, edges->edge)) return 1;
	return edge_in_set(edge, edges->rest);
}

// add one edge b/w from and to nodes.
void add_FSA_edge(int from, int to, Declaration edge) {
  // check first if the edge has been in the tnode.
  // if not, add it, otherwise do nothing.
  static int count = 0;
  EDGES edgeset = FSA_graph[to];

  if ((from <= 1) ||
      (to <= 1)) {
    fatal_error("adding edges for an unidentified edge.");
  }
  if (fiber_debug & PUSH_FIBER) {
    printf("add one edge b/w %d %d (%s)\n", from, to,
	   edge ? decl_name(edge) : "<epsilon>");
  }
  for (edgeset = FSA_graph[to]; edgeset; edgeset=edgeset->rest) {
    if (edgeset->from == from && edgeset->edge == edge) {
      if (fiber_debug & PUSH_FIBER) {
	printf("	edge already exist.\n");
      }
      return;
    }
  }
  edgeset = (EDGES)malloc(sizeof(struct edges));
  edgeset->rest = FSA_graph[to];
  edgeset->edge = edge;
  edgeset->from = from;
  FSA_graph[to] = edgeset;
  done = FALSE;
  if (fiber_debug & PUSH_FIBER) {
    printf("	new edge (#%d) is added.\n",++count);
  }
}

// union of two nodesets. ns1 = ns1 + ns2
NODESET nodeset_union(NODESET nodeset1, NODESET nodeset2) {
	NODESET ns1, ns2;

	for (ns2 = nodeset2; ns2; ns2 = ns2->rest)	{
		for (ns1 = nodeset1; ns1; ns1 = ns1->rest) {
			if (ns2->node == ns1->node) break;	// node exist
		}
		if (ns1 == NULL) { // ns2 is a new node
			NODESET newnode = (NODESET)malloc(sizeof(struct nodeset));
			newnode->node = ns2->node;
			newnode->rest = nodeset1;
			nodeset1 = newnode;
		}
	}	// for ns2
	return nodeset1;
}

// get the node index from the decl
int get_node_decl(Declaration decl){
  int index = Declaration_info(decl)->index;
  if (index < 0 || index > 2000) {
    aps_error(decl,"Warning: FSA index for %s is bogus: %d\n",
	      decl_name(decl),index);
  }
  return index;
};

// get the node index from the expression
int get_node_expr(Expression expr){
	return Expression_info(expr)->index;
}

// construct the nodeset with only one node.
NODESET set_of_node(int node) {
  NODESET ns = (NODESET)malloc(sizeof(struct nodeset));
  if (node == 0) node = FSA_default_node;
  if (node == 1) node = FSA_default_node+1;
  ns->rest = NULL;
  ns->node = node;
  return ns;
}

// if i in the nodeset
int node_in_set(NODESET ns, int i){
	NODESET p = ns;
	while (p) {
		if (p->node == i)
			return 1;
		p = p->rest;
	}
	return 0;
}

void print_nodeset(NODESET ns){
	NODESET p = ns;
	while (p) {
		printf("%d", p->node);
		p = p->rest;
		if (p) printf(", ");
	}
}

// add new node to the head of the node list.
NODESET add_to_nodelist(NODESET list, int node){
	NODESET newlist = (NODESET)malloc(sizeof(struct nodeset));
	newlist->rest = list;
	newlist->node = node;
	return newlist;
}

// add the node (represented as an int ) into the nodeset.
NODESET add_to_nodeset(NODESET ns, int node){
  if ((ns!=NULL) && (ns->node == node))
    return ns;
  {
    NODESET newset = (NODESET)malloc(sizeof(struct nodeset));
    newset->rest = ns;
    newset->node = node;
    
    return newset;
  }
}
// Given oset, get a nodeset = {Qb | b in oset}
NODESET oset_to_nodeset(OSET oset) {
	OSET p= oset;
	NODESET ns = NULL;
	while (p) {
		ns = add_to_nodeset(ns,get_node_decl(p->o));	
		p = p->rest;
	}
	return ns;
}

// Given uset, get a nodeset = {Qb | b in uset }
NODESET uset_to_nodeset(USET uset) {
	USET p = uset;
	NODESET ns = NULL;
	while (p) {
		ns = add_to_nodeset(ns, get_node_expr(p->u));
		p = p->rest;
	}
	return ns;
}

NODESET link_expr_rhs(Expression e, NODESET ns){
  if (e==NULL) return EMPTY_NODESET;
  if (!local_type_p(infer_expr_type(e))) ns = default_node_set;
  switch (Expression_KEY(e)){
  default: aps_error(e, "wrong: not a proper expression to add edges.");
    return EMPTY_NODESET;
  case KEYinteger_const:
  case KEYreal_const:
  case KEYstring_const:
  case KEYchar_const:
    return default_node_set;
    
  case KEYrepeat:
          RETURN link_expr_rhs(repeat_expr(e),ns);
	  
  case KEYvalue_use: {
    Declaration decl = USE_DECL(value_use_use(e));
    if (DECL_IS_LOCAL(decl)) {
      if (DECL_IS_SHARED(decl)  &&
	  responsible_node_declaration(e) != NULL) {
	// use of a global value in a rule (need to use shared_info)
	
	NODESET n;
	for (n=ns; n; n = n->rest) {
	  add_FSA_edge(get_node_expr(e)+1, n->node,decl);
	  add_FSA_edge(get_node_expr(e), n->node, reverse_field(decl));
	  // Qe(-) to n: bar node is even number.
	  // the edge is reverse_field(fdecl);
	} // for end
	
	{ 
    USET eu = get_uset(e);
	  OSET oset = doOU(e, eu);
	  return oset_to_nodeset(oset);
	}
	
      } else {
	return set_of_node(get_node_decl(decl)); // {Qd}
      }
    } // else if not local, we can ignore
  } // case value_use
  break;
  
	case KEYfuncall: {
	  Declaration fdecl;
	  
	  if ((fdecl = attr_ref_p(e)) != NULL) { // attr ref: X.a
				// same as value_use
	    return set_of_node(get_node_decl(fdecl)); //{Qd}
	  } else if ((fdecl = field_ref_p(e)) != NULL) { //field ref: w.f
	    NODESET n;
	    if (fiber_debug & ADD_FSA_EDGE) printf("Starting to add edges for field ref of %s\n",decl_name(fdecl));
	    for (n=ns; n; n = n->rest) {
	      add_FSA_edge(get_node_expr(e)+1, n->node, fdecl);
	      add_FSA_edge(get_node_expr(e), n->node, reverse_field(fdecl));
	      // Qe(-) to n: bar node is even number.
	      // the edge is reverse_field(fdecl);
	    } // for end
	    {
	      Expression object = field_ref_object(e);
	      link_expr_rhs(object, set_of_node(get_node_expr(e)+1));	// Qe(-)
	      {
<<<<<<< HEAD
    USET eu = get_uset(e);
		OSET oset = doOU(e, eu);
=======
		OSET oset = doOU(e, EMPTY_USET);
	    if (fiber_debug & ADD_FSA_EDGE) printf("ending to add edges for field ref of %s\n",decl_name(fdecl));
>>>>>>> boyland-master
		return oset_to_nodeset(oset);
	      }
	    }
	    
	  } else if ((fdecl = local_call_p(e)) != NULL) { // local call
			//! need to deal with local call
      Declaration fsi = phylum_shared_info_attribute(fdecl, mystate);
      // responsible shared info: shared_info on X0.
      Declaration rsi = responsible_node_shared_info(e, mystate);

      if (rsi) {
			//!! deal with shared info here??
			NODESET ns = set_of_node(get_node_decl(rsi));
      }

		// normal stuff
	    Expression arg;
	    Declaration result = some_function_decl_result(fdecl);
	    
	    for (arg = first_Actual(funcall_actuals(e));
		 arg != NULL;
		 arg = Expression_info(arg)->next_expr){
	      
				// get decl of formal:
	      Declaration f = function_actual_formal(fdecl,arg,e);
				// construct the set of node Qf(-)
	      NODESET nodeset_f = set_of_node(get_node_decl(f)+1); // {Qf(-)}
	      link_expr_rhs(arg, nodeset_f);
	    }  // for
	    return set_of_node(get_node_decl(result)); // {Qresult}
	    
	  } else { // primitive call

	    NODESET nodeset = EMPTY_NODESET;
	    Expression arg = first_Actual(funcall_actuals(e));
	    for (; arg; arg = Expression_info(arg)->next_expr){
	      nodeset = nodeset_union(nodeset, link_expr_rhs(arg, ns));
	    }
	    return nodeset;
	  }
	  break;
	} // KEYfuncall
	} // switch
  return default_node_set;
}

// edge process to the left hand side of assignment.
NODESET link_expr_lhs(Expression e, NODESET ns) {
  if (e==NULL) return EMPTY_NODESET;
  if (!local_type_p(infer_expr_type(e))) ns = default_node_set;
  switch (Expression_KEY(e)){
  default: aps_error(e, "wrong: not a proper expression to add edges.");
    return EMPTY_NODESET;
  case KEYrepeat:
    return link_expr_lhs(repeat_expr(e),ns);
  case KEYvalue_use: {
    Declaration decl = USE_DECL(value_use_use(e));
    if (DECL_IS_LOCAL(decl) &&
	DECL_IS_SHARED(decl)  &&
	responsible_node_declaration(e) != NULL) {
      // use of a global value in a rule (need to use shared_info)

      NODESET n;
      for (n=ns; n; n = n->rest) {
	// Qe(-)----f.--->n
	add_FSA_edge(get_node_expr(e)+1, n->node, reverse_field(decl));
	add_FSA_edge(get_node_expr(e), n->node, decl);
	// Qe(-) to n: bar node is even number.
	// the edge is reverse_field(fdecl);
      } // for end
      {
	USET uset = doUO(e, EMPTY_OSET);
	return uset_to_nodeset(uset);
      }

    } //if 
    return set_of_node(get_node_decl(decl)+1);  // Qd(-)
    break;
  } // case value_use
	case KEYfuncall: {
	  Declaration fdecl;
	  if ((fdecl = attr_ref_p(e)) != NULL) { // attr ref: X.a
				// same as value_use
	    return set_of_node(get_node_decl(fdecl)+1); //{Qd(-)}
	  } else if ((fdecl = field_ref_p(e)) != NULL) { //field ref: w.f
	    NODESET n;
	    for (n=ns; n; n = n->rest) {
	      // Qe(-)----f.--->n
	      add_FSA_edge(get_node_expr(e)+1, n->node, reverse_field(fdecl));
	      add_FSA_edge(get_node_expr(e), n->node, fdecl);
	      // Qe(-) to n: bar node is even number.
	      // the edge is reverse_field(fdecl);
	    } // for end
	    {
	      Expression object = field_ref_object(e);
	      link_expr_lhs(object, set_of_node(get_node_expr(e)+1) );	// Qe(-)
	      {
    USET eu = get_uset(e);
		USET uset = doUO(e, eu);
		// printf("DEBUG: after doUO in link_expr.\n");
		return uset_to_nodeset(uset);
	      }
	    }
	  } // if end
	  break;
	}	// KEYfuncall
	} // switch
}

static int edge_num = 0;
// print edges
void print_edges(EDGES edges, int to){
  EDGES p = edges;
  while (p) {
    printf("    edge: %d ---- %d : %s\n", p->from, to,
	   p->edge ? decl_name(p->edge) : "<epsilon>");
    p = p->rest;
    edge_num++;
  }
}


void print_FSA(){
	int from, to;
	int max = FSA_next_node_index;
	for (to = 0; to < max; to++){
	  //	printf(" edges from node %d to node %d:\n", from, to);
	  print_edges(FSA_graph[to], to);	
	}
}

NODESET beclose(NODESET N){
  NODESET p = N;
  NODESET head = p;
  NODESET tail = NULL;
  BOOL done;
  do {
    NODESET next_tail = head;
    done = TRUE;
    for (p=head; p != tail; p=p->rest) {
      int node = p->node;
      EDGES es;
      for (es = FSA_graph[node]; es; es=es->rest) {
	int from = es->from;
	if (es->edge == NULL && !node_in_set(head,from)) { // an epsilon edge
	  head = add_to_nodeset(head,from);
	  done = FALSE;
	}
      }
    }
    tail = next_tail;
  } while (!done);
  return head;
}

int nodeset_hash(NODESET p)
{
  int h = 0;
  while (p != NULL) {
    h += p->node;
    p = p->rest;
  }
  return h;
}

NODE_IN_TREE makeNode(NODE_IN_TREE T, Declaration f, BOOL reverse) {
  NODESET result = NULL;
  NODE_IN_TREE ret = NULL;
  NODESET p;
  
  if (T == NULL) return NULL;
  
  for (p = T->ns; p; p=p->rest) {
    EDGES ns;
    for (ns = FSA_graph[p->node]; ns != NULL; ns=ns->rest) {
      if (ns->edge == f && !node_in_set(result,ns->from)) {
	result = add_to_nodeset(result, ns->from);
      }
    }
  }	
  if (result == NULL) return NULL;

  ret = (NODE_IN_TREE)malloc(sizeof(struct node_in_tree));
  ret->ns = result;
  if ((T->dir == FORWARD_) == !reverse)
    ret->dir = FORWARD_;
  else
    ret->dir = BACKWARD_;
  ret->hash = nodeset_hash(result);

  (ret->as_fiber).field = f;
  (ret->as_fiber).shorter = &(T->as_fiber); // create shorter of fiber

  return ret;
}

// if two DFA node are same: have the same number and node states
int same_DFA_node(NODE_IN_TREE t1, NODE_IN_TREE t2){
  if (t1 == NULL || t2 == NULL) return 0;	// either empty
  if (t1->dir != t2->dir) return 0;
  if (t1->hash != t2->hash) return 0;

  {
    NODESET ns1 = t1->ns;
    NODESET ns2 = t2->ns;
    
    if (ns1 == NULL || ns2 == NULL) return 0;
    {
      NODESET p = ns1;
      while (p) {
	if (node_in_set(ns2, p->node)) 
	  p = p->rest;
	else return 0;
      }
      p = ns2;
      while(p) {
	if (node_in_set(ns1, p->node)) 
	  p = p->rest;
	else return 0;
      }
      return 1;
    }
  }
}

// look for the same node from created node list: DFA[]
NODE_IN_TREE get_same(NODE_IN_TREE t){
	int i;
	/*
	for (i=1; i<= DFA_node_number; i++) {
		if (same_DFA_node(t, DFA[i])) return DFA[i];
	}
	*/
	NODE_IN_TREE t1 = t;
        do {
	  t1 = (NODE_IN_TREE)t1->as_fiber.shorter;
        } while (t1 != NULL && !same_DFA_node(t,t1));
	return t1;
}

// whether t is in worklist wl?
int in_worklist(NODE_IN_TREE t, WORKLIST wl){
  if (t == NULL) return 1;	// empty is always in worklist; 
  // then in makeDFA(), we will do nothing.
  if (wl == NULL) return 0;	// worklist is empty; 

  {
    WORKLIST p = wl;
    while (p) {
      if (same_DFA_node(t, p->tree_node)) return 1;
      p = p->rest;
    }
    return 0;
  }
}

void print_DFA_node(int);

// add the DFA tree node to the worklist
WORKLIST add_to_DFA_worklist(WORKLIST wl, NODE_IN_TREE t) {
  if (t== NULL) return wl;		// never add empty node to worklist
  if (fiber_debug & ADD_FIBER) {
    //printf("Adding %d to worklist:\n",t->index);
    printf("#%d: ",t->index);
    print_fiber(&t->as_fiber,stdout);
    printf("\n");
  }
  {
    WORKLIST new = (WORKLIST)malloc(sizeof(struct worklist));
    new->tree_node = t;
    new->rest = wl;
    return new;	
  }
}

/* Return true is this state is in the fiber set
 * for some declaration or expression.
 */
BOOL is_reachable(NODE_IN_TREE n)
{
  NODESET ns, ns2;
  for (ns = n->ns; ns; ns=ns->rest) {
    int i = ns->node & ~1;
    for (ns2 = ns->rest; ns2; ns2=ns2->rest) {
      if ((ns2->node&~1) == i) return TRUE;
    }
  }
  return FALSE;
}

NODE_IN_TREE check_transition(NODE_IN_TREE to, Declaration trans,
			      NODE_IN_TREE from)
{
  if (to != NULL) {
    to->ns = beclose(to->ns);

    {    
      NODE_IN_TREE old = get_same(to);	
      if (old != NULL) {		// same node already exist. 
	DFA_tree[old->index][from->index] = trans;
      }
      else if (is_reachable(to)) { // this is a new node
	// create one node for DFA
	if (++DFA_node_number >= MAX_DFA_NUMBER) {
	  fatal_error("Cannot handle more than %d DFA nodes!",
		      MAX_DFA_NUMBER);
	}
	//printf("debug: create one node: %d\n", DFA_node_number);
	to->index = DFA_node_number;
	(to->as_fiber).field = trans;
	(to->as_fiber).shorter = &(from->as_fiber); // create shorter of fiber
	DFA[DFA_node_number] = to;
	DFA_tree[to->index][from->index] = trans;
	return to;
      };
    };
  }
  return NULL;
}

void makeDFA(){
  NODE_IN_TREE T;	// first member in worklist
  int dir;		// direction of the node: FORWARD or BACKWARD
  wl = (WORKLIST)malloc(sizeof(struct worklist));	// init worklist;
  T = &base_DFA_node;
  T->ns = beclose(omega);
  T->dir = FORWARD_;		// initial direction
  T->hash = nodeset_hash(T->ns);
  wl->rest = NULL;
  wl->tree_node = T;
  
  DFA[1] = T;
  T->index = 1;
  
  while (wl !=  NULL) {
    //		printf("debug: working on worklist\n");
    EDGES e = all_fields_list;
    T = wl->tree_node;
    wl = wl->rest;	// remove the first node
    for (; e != NULL; e = e->rest){
      Declaration f = e->edge;
      NODE_IN_TREE Nf = makeNode(T, f, 0);
      NODE_IN_TREE Nfd = makeNode(T, reverse_field(f), 1);

      wl = add_to_DFA_worklist(wl,check_transition(Nf,f,T));
      wl = add_to_DFA_worklist(wl,check_transition(Nfd,reverse_field(f),T));
    }	// for any f in F
  }		// while worklist not empty
}

// print the all FSA nodes in one DFA node
void print_DFA_node(int i){
	NODESET ns = DFA[i]->ns;
	int dir = DFA[i]->dir;

	printf("# %d: ",i);

	while (ns) {
		printf("%d,", ns->node);
		ns = ns->rest;
	}
	if (dir == FORWARD_) printf("FORWARD");
	else printf("BACKWARD");
	printf("\n");
}

void print_DFA_all_nodes(){
	int i;
	printf("DFA total node number is: %d\n", DFA_node_number);
	for (i=1; i<= DFA_node_number; i++) {
		print_DFA_node(i);
	}
}

// print the nodes of DFA and the edge b/w nodes
void print_DFA(){
	int i,j;
	for (i= 1; i<= DFA_node_number; i++) {
		for (j=1; j<= DFA_node_number; j++) {
			if (DFA_tree[j][i] != NULL) { 
				printf("the edge b/w node: #%d ", i);
//			print_DFA_node(i);
				printf(" to node: #%d ", j);
//			print_DFA_node(j);
				printf(" is : %s\n", decl_name(DFA_tree[j][i]));
			}
		}
	}
}
// add a fiber to fiber set
FIBERSET DFA_add_to_fiberset(FIBERSET old, struct fiber* fb, void* tnode) {
	FIBERSET new = (FIBERSET)malloc(sizeof(struct fiberset));

	new->rest = old;
	new->fiber = fb;
	new->tnode = tnode;

	return new;
}


// get F(x)^F(x)/bar = { T| Qx in T && Qx(-) in T && T in DFA tree}
void *DFA_fiber_set(void *u, void *node)
{
  STATE *state = (STATE *)u;
  
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    {
      Declaration decl = (Declaration)node;
      FIBERSETS*  decl_fsets;
      int i;
      
      // for any x
      int index = Declaration_info(decl)->index ;
      if (index == 0) break;		// not the interested x
      
      // init NORMAL and REVERSE of FINAL fibersets here
      decl_fsets = &(Declaration_info(decl)->decl_fibersets);
      
      decl_fsets->set[FIBERSET_NORMAL_FINAL] = NULL;
      decl_fsets->set[FIBERSET_REVERSE_FINAL] = NULL;
      
      if (fiber_debug & FIBER_FINAL) {
	printf("fiber set for ");
	switch (Declaration_KEY(decl)) {
	case KEYassign:
	  printf("assignment on line %d",tnode_line_number(decl));
	  break;
	default:
	  puts(decl_name(decl));
	  break;
	}
	printf(" /%d is ",Declaration_info(decl)->index);
      }
      for (i= 2; i<= DFA_node_number; i++) {  // for any DFA tree node
	// fiberset doesn't include base fiber
        if (node_in_set(DFA[i]->ns, index) &&
            node_in_set(DFA[i]->ns, index+1) ) {
          if (fiber_debug & FIBER_FINAL) printf(" %d,", i);
	  if (!node_in_set(DFA_result_set, i))
	    DFA_result_set = add_to_nodeset(DFA_result_set, i);
	  
	  // add to fiber set
	  if (DFA[i]->dir == FORWARD_) {	// forward fiber
	    decl_fsets->set[FIBERSET_NORMAL_FINAL] = DFA_add_to_fiberset(
									 decl_fsets->set[FIBERSET_NORMAL_FINAL], 
									 &(DFA[i]->as_fiber), node);
	  }
	  else		{ 										// reverse fiber
	    decl_fsets->set[FIBERSET_REVERSE_FINAL] = DFA_add_to_fiberset(
									  decl_fsets->set[FIBERSET_REVERSE_FINAL], 
									  &(DFA[i]->as_fiber), node);
	  }
	}
      }
      if (fiber_debug & FIBER_FINAL) printf("\n");
      break;
    }
  case KEYExpression: {
    return NULL;
  }
  } // switch node
  return u;
}

int buff_size = 40960;
int max_field_len = 64;

// according to DFA_result_set, print all possible fibers
void print_DFA_path(){
  NODESET p = DFA_result_set;
  while (p){
    printf("Paths from node #%d: ",p->node);
    print_fiber(&(DFA[p->node]->as_fiber),stdout);
    printf(":\n");
    {
      Declaration self = DFA_tree[p->node][p->node];
      int i;
      for (i = 0; i <= DFA_node_number; ++i) {
	Declaration f = DFA_tree[p->node][i];
	if (f != 0) {
	  if (f == self) {
	    if (i != p->node) {
	      printf("  %s+.#%d\n",decl_name(f),i);
	    }
	  } else {
	    printf("  %s.#%d\n",decl_name(f),i);
	  }
	}
      }
    }
    /*print_DFA_path_rec(p->node);*/
    p = p->rest;
    printf("\n");
   } //while
}

// 

int circle_check_rec(NODESET old, int node){
	int i;
	NODESET new;
	int terminal = 1;
	for (i=1; i<= DFA_node_number; i++){
		if (i != node) {
			if (DFA_tree[node][i] != NULL) {
				if (node_in_set(old, i)) return 1;

				terminal = 0;
				new = add_to_nodeset(old, i);
				return circle_check_rec(new, i);
			}
		}
	}
	if (terminal == 1) return 0;
}

int circle_check(){
	NODESET p = DFA_result_set;
	int circle =0;
	while (p) {
	  //printf("If exist a circle from node #%d: ",p->node);
		int i = circle_check_rec(NULL,p->node);
		circle = circle + i;
		//printf("%d\n", i);
		p = p->rest;
	}
	return circle;
}

// set (T->as_fiber).longer for any T belong to DFA nodes.
void longer(){
	int i,j;
	ALIST p;
	for (i=1; i <= DFA_node_number; i++) {
		p = NULL;
		for (j=1; j<= DFA_node_number; j++) {
			if (DFA_tree[j][i]) { 
					p = acons(DFA_tree[j][i], DFA[j], p);
			}
		}
		(DFA[i]->as_fiber).longer = p;
	}
}

/**/
void fiber_module(Declaration module, STATE *s) {
  /* initialise FIELD_DECL_P and DECL_IS_SHARED */
  init_field_decls(module,s);
  /* initialize EXPR_IS_RHS and EXPR_IS_LHS */
  traverse_Declaration(init_rhs_lhs,s,module);
  /* set up initial fiber sets */

	mystate = s;
	
  traverse_Declaration(preinitialize_fibersets,s,module);
//  traverse_Declaration(initialize_fibersets,s,module);

  // initialize the O set of the start phylum's shared_info
  {
    OSET p = newblock(struct oset);
    p->o = module;
    p->rest = NULL;
    add_to_oset(module,p);
    {
      Declaration spsi = phylum_shared_info_attribute(s->start_phylum,s);
      add_to_oset(spsi,p);

      /* computing the O and U set */ 
      // printf("new fiber program begin.\n");
      
      // testing the function: test_add_oset()
      // printf("testing add_to_oset.......\n");
      // traverse_Declaration(test_add_oset, module, module);
      
      done = FALSE;
      while (!done) {
	done = TRUE;
	traverse_Declaration(compute_OU, module, module);
	add_to_uset(module,get_uset(spsi));
      }
      
      // print all ou set of any Decl node.
      if (fiber_debug & ALL_FIBERSETS) {
	traverse_Declaration(print_all_ou, module, module);
	
	// print OU of shared_info
	printf("starting print OU set of shared_info.\n");
	{
	  Declaration instance =
	    POLYMORPHIC_INSTANCES(MODULE_SHARED_INFO_POLYMORPHIC(module));
	  while (instance != NULL) {
	    Declaration si = list_Declarations_elem(block_body(polymorphic_body(instance)));
	    // printf("For %s\n",decl_name(si));
	    print_all_ou(NULL, si);	
	    instance = DECL_NEXT(instance);
	  }
	}
      }
    }
  }

	// count the nodes for FSA and flag them on tree-nodes.
	traverse_Declaration(count_node, module, module);
	{
	  Declaration instance =
	    POLYMORPHIC_INSTANCES(MODULE_SHARED_INFO_POLYMORPHIC(module));
	  while (instance != NULL) {
	    Declaration si = list_Declarations_elem(block_body(polymorphic_body(instance)));
	    // printf("For %s\n",decl_name(si));
	    count_node(si, si);	
	    instance = DECL_NEXT(instance);
	  }
	}

	if (fiber_debug & ALL_FIBERSETS) {
	  printf("\nFSA: the number of nodes created in FSA: %d\n\n",
		 FSA_next_node_index);
	  printf("Fields_list is :\n");
	  print_fields(all_fields_list);
	}

	// initionlize the graph 
//	FSA_graph = (EDGES)malloc(sizeof(struct edges)*index);

	// printf("DEBUG: traverse declaration to build FSA.\n");
	traverse_Declaration(build_FSA, s, module);
	{
	  Declaration instance =
	    POLYMORPHIC_INSTANCES(MODULE_SHARED_INFO_POLYMORPHIC(module));
	  while (instance != NULL) {
	    Declaration si = list_Declarations_elem(block_body(polymorphic_body(instance)));
	    build_FSA(s, si);	
	    instance = DECL_NEXT(instance);
	  }
	}

	//printf("DEBUG: after traverse build_FSA.\n");

	if (fiber_debug & ALL_FIBERSETS) {
		printf("\nFSA is built: the graph is: \n");
		print_FSA();
		printf("the total edges is :%d (density = %g)\n", edge_num,
		       ((double)edge_num)/(FSA_next_node_index*FSA_next_node_index));
	}

//	printf("the nodeset of OMIGA is:\n");
//	print_nodeset(omega);

// build DFA

	// printf("now making DFA.\n");
	makeDFA();

	if (fiber_debug & ALL_FIBERSETS) {
		print_DFA();
	}

	if (fiber_debug & ALL_FIBERSETS) {
		printf("\nDFA nodes details: \n");
	  print_DFA_all_nodes();
	  printf("\nDFA: the number of node created: %d\n\n", DFA_node_number);
	}

	traverse_Declaration(DFA_fiber_set, module, module);
	{
	  Declaration instance =
	    POLYMORPHIC_INSTANCES(MODULE_SHARED_INFO_POLYMORPHIC(module));
	  while (instance != NULL) {
	    Declaration si = list_Declarations_elem(block_body(polymorphic_body(instance)));
	    // printf("For %s\n",decl_name(si));
	    DFA_fiber_set(NULL, si);	
	    instance = DECL_NEXT(instance);
	  }
	}

	if (fiber_debug & ALL_FIBERSETS) {
	  printf("\n DFA result set: ");
	  print_nodeset(DFA_result_set);
	  printf("\n");
	}

// establish Fiber's longer for any node in DFA tree.
	longer();

// print DFA all possible path
	if (fiber_debug & FIBER_FINAL) {
		if (circle_check() == 0){
			print_DFA_path();
		}
	}

// end of summer 2003

	return;

  /* go through fiber worklist */
  { FIBERSET fs;
    for (;;) {
      fs = get_next_fiber();
      if (fs == NULL) break;
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
    {
      Declaration decl = (Declaration)tnode;
      switch (Declaration_KEY(decl)) {
      case KEYassign:
	printf(" of assignment on line %d",tnode_line_number(decl));
	break;
      default:
	printf(" of %s",decl_name(decl));
	break;
      }
    }
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


