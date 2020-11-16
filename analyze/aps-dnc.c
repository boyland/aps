/* Testing DNC with fibering of conditional attribute grammars
 * written in APS syntax.  First we initialize the graphs for
 * every phylum and for every production.  Then we interate
 * computing summary graphs and then augmented depedency graphs
 * for productions until we reach a fixed point.
 */
#include <stdio.h>
#include "jbb-alloc.h"
#include "aps-ag.h"

#include "aps-tree-dump.h"

int analysis_debug = 0;


/*** FUNCTIONS FOR INSTANCES */

BOOL fibered_attr_equal(FIBERED_ATTRIBUTE *fa1, FIBERED_ATTRIBUTE *fa2) {
  return fa1->attr == fa2->attr && fa1->fiber == fa2->fiber;
}

enum instance_direction fibered_attr_direction(FIBERED_ATTRIBUTE *fa) {
  if (ATTR_DECL_IS_SYN(fa->attr)) {
    if (fiber_is_reverse(fa->fiber)) {
      return instance_inward;
    } else {
      return instance_outward;
    }
  } else if (ATTR_DECL_IS_INH(fa->attr)) {
    if (fiber_is_reverse(fa->fiber)) {
      return instance_outward;
    } else {
      return instance_inward;
    }
  } else {
    return instance_local;
  }
}

enum instance_direction invert_direction(enum instance_direction dir) {
  switch (dir) {
  case instance_inward: return instance_outward;
  case instance_outward: return instance_inward;
  case instance_local: return instance_local;
  }
  fatal_error("control flow fell through invert_direction");
}

enum instance_direction instance_direction(INSTANCE *i) {
  enum instance_direction dir = fibered_attr_direction(&i->fibered_attr);
  if (i->node == NULL) {
    return dir;
  } else if (DECL_IS_LHS(i->node)) {
    return dir;
  } else if (DECL_IS_RHS(i->node)) {
    return invert_direction(dir);
  } else if (DECL_IS_LOCAL(i->node)) {
    return instance_local;
  } else {
    fatal_error("%d: unknown attributed node",tnode_line_number(i->node));
  }
}


/*** FUNCTIONS FOR EDGES ***/

DEPENDENCY dependency_join(DEPENDENCY k1, DEPENDENCY k2)
{
  return k1 | k2;
}

DEPENDENCY dependency_trans(DEPENDENCY k1, DEPENDENCY k2)
{
  // complicated because fiber trans non-fiber = non-fiber
  // but maybe-carrying trans non-carrying = non-carrying
  // and trans never gives direct.
  if (k1 == no_dependency || k2 == no_dependency) return no_dependency;
  return (((k1 & DEPENDENCY_NOT_JUST_FIBER)|
	   (k2 & DEPENDENCY_NOT_JUST_FIBER)) |
	  ((k1 & DEPENDENCY_MAYBE_CARRYING)&
	   (k2 & DEPENDENCY_MAYBE_CARRYING)) |
    ((k1 & DEPENDENCY_MAYBE_SIMPLE)&
	   (k2 & DEPENDENCY_MAYBE_SIMPLE)) | 
	  SOME_DEPENDENCY);
}

DEPENDENCY dependency_indirect(DEPENDENCY k)
{
  return k&~DEPENDENCY_MAYBE_DIRECT;
}

int worklist_length(AUG_GRAPH *aug_graph) {
  int i = 0;
  EDGESET e;
  for (e = aug_graph->worklist_head; e != NULL; e = e->next_in_edge_worklist) {
    if (e->next_in_edge_worklist == NULL && e != aug_graph->worklist_tail)
      fatal_error("Worklist structure corrupted");
    ++i;
  }
  return i;
}

void check_two_edge_cycle(EDGESET new_edge, AUG_GRAPH *aug_graph);

void add_to_worklist(EDGESET node, AUG_GRAPH *aug_graph) {
  /* First check if
   * (1) we actually have an aug_graph
   * (2) the edge isn't already in the list
   *     in which case it points to another edge or is the tail
   */
  if (aug_graph != NULL &&
      node->next_in_edge_worklist == NULL &&
      aug_graph->worklist_tail != node) {
    if (analysis_debug & WORKLIST_CHANGES) {
      printf("Worklist gets %d ",worklist_length(aug_graph)+1);
      print_edgeset(node,stdout);
    }
    if (aug_graph->worklist_tail == NULL) {
      if (aug_graph->worklist_head != NULL)
	fatal_error("Worklist head is wrong!");
      aug_graph->worklist_head = node;
    } else {
      if (aug_graph->worklist_head == NULL)
	fatal_error("Worklist head is NULL");
      aug_graph->worklist_tail->next_in_edge_worklist = node;
    }
    aug_graph->worklist_tail = node;
    if (analysis_debug & TWO_EDGE_CYCLE) {
      check_two_edge_cycle(node, aug_graph);
    }
  }
}

/** Remove an edgeset from the worklist if it is in it.
 * There are multiple cases: <ul>
 * <li> The element is at the head of the list.
 * <li> The element is at the tail of a list.
 * <li> The element is in the middle somewhere.
 * <li> The element is not in the worklist. </ul>
 */
void remove_from_worklist(EDGESET node, AUG_GRAPH *aug_graph) {
  if (node->next_in_edge_worklist != NULL) {
    /* hard work */
    EDGESET *ep = &aug_graph->worklist_head;
    while (*ep != node) {
      if (*ep == NULL) fatal_error("Not in worklist!");
      ep = &(*ep)->next_in_edge_worklist;
    }
    *ep = node->next_in_edge_worklist;
  } else if (node == aug_graph->worklist_tail) {
    if (node == aug_graph->worklist_head) {
      aug_graph->worklist_head = NULL;
      aug_graph->worklist_tail = NULL;
    } else {
      EDGESET e = aug_graph->worklist_head;
      while (e->next_in_edge_worklist != node) {
	if (e->next_in_edge_worklist == NULL)
	  fatal_error("Worklist in two pieces");
	e = e->next_in_edge_worklist;
      }
      e->next_in_edge_worklist = NULL;
      aug_graph->worklist_tail = e;
    }
  }
  node->kind = no_dependency;
}

static EDGESET edgeset_freelist = NULL;
EDGESET new_edgeset(INSTANCE *source,
		    INSTANCE *sink,
		    CONDITION *cond,
		    DEPENDENCY kind) {
  EDGESET new_edge;
  if (edgeset_freelist == NULL) {
    new_edge = (EDGESET)HALLOC(sizeof(struct edgeset));
  } else {
    new_edge = edgeset_freelist;
    edgeset_freelist = edgeset_freelist->rest;
    if (new_edge->kind != no_dependency)
      fatal_error("edgeset freelist corruption! (1)");
  }
  new_edge->rest = NULL;
  new_edge->source = source;
  new_edge->sink = sink;
  new_edge->cond = *cond;
  new_edge->kind = kind;
  new_edge->next_in_edge_worklist = NULL;
  if (analysis_debug & ADD_EDGE) {
    fputs("  ",stdout);
    print_edgeset(new_edge,stdout);
  }
  return new_edge;
}

void free_edge(EDGESET old, AUG_GRAPH *aug_graph) {
  old->rest = edgeset_freelist;
  if (old->kind == no_dependency)
    fatal_error("edgeset freelist corruption! (2)");
  old->kind = no_dependency;
  remove_from_worklist(old,aug_graph);
  edgeset_freelist = old;
}

void free_edgeset(EDGESET es, AUG_GRAPH *aug_graph) {
  while (es != NULL) {
    EDGESET old = es;
    es = es->rest;
    /* printf("Freeing 0x%x\n",old); */
    free_edge(old,aug_graph);
  }
}

DEPENDENCY edgeset_kind(EDGESET es) {
  DEPENDENCY max=no_dependency;
  for (; es != NULL; es=es->rest) {
    max=dependency_join(max,es->kind);
  }
  return max;
}

DEPENDENCY edgeset_lowerbound(EDGESET es) {
  DEPENDENCY max=no_dependency;
  for (; es != NULL; es=es->rest) {
    if (es->cond.positive|es->cond.negative) continue;
    max=dependency_join(max,es->kind);
  }
  return max;
}

/** Return TRUE if the edge is already in the graph. */
BOOL edge_present(INSTANCE *source,
		  INSTANCE *sink,
		  CONDITION* cond,
		  DEPENDENCY kind,
		  AUG_GRAPH *aug_graph)
{
  int index = source->index*(aug_graph->instances.length)+sink->index;

  EDGESET es = aug_graph->graph[index];
  while (es != NULL) {
    switch (cond_compare(cond,&es->cond)) {
    case CONDlt:
    case CONDeq:
      if (AT_MOST(kind,es->kind)) return TRUE;
      break;
    default:
      break;
    }
    es = es->rest;
  }
  return FALSE;
}

EDGESET add_edge(INSTANCE *source,
		 INSTANCE *sink,
		 CONDITION *cond,
		 DEPENDENCY kind,
		 EDGESET current,
		 AUG_GRAPH *aug_graph) {
  if (current == NULL) {
    EDGESET new_edge=new_edgeset(source,sink,cond,kind);
    add_to_worklist(new_edge,aug_graph);
    return new_edge;
  } else {
    enum CONDcompare comp = cond_compare(cond,&current->cond);
    if (current->source != source ||
	current->sink != sink) fatal_error("edgeset mixup");
    if (AT_MOST(kind,current->kind) &&
	(comp == CONDlt || comp == CONDeq)) {
      /* already entailed (or equal) */
      return current;
    } else if (AT_MOST(current->kind,kind) &&
	       (comp == CONDgt || comp == CONDeq)) {
      /* current entry is entailed, so remove */
      EDGESET rest = current->rest;
      free_edge(current,aug_graph);
      return add_edge(source,sink,cond,kind,rest,aug_graph);
    } else if (current->kind == kind && comp == CONDcomp) {
      /* edges are complements of each other */
      EDGESET rest = current->rest;
      CONDITION merged;
      unsigned common;
      merged.positive = cond->positive|current->cond.positive;
      merged.negative = cond->negative|current->cond.negative;
      common = merged.positive&merged.negative;
      if (common != 0 && !ONE_BIT(common))
	fatal_error("bad condition computation");
      merged.positive &= ~common;
      merged.negative &= ~common;
      rest = add_edge(source,sink,&merged,kind,rest,aug_graph);
      free_edge(current,aug_graph);
      return rest;
    } else {
      /* there is nothing to do */
      current->rest =
	add_edge(source,sink,cond,kind,current->rest,aug_graph);
      return current;
    }
  }
}

void add_edge_to_graph(INSTANCE *source,
		       INSTANCE *sink,
		       CONDITION *cond,
		       DEPENDENCY kind,
		       AUG_GRAPH *aug_graph) {
  int index = source->index*(aug_graph->instances.length)+sink->index;

  aug_graph->graph[index] =
    add_edge(source,sink,cond,kind,aug_graph->graph[index],aug_graph);
}

void add_transitive_edge_to_graph(INSTANCE *source,
				  INSTANCE *sink,
				  CONDITION *cond1,
				  CONDITION *cond2,
				  DEPENDENCY kind1,
				  DEPENDENCY kind2,
				  AUG_GRAPH *aug_graph) {
  CONDITION cond;
  DEPENDENCY kind=dependency_trans(kind1,kind2);
  cond.positive = cond1->positive|cond2->positive;
  cond.negative = cond1->negative|cond2->negative;
  if (cond.positive & cond.negative) return;
  add_edge_to_graph(source,sink,&cond,kind,aug_graph);
}
    
void close_using_edge(AUG_GRAPH *aug_graph, EDGESET edge);

void check_two_edge_cycle(EDGESET new_edge, AUG_GRAPH *aug_graph)
{
  /* First find the set of edges in the opposite direction */
  INSTANCE *source = new_edge->source;
  INSTANCE *sink = new_edge->sink;
  int oindex = sink->index*(aug_graph->instances.length)+source->index;
  EDGESET others = aug_graph->graph[oindex];
  static EDGESET recursive = 0;

  /* Now see if any of them cause a cycle with the given edge */
  for (; others != NULL; others=others->rest) {
    DEPENDENCY kind = dependency_trans(new_edge->kind,others->kind);
    CONDITION cond;

    if (kind == no_dependency) continue;

    cond.positive = new_edge->cond.positive | others->cond.positive;
    cond.negative = new_edge->cond.negative | others->cond.negative;
    if (cond.positive & cond.negative) continue;

    /* found a two edge cycle */

    printf("Found a two-edge cycle: \n  ");
    print_edge(new_edge,stdout); fputs("  ",stdout);
    print_edge(others,stdout);

    if (recursive) {
      printf("Caused by previous add: \n  ");
      print_edge(recursive,stdout);
    }
  }

  /* force transitive edges */
  if (!recursive) {
    recursive = new_edge;
    close_using_edge(aug_graph,new_edge);
    recursive = 0;
  }
}


/*** AUXILIARY FUNCTIONS FOR IFS ***/

static void *count_if_rules(void *pint, void *node) {
  int *count = (int *)pint;
  Match m;
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration decl = (Declaration)node;
    switch (Declaration_KEY(decl)) {
    case KEYmodule_decl: return NULL;
    case KEYif_stmt:
      Declaration_info(decl)->if_index = *count;
      ++*count;
      break;
    case KEYcase_stmt:
      for (m = first_Match(case_stmt_matchers(decl)); m; m=MATCH_NEXT(m)) {
	Match_info(m)->if_index = *count;
	++*count;
      }
      break;
    default: break;
    }
  }
  return pint;
}

static void *get_if_rules(void *varray, void *node) {
  void** array = (void**)varray;
  Match m;
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration decl = (Declaration)node;
    switch (Declaration_KEY(decl)) {
    case KEYmodule_decl: return NULL;
    case KEYif_stmt:
      array[Declaration_info(decl)->if_index] = decl;
      break;
    case KEYcase_stmt:
      for (m = first_Match(case_stmt_matchers(decl)); m; m=MATCH_NEXT(m)) {
	array[Match_info(m)->if_index] = m;
      }
      break;
    default: break;
    }
  }
  return array;
}

/* link together all the conditions in a case */
static void *get_match_tests(void *vpexpr, void *node)
{
  Expression* pexpr = (Expression*)vpexpr;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  default:
    return NULL;
  case KEYMatch:
    traverse_Pattern(get_match_tests,vpexpr,matcher_pat((Match)node));
    Match_info((Match)node)->match_test = *pexpr;
    return NULL;
  case KEYMatches:
  case KEYPatternActuals:
    break;
  case KEYPattern:
    { Pattern pat = (Pattern)node;
      switch (Pattern_KEY(pat)) {
      case KEYcondition:
	{ Expression cond = condition_e(pat);
	  Expression expr = *pexpr;
	  Expression_info(cond)->next_expr = expr;
	  *pexpr = cond;
	}
	break;
      default:
	break;
      }
    }
  }
  return vpexpr;
}

static Expression if_rule_test(void* if_rule) {
  switch (ABSTRACT_APS_tnode_phylum(if_rule)) {
  case KEYDeclaration:
    return if_stmt_cond((Declaration)if_rule);
  case KEYMatch:
    return Match_info((Match)if_rule)->match_test;
  default:
    fatal_error("%d: unknown if_rule",tnode_line_number(if_rule));
  }
}

static CONDITION* if_rule_cond(void *if_rule) {
  switch (ABSTRACT_APS_tnode_phylum(if_rule)) {
  case KEYDeclaration:
    return &Declaration_info((Declaration)if_rule)->decl_cond;
  case KEYMatch:
    return &Match_info((Match)if_rule)->match_cond;
  default:
    fatal_error("%d: unknown if_rule",tnode_line_number(if_rule));
  }
}

int if_rule_index(void *if_rule) {
  switch (ABSTRACT_APS_tnode_phylum(if_rule)) {
  case KEYDeclaration:
    return Declaration_info((Declaration)if_rule)->if_index;
  case KEYMatch:
    return Match_info((Match)if_rule)->if_index;
  default:
    fatal_error("%d: unknown if_rule",tnode_line_number(if_rule));
    /*NOTREACHED*/
    return 0;
  }
}
 
int if_rule_p(void *if_rule) {
  switch (ABSTRACT_APS_tnode_phylum(if_rule)) {
  case KEYDeclaration:
    return Declaration_KEY(if_rule) == KEYif_stmt;
  case KEYMatch:
    return TRUE;
  default:
    return 0;
  }
}
 
static void *init_decl_cond(void *vcond, void *node) {
  CONDITION *cond = (CONDITION *)vcond;
  Declaration decl = (Declaration)node; /*but maybe not*/
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    Declaration_info(decl)->decl_cond = *cond;
    switch (Declaration_KEY(decl)) {
    case KEYmodule_decl: return NULL;
    case KEYif_stmt:
      {
	Block if_true = if_stmt_if_true(decl);
	Block if_false = if_stmt_if_false(decl);
	int index = Declaration_info(decl)->if_index;
	CONDITION new_cond;
	new_cond.positive = cond->positive | (1 << index);
	new_cond.negative = cond->negative;
	traverse_Block(init_decl_cond,&new_cond,if_true);
	new_cond.positive = cond->positive;
	new_cond.negative = cond->negative | (1 << index);
	traverse_Block(init_decl_cond,&new_cond,if_false);
      }
      return NULL;
    case KEYcase_stmt:
      {
	Matches ms = case_stmt_matchers(decl);
	Expression testvar = case_stmt_expr(decl);
	CONDITION new_cond = *cond;
	Match m;

	Expression_info(testvar)->next_expr = 0;
	traverse_Matches(get_match_tests,&testvar,ms);
	for (m = first_Match(ms); m; m=MATCH_NEXT(m)) {
	  int index = Match_info(m)->if_index;
	  Match_info(m)->match_cond = new_cond;
	  new_cond.positive |= (1 << index); /* first set it */
	  traverse_Block(init_decl_cond,&new_cond,matcher_body(m));
	  new_cond.positive &= ~(1 << index); /* now clear it */
	  new_cond.negative |= (1 << index); /* set negative for rest */
	}
	traverse_Block(init_decl_cond,&new_cond,case_stmt_default(decl));
      }
      return NULL;
    default: break;
    }
  default:
    break;
  }
  return vcond;
}


/*** AUXILIARY FUNCTIONS FOR INSTANCES ***/

Declaration proc_call_p(Expression e) {
  Declaration decl = local_call_p(e);
  if (decl != NULL && Declaration_KEY(decl) == KEYprocedure_decl)
    return decl;
  else
    return NULL;
}

static void assign_instance(INSTANCE *array, int index,
			    Declaration attr, FIBER fiber,
			    Declaration node) {
  if (attr == NULL) fatal_error("null instance!");
  if (array == NULL) return;
  array[index].fibered_attr.attr = attr;
  array[index].fibered_attr.fiber = fiber;
  array[index].node = node;
  array[index].index = index;
  if (analysis_debug & CREATE_INSTANCE) {
    printf("Created instance %d: ", index);
    print_instance(&array[index],stdout);
    printf("\n");
  }
}

static int assign_instances(INSTANCE *array, int index,
			    Declaration attr, Declaration node) {
  FIBERSET fiberset;
  assign_instance(array,index++,attr,NULL,node);
  for (fiberset = fiberset_for(attr,FIBERSET_NORMAL_FINAL);
       fiberset != NULL;
       fiberset=fiberset->rest) {
    assign_instance(array,index++,attr,fiberset->fiber,node);
  }
  for (fiberset = fiberset_for(attr,FIBERSET_REVERSE_FINAL);
       fiberset != NULL;
       fiberset=fiberset->rest) {
    assign_instance(array,index++,attr,fiberset->fiber,node);
  }
  return index;
}

static Type infer_some_value_decl_type(Declaration d) {
  if (Declaration_KEY(d) == KEYnormal_formal) {
    return infer_formal_type(d);
  } else {
    return some_value_decl_type(d);
  }
}

/** Count and then assign instances.
 * Called in two cases: <ul>
 * <li> one to set instance indices and count instances
 * <li> assign instances to the array
 * </ul>
 */
static void *get_instances(void *vaug_graph, void *node) {
  AUG_GRAPH *aug_graph = (AUG_GRAPH *)vaug_graph;
  INSTANCE *array = aug_graph->instances.array;
  STATE *s = aug_graph->global_state;
  int index = -1;

  if (array == NULL) {
    index = aug_graph->instances.length;
  }
  
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration decl = (Declaration)node;
    if (index == -1) index = Declaration_info(decl)->instance_index;
    switch (Declaration_KEY(decl)) {
    case KEYmodule_decl:
      /* we let the module_decl represent the instance of the
       * root phylum in the global dependency graph.
       */
      if (array == NULL) Declaration_info(decl)->instance_index = index;
      {
	ATTRSET attrset=attrset_for(s,s->start_phylum);
	for (; attrset != NULL; attrset=attrset->rest) {
	  Declaration attr = attrset->attr;
	  FIBERSET fiberset;
	  assign_instance(array,index++,attr,NULL,decl);
	  for (fiberset = fiberset_for(attr,FIBERSET_NORMAL_FINAL);
	       fiberset != NULL;
	       fiberset=fiberset->rest) {
	    assign_instance(array,index++,attr,fiberset->fiber,decl);
	  }
	  for (fiberset = fiberset_for(attr,FIBERSET_REVERSE_FINAL);
	       fiberset != NULL;
	       fiberset=fiberset->rest) {
	    assign_instance(array,index++,attr,fiberset->fiber,decl);
	  }
	}
      }
      break;
    case KEYsome_function_decl:
    case KEYtop_level_match:
      /* don't look inside (unless its what we're doing the analysis for) */
      if (aug_graph->match_rule != decl) return NULL;
      if (array == NULL) Declaration_info(decl)->instance_index = index;
      /* if it has attributes it is the parameters, shared_info and result
       * for a function_decl.
       */
      { ATTRSET attrset=attrset_for(s,decl);
	for (; attrset != NULL; attrset=attrset->rest) {
	  Declaration attr = attrset->attr;
	  FIBERSET fiberset;
	  assign_instance(array,index++,attr,NULL,decl);
	  for (fiberset = fiberset_for(attr,FIBERSET_NORMAL_FINAL);
	       fiberset != NULL;
	       fiberset=fiberset->rest) {
	    assign_instance(array,index++,attr,fiberset->fiber,decl);
	  }
	  for (fiberset = fiberset_for(attr,FIBERSET_REVERSE_FINAL);
	       fiberset != NULL;
	       fiberset=fiberset->rest) {
	    assign_instance(array,index++,attr,fiberset->fiber,decl);
	  }
	}
      }
      break;
    case KEYformal: case KEYvalue_decl:
      if (array == NULL) Declaration_info(decl)->instance_index = index;
      { Type ty = infer_some_value_decl_type(decl);
      /*if (Type_KEY(ty) == KEYremote_type)
	ty = remote_type_nodetype(ty); */
	switch (Type_KEY(ty)) {
	default:
	  fprintf(stderr,"cannot handle type: ");
	  print_Type(ty,stderr);
	  fatal_error("\n%d:abort",tnode_line_number(ty));
	case KEYtype_use:
	  { Declaration tdecl = Use_info(type_use_use(ty))->use_decl;
	    if (tdecl == NULL) fatal_error("%d:type not bound",
					   tnode_line_number(ty));
	    /*printf("%d: finding instances for %s",tnode_line_number(decl),
	      decl_name(decl)); */
	    /* first direct fibers (but not for nodes & parameters) */
	    if (0 == (Declaration_info(decl)->decl_flags &
		      (ATTR_DECL_INH_FLAG|ATTR_DECL_SYN_FLAG|
		       DECL_LHS_FLAG|DECL_RHS_FLAG)))
	    { FIBERSET fiberset;
	      assign_instance(array,index++,decl,NULL,NULL);
	       /* printf("%s, first option: ",decl_name(decl));
		 print_fiberset(fiberset_for(decl,FIBERSET_NORMAL_FINAL),
		 stdout);
		 printf("\n"); */
	      for (fiberset = fiberset_for(decl,FIBERSET_NORMAL_FINAL);
		   fiberset != NULL;
		   fiberset=fiberset->rest) {
		assign_instance(array,index++,decl,fiberset->fiber,NULL);
	      }
	      /* printf(", ");
		 print_fiberset(fiberset_for(decl,FIBERSET_REVERSE_FINAL),stdout);
		 printf("\n"); */
	      for (fiberset = fiberset_for(decl,FIBERSET_REVERSE_FINAL);
		   fiberset != NULL;
		   fiberset=fiberset->rest) {
		assign_instance(array,index++,decl,fiberset->fiber,NULL);
	      }
	    } else
	    /* then fibers on attributes */
	    { ATTRSET attrset=attrset_for(s,tdecl);
	      /*printf(" Second option: ");
	        print_attrset(attrset,stdout);
	        printf("\n");*/
	      for (; attrset != NULL; attrset=attrset->rest) {
		Declaration attr = attrset->attr;
		FIBERSET fiberset;
		assign_instance(array,index++,attr,NULL,decl);
		for (fiberset = fiberset_for(attr,FIBERSET_NORMAL_FINAL);
		     fiberset != NULL;
		     fiberset=fiberset->rest) {
		  assign_instance(array,index++,attr,fiberset->fiber,decl);
		}
		for (fiberset = fiberset_for(attr,FIBERSET_REVERSE_FINAL);
		     fiberset != NULL;
		     fiberset=fiberset->rest) {
		  assign_instance(array,index++,attr,fiberset->fiber,decl);
		}
	      }
	    }
	  }
	  break;
	}
      }
      break;
    case KEYassign:
      { Declaration pdecl = proc_call_p(assign_rhs(decl));
	if (pdecl != NULL) {
	  if (array == NULL) {
	    STATE *s = aug_graph->global_state;
	    int i;
	    Declaration_info(decl)->instance_index = index;
	    Declaration_info(decl)->decl_flags |= DECL_RHS_FLAG;
	    for (i=0; i < s->phyla.length; ++i) {
	      if (s->phyla.array[i] == pdecl) {
		Declaration_info(decl)->node_phy_graph = &s->phy_graphs[i];
		break;
	      }
	    }
	  }
	  /* assertion check */
	  if (index != Declaration_info(decl)->instance_index)
	    fatal_error("%d: instance index %d != %d",tnode_line_number(decl),
			Declaration_info(decl)->instance_index,index);
	  { ATTRSET attrset=attrset_for(s,pdecl);
	    for (; attrset != NULL; attrset=attrset->rest) {
	      Declaration attr = attrset->attr;
	      index = assign_instances(array,index,attr,decl);
	    }
	  }
	}
      }
      break;
    case KEYpragma_call:
    case KEYattribute_decl:
    case KEYphylum_decl:
    case KEYtype_decl:
    case KEYconstructor_decl:
      return NULL;
    case KEYif_stmt:
      /* don't mess with instance_index */
      break;
    default:
      if (array == NULL) Declaration_info(decl)->instance_index = index;
    }
  } else if (ABSTRACT_APS_tnode_phylum(node) == KEYExpression) {
    Expression e = (Expression)node;
    Declaration fdecl = NULL;
    switch (Expression_KEY(e)) {
    default:
      break;
    case KEYfuncall:
      if ((fdecl = local_call_p(e)) != NULL) {
	ATTRSET attrset=attrset_for(s,fdecl);
	Declaration proxy = Expression_info(e)->funcall_proxy;

	/* printf("%d: Found local function call of %s\n",
	 *      tnode_line_number(e),decl_name(fdecl));
	 */
	if (array == NULL) {
	  extern int aps_yylineno;
	  aps_yylineno = tnode_line_number(e);
	  proxy = pragma_call(def_name(some_function_decl_def(fdecl)),
			      nil_Expressions());
	  Expression_info(e)->funcall_proxy = proxy;
	  Declaration_info(proxy)->instance_index = index;
	  Declaration_info(proxy)->node_phy_graph = 
	    summary_graph_for(s,fdecl);
	  Declaration_info(proxy)->decl_flags |= DECL_RHS_FLAG;
	} else {
	  index =  Declaration_info(proxy)->instance_index;
	}

	for (; attrset != NULL; attrset=attrset->rest) {
	  Declaration attr = attrset->attr;
	  index = assign_instances(array,index,attr,proxy);
	}
      }
      break;
    }
  }
  if (array == NULL) aug_graph->instances.length = index;
  return vaug_graph;
}

static INSTANCE *get_instance_or_null(Declaration attr, FIBER fiber, 
				      Declaration node, AUG_GRAPH *aug_graph)
{
  int i;
  INSTANCE *array = aug_graph->instances.array;
  int n = aug_graph->instances.length;
  int start = Declaration_info((node==NULL)?attr:node)->instance_index;

  if (fiber == base_fiber) fiber = NULL;

  for (i=start; i < n; ++i) {
    if (array[i].fibered_attr.attr == attr &&
	array[i].fibered_attr.fiber == fiber &&
	array[i].node == node) return &array[i];
  }
  return NULL;
}

INSTANCE *get_instance(Declaration attr, FIBER fiber,
		       Declaration node, AUG_GRAPH *aug_graph)
{
  INSTANCE *instance = get_instance_or_null(attr,fiber,node,aug_graph);
  if (instance != NULL) return instance;
  { INSTANCE in;
    int i;
    INSTANCE *array = aug_graph->instances.array;
    int n = aug_graph->instances.length;
    int start = Declaration_info((node==NULL)?attr:node)->instance_index;
    
    in.fibered_attr.attr = attr;
    in.fibered_attr.fiber = fiber;
    in.node = node;

    fputs("Looking for ",stderr);
    print_instance(&in,stderr);
    fputc('\n',stderr);
    for (i=0; i < n; ++i) {
      print_instance(&array[i],stderr);
      if (i < start) fputs(" (ignored)",stderr);
      fputc('\n',stderr);
    }
  }
  fatal_error("Could not get instance");
  return NULL;
}

static INSTANCE *get_summary_instance_or_null(Declaration attr, FIBER fiber, 
					      PHY_GRAPH* phy_graph)
{
  int i;
  INSTANCE *array = phy_graph->instances.array;
  int n = phy_graph->instances.length;
  int start = Declaration_info(attr)->instance_index;

  if (fiber == base_fiber) fiber = NULL;

  for (i=start; i < n; ++i) {
    if (array[i].fibered_attr.attr == attr &&
	array[i].fibered_attr.fiber == fiber &&
	array[i].node == NULL) return &array[i];
  }
  return NULL;
}

INSTANCE *get_summary_instance(Declaration attr, FIBER fiber,
			       PHY_GRAPH *phy_graph)
{
  INSTANCE *instance =
    get_summary_instance_or_null(attr,fiber,phy_graph);
  if (instance != NULL) return instance;
  { INSTANCE in;
    int i;
    INSTANCE *array = phy_graph->instances.array;
    int n = phy_graph->instances.length;
    int start = Declaration_info(attr)->instance_index;
    
    in.fibered_attr.attr = attr;
    in.fibered_attr.fiber = fiber;
    in.node = NULL;

    fputs("Looking for summary ",stderr);
    print_instance(&in,stderr);
    fputc('\n',stderr);
    for (i=0; i < n; ++i) {
      print_instance(&array[i],stderr);
      if (i < start) fputs(" (ignored)",stderr);
      fputc('\n',stderr);
    }
  }
  fatal_error("Could not get instance");
  return NULL;
}


/*******************************************				    
    Dependencies to edges 
 *********************************************/

typedef struct dep_modifier {
  Declaration field;
  struct dep_modifier *next;
} MODIFIER;
#define NO_MODIFIER ((MODIFIER*)0)

FIBER dep_modifier_fiber(MODIFIER* dm, FIBER f) {
  if (f == NULL) f = base_fiber;
  if (dm == 0) return f;
  f = dep_modifier_fiber(dm->next, f);
  if (f == 0) return f;
  f = (FIBER)assoc(dm->field,f->longer);
  return f;
}

typedef struct dep_vertex {
  Declaration attr;
  Declaration node;
  MODIFIER* modifier;
} VERTEX;

void print_dep_vertex(VERTEX*,FILE*);

INSTANCE* dep_vertex_instance(VERTEX* dv, FIBER f, AUG_GRAPH* aug_graph)
{
  f = dep_modifier_fiber(dv->modifier,f);
  if (f == NULL) return NULL;
  return get_instance_or_null(dv->attr,f,dv->node,aug_graph);
}

INSTANCE* summary_vertex_instance(VERTEX* dv, FIBER f, PHY_GRAPH* phy_graph)
{
  f = dep_modifier_fiber(dv->modifier,f);
  if (f == NULL) return NULL;
  return get_summary_instance_or_null(dv->attr,f,phy_graph);  
}

static BOOL decl_is_collection(Declaration d) {
  if (!d) return FALSE;
  switch (Declaration_KEY(d)) {
  case KEYvalue_decl: 
    return direction_is_collection(value_decl_direction(d));
  case KEYattribute_decl: 
    return direction_is_collection(attribute_decl_direction(d));
    // XXX: should handle value_renaming
  default:
    return FALSE;
  }
}

static BOOL decl_is_circular(Declaration d) {
  if (!d) return FALSE;
  switch (Declaration_KEY(d)) {
  case KEYvalue_decl: 
    return direction_is_circular(value_decl_direction(d));
  case KEYattribute_decl: 
    return direction_is_circular(attribute_decl_direction(d));
  default:
    return FALSE;
  }
}

// return true if we are sure this vertex represents an input dependency
static BOOL vertex_is_input(VERTEX* v) 
{
  BOOL result;

  if (!v->attr) return FALSE;
  if (v->modifier) return FALSE;

  if (ATTR_DECL_IS_INH(v->attr)) result = TRUE;
  else if (ATTR_DECL_IS_SYN(v->attr)) result = FALSE;
  else return FALSE;

  if (!v->node) return result;

  if (DECL_IS_RHS(v->node)) result = !result;
  else if (!(DECL_IS_LHS(v->node))) result = FALSE;
  return result;
}

static BOOL vertex_is_output(VERTEX* v) 
{
  if (!v->attr) return FALSE;
  BOOL result;
  if (ATTR_DECL_IS_INH(v->attr)) result = FALSE;
  else if (ATTR_DECL_IS_SYN(v->attr)) result = TRUE;
  else return FALSE;

  if (!v->node) return result;

  if (DECL_IS_RHS(v->node)) result = !result;
  else if (!(DECL_IS_LHS(v->node))) result = FALSE;
  return result;
}

/**
 * Connect the two vertices together, and if the kind is not indirect,
 * add fiber dependencies too.
 */
void add_edges_to_graph(VERTEX* v1,
			VERTEX* v2,
			CONDITION *cond,
			DEPENDENCY kind,
			AUG_GRAPH *aug_graph) {
  STATE *s = aug_graph->global_state;
  int i;

  if (analysis_debug & ADD_EDGE) {
    print_dep_vertex(v1,stdout);
    fputs("->",stdout);
    print_dep_vertex(v2,stdout);
    fputs(":",stdout);
    print_edge_helper(kind,cond,stdout);
    puts("");
  }

  // first add simple edge
  {
    INSTANCE *source = dep_vertex_instance(v1,0,aug_graph);
    INSTANCE *sink = dep_vertex_instance(v2,0,aug_graph);
    if (source != NULL && sink != NULL) {
      add_edge_to_graph(source,sink,cond,kind,aug_graph);
    } else if (analysis_debug & ADD_EDGE) {
      printf("  not added %s %s\n", 
	     source ? "" : "null source",
	     sink ? "" : "null sink");
    }
  }

  // if not carrying, don't add fiber dependencies
  if ((kind & DEPENDENCY_MAYBE_CARRYING) == 0) return;

  // if carrying, then we add fiber dependencies.
  kind &= ~DEPENDENCY_NOT_JUST_FIBER;
  
  for (i=0; i < s->fibers.length; ++i) {
    FIBER f = s->fibers.array[i];
    INSTANCE *i1 = dep_vertex_instance(v1,f,aug_graph);
    INSTANCE *i2 = dep_vertex_instance(v2,f,aug_graph);
    if (f == base_fiber || i1 == NULL || i2 == NULL) continue;
    if (fiber_is_reverse(f)) {
      INSTANCE *i = i1;
      i1 = i2;
      i2 = i;
    }
    add_edge_to_graph(i1,i2,cond,kind,aug_graph);
  }
}

Declaration attr_ref_node_decl(Expression e)
{
  Expression node = attr_ref_object(e);
  switch (Expression_KEY(node)) {
  default: fatal_error("%d: can't handle this attribute instance",
		       tnode_line_number(node));
  case KEYvalue_use:
    return USE_DECL(value_use_use(node));
  }
}

/** Add edges to the dependency graph to represent dependencies
 * from the value computed in the expression to the given sink
 * @param cond condition under which dependency holds
 * @param mod modifier to apply to instances found
 * @param kind whether carrying/non-fiber etc.
 */
static void record_expression_dependencies(VERTEX *sink, CONDITION *cond,
					   DEPENDENCY kind, MODIFIER *mod,
					   Expression e, AUG_GRAPH *aug_graph)
{
  VERTEX source;
  MODIFIER new_mod;
  /* Several different cases
   *
   * l
   * n.attr
   * f(...)
   * constructor(...)
   * expr.field
   * shared
   */
  switch (Expression_KEY(e)) {
  default:
    fatal_error("%d: cannot handle this expression (%d)\n",
		tnode_line_number(e), Expression_KEY(e));
    break;
  case KEYinteger_const:
  case KEYreal_const:
  case KEYstring_const:
  case KEYchar_const:
    /* nothing to do */
    break;
  case KEYrepeat:
    record_expression_dependencies(sink,cond,kind,mod,
				   repeat_expr(e),aug_graph);
    break;
  case KEYvalue_use:
    { Declaration decl=Use_info(value_use_use(e))->use_decl;
      Declaration rdecl;
      int new_kind = kind;
      if (!decl_is_circular(decl)) new_kind |= DEPENDENCY_MAYBE_SIMPLE;
      if (decl == NULL)
	fatal_error("%d: unbound expression",tnode_line_number(e));
      if (DECL_IS_LOCAL(decl) &&
	  DECL_IS_SHARED(decl) &&
	  (rdecl = responsible_node_declaration(e)) != NULL) {
	/* a shared dependency: we get it from the shared_info */
	Declaration phy = node_decl_phylum(rdecl);

	source.node = rdecl;
	source.attr = 
	  phylum_shared_info_attribute(phy,aug_graph->global_state);
	source.modifier = NO_MODIFIER;
	if (vertex_is_output(&source)) aps_warning(e,"Dependence on output value");
	add_edges_to_graph(&source,sink,cond,new_kind&~DEPENDENCY_MAYBE_CARRYING,
			   aug_graph);

	new_mod.field = decl;
	new_mod.next = mod;
	source.modifier = &new_mod;
	add_edges_to_graph(&source,sink,cond,new_kind,aug_graph);
      } else if (Declaration_info(decl)->decl_flags &
		 (ATTR_DECL_INH_FLAG|ATTR_DECL_SYN_FLAG)) {
	/* a use of parameter or result (we hope) */
	Declaration fdecl = aug_graph->match_rule;
	source.node = aug_graph->match_rule;
	source.attr = decl;
	source.modifier = mod;
	if (vertex_is_output(&source)) aps_warning(e,"Dependence on output value");
	add_edges_to_graph(&source,sink,cond,new_kind,aug_graph);
      } else {
	source.node = NULL;
	source.attr = decl;
	source.modifier = mod;
	if (vertex_is_output(&source)) aps_warning(e,"Dependence on output value");
	add_edges_to_graph(&source,sink,cond,new_kind,aug_graph);
      }
    }
    break;
  case KEYfuncall:
    { Declaration decl;
      int new_kind = kind;
      if ((decl = attr_ref_p(e)) != NULL) {
  if (!decl_is_circular(decl)) new_kind |= DEPENDENCY_MAYBE_SIMPLE; else new_kind = kind;
	source.node = attr_ref_node_decl(e);
	source.attr = decl;
	source.modifier = mod;
	if (vertex_is_output(&source)) aps_warning(e,"Dependence on output value");
	add_edges_to_graph(&source,sink,cond,new_kind,aug_graph);
      } else if ((decl = field_ref_p(e)) != NULL) {
  if (!decl_is_circular(decl)) new_kind |= DEPENDENCY_MAYBE_SIMPLE; else new_kind = kind;
	Expression object = field_ref_object(e);
	new_mod.field = decl;
	new_mod.next = mod;
	// first the dependency on the pointer itself (NOT carrying)
	record_expression_dependencies(sink,cond,
				       new_kind&~DEPENDENCY_MAYBE_CARRYING,
				       NO_MODIFIER,object,aug_graph);
	// then the dependency on the field (possibly carrying)
	record_expression_dependencies(sink,cond,new_kind,&new_mod,object,
				       aug_graph);
      } else if ((decl = local_call_p(e)) != NULL) {
  if (!decl_is_circular(decl)) new_kind |= DEPENDENCY_MAYBE_SIMPLE; else new_kind = kind;
	Declaration result = some_function_decl_result(decl);
	Expression actual = first_Actual(funcall_actuals(e));
	/* first depend on the arguments (not carrying, no fibers) */
	if (mod == NO_MODIFIER) {
	  for (;actual!=NULL; actual=Expression_info(actual)->next_actual) {
	    record_expression_dependencies(sink,cond,
					   new_kind&~DEPENDENCY_MAYBE_CARRYING,
					   NO_MODIFIER,actual,aug_graph);
	  }
	}

	/* attach to result, and somewhere else ? attach actuals */
	/* printf("%d: looking at local function %s\n",
	 *	 tnode_line_number(e),decl_name(decl));
	 */
	{
	  Declaration proxy = Expression_info(e)->funcall_proxy;
	  source.node = proxy;
	  source.attr = result;
	  source.modifier = mod;
	  if (vertex_is_output(&source)) aps_warning(e,"Dependence on output value");
	  add_edges_to_graph(&source,sink,cond,kind,aug_graph);
	}
      } else {
	/* some random (external) function call */
	Expression actual = first_Actual(funcall_actuals(e));
	for (; actual != NULL; actual=Expression_info(actual)->next_actual) {
	  record_expression_dependencies(sink,cond,kind,mod,
					 actual,aug_graph);
	}
      }
    }
    break;
  }
}

void record_condition_dependencies(VERTEX *sink, CONDITION *cond,
				   AUG_GRAPH *aug_graph) {
  int i;
  unsigned bits=cond->positive|cond->negative;
  /*
  { print_instance(sink,stdout);
    printf(" <- ");
    print_condition(cond,stdout);
    printf(" = 0%o\n",bits); }
    */
  for (i=0; i < aug_graph->if_rules.length; ++i) {
    int mask = (1 << i);
    if (mask & bits) {
      void* if_rule = aug_graph->if_rules.array[i];
      /* printf("Getting dependencies for condition %d\n",i); */
      CONDITION *cond2 = if_rule_cond(if_rule);
      VERTEX if_vertex;
      if_vertex.node = 0;
      if_vertex.attr = if_rule;
      if_vertex.modifier = NO_MODIFIER;
      add_edges_to_graph(&if_vertex,sink,cond2,control_dependency,aug_graph);
    }
  }
}

static void set_value_for(VERTEX* sink, Expression rhs, AUG_GRAPH *aug_graph)
{
  Expression_info(rhs)->value_for = dep_vertex_instance(sink,NULL,aug_graph);
}

/** Add edges to the dependency graph to represent dependencies
 * from the rhs to the modified expression.
 * @param cond condition under which dependency holds
 * @param mod modifier to apply to instances found in the lhs
 * @param kind whether carrying/non-fiber etc.
 * @param the rhs that this depends on
 */
static void record_lhs_dependencies(Expression lhs, CONDITION *cond,
				    DEPENDENCY kind, MODIFIER *mod,
				    Expression rhs, AUG_GRAPH *aug_graph)
{
  switch (Expression_KEY(lhs)) {
  default:
    fatal_error("%d: Unknown lhs key %d",
		tnode_line_number(lhs),Expression_KEY(lhs));
    break;
  case KEYinteger_const:
  case KEYreal_const:
  case KEYstring_const:
  case KEYchar_const:
    if (mod != NULL) {
      aps_error(lhs,"Cannot assign a constant.\n");
    }
    /* no dependencies */
    break;
  case KEYvalue_use:
    {
      Declaration decl = USE_DECL(value_use_use(lhs));
      VERTEX sink;
      MODIFIER new_mod;
	
      if (shared_use_p(lhs) != NULL) {
	if (!mod) {
	  if (def_is_constant(value_decl_def(decl))) {
	      aps_error(lhs,"Assignment to non-var disallowed: %s",
			decl_name(decl));
	  }
	}
	/* Assignment of shared global (or a field of a shared global) */
	new_mod.next = mod;
	if (EXPR_IS_LHS(lhs)) {
	  new_mod.field = reverse_field(decl);
	} else {
	  new_mod.field = decl;
	}
	sink.node = responsible_node_declaration(lhs);
	if (sink.node == NULL) {
	  fatal_error("%d: Assignment of global %s in global space???",
		      tnode_line_number(lhs), decl_name(decl));
	  return;
	}
	/*!!!HERE! What if this is null ? */
	sink.attr =
	  phylum_shared_info_attribute(node_decl_phylum(sink.node),
				       aug_graph->global_state);
	sink.modifier = &new_mod;
	/* should also force a backwards depend on the shared info,
	 * but shared info is actually uninteresting,
	 * so I'll skip it...
	 */
      } else if (Declaration_info(decl)->decl_flags &
		 (ATTR_DECL_INH_FLAG|ATTR_DECL_SYN_FLAG)) {
	/* a use of parameter or result (we hope) */
	sink.node = aug_graph->match_rule;
	sink.attr = decl;
	sink.modifier = mod;
      } else {
	sink.node = 0;
	sink.attr = decl;
	sink.modifier = mod;
      }
      set_value_for(&sink,rhs,aug_graph);
      if (vertex_is_input(&sink)) aps_error(lhs,"Assignment of input value");
      // don't make error even if not output: local attributes!
      record_expression_dependencies(&sink,cond,kind,NULL,rhs,aug_graph);
      record_condition_dependencies(&sink,cond,aug_graph);
    }
    break;
  case KEYfuncall:
    {
      int new_kind = kind;
      Declaration field, attr, fdecl, decl;
      VERTEX sink;
      if ((field = field_ref_p(lhs)) != NULL) {
  if (!decl_is_circular(field)) new_kind |= DEPENDENCY_MAYBE_SIMPLE; else new_kind = kind;
	/* Assignment of a field, or a field of a field */
	Expression object = field_ref_object(lhs);
	MODIFIER new_mod;
	new_mod.next = mod;
	if (EXPR_IS_LHS(lhs)) {
	  new_mod.field = reverse_field(field);
	} else {
	  new_mod.field = field;
	}
	record_lhs_dependencies(object,cond,kind,&new_mod,rhs,aug_graph);
	record_lhs_dependencies(object,cond,control_dependency,
				&new_mod,object,aug_graph);
      } else if ((attr = attr_ref_p(lhs)) != NULL) {
  if (!decl_is_circular(attr)) new_kind |= DEPENDENCY_MAYBE_SIMPLE; else new_kind = kind;
	sink.node = attr_ref_node_decl(lhs);
	sink.attr = attr;
	sink.modifier = mod;
	set_value_for(&sink,rhs,aug_graph);
	if (vertex_is_input(&sink)) aps_error(lhs,"Assignment of input value");
	record_expression_dependencies(&sink,cond,kind,NULL,rhs,aug_graph);
	record_condition_dependencies(&sink,cond,aug_graph);
      } else if ((fdecl = local_call_p(lhs)) != NULL) {     
  if (!decl_is_circular(fdecl)) new_kind |= DEPENDENCY_MAYBE_SIMPLE; else new_kind = kind;
	Declaration result = some_function_decl_result(decl);
	Declaration proxy = Expression_info(lhs)->funcall_proxy;
	if (mod == NO_MODIFIER) {
	  aps_error(lhs,"How can we assign the result of a function?");
	}
	sink.node = proxy;
	sink.attr = result;
	sink.modifier = mod;
	set_value_for(&sink,rhs,aug_graph);
	if (vertex_is_input(&sink)) aps_error(lhs,"Assignment of input value");
	record_expression_dependencies(&sink,cond,new_kind,NULL,rhs,aug_graph);
	record_condition_dependencies(&sink,cond,aug_graph);
      } else {
	Expression actual = first_Actual(funcall_actuals(lhs));
	if (mod == NO_MODIFIER) {
	  aps_error(lhs,"How can we assign the result of a function?");
	}
	for (; actual != NULL; actual=Expression_info(actual)->next_actual) {
	  record_lhs_dependencies(actual,cond,kind,mod,rhs,aug_graph);
	}
      }
    }
    break;
  }
}

/* Initialize the edge set in the augmented dependency graph
 * from each rule for the production.  This is the meat of
 * the analysis process.  We use fiber information to get the
 * dependencies.
 */
static void *get_edges(void *vaug_graph, void *node) {
  AUG_GRAPH *aug_graph = (AUG_GRAPH *)vaug_graph;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  default:
    break;
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      CONDITION *cond = &Declaration_info(decl)->decl_cond;
      /* Several different cases:
       *
       * l : type := value;
       * o : type := constr(...);
       * l := value
       * n.attr := value;
       * o.field := value
       * expr.coll :> value;
       * shared :> value;
       * and then "if" and "case."
       *
       * In all cases, we have to use the condition
       * on the declaration as well.
       *
       */
      switch (Declaration_KEY(decl)) {
      case KEYsome_function_decl:
      case KEYtop_level_match:
      case KEYattribute_decl:
      case KEYpragma_call:
      case KEYphylum_decl:
      case KEYtype_decl:
      case KEYtype_renaming:
      case KEYconstructor_decl:
	/* Don't look in nested things */
	return NULL;
      case KEYformal:
	{ Declaration case_stmt = formal_in_case_p(decl);
	  if (case_stmt != NULL) {
	    Expression expr = case_stmt_expr(case_stmt);
	    VERTEX f;
	    f.node = 0;
	    f.attr = decl;
	    f.modifier = NO_MODIFIER;
	    record_condition_dependencies(&f,cond,aug_graph);
	    record_expression_dependencies(&f,cond,dependency,NO_MODIFIER,
					   expr,aug_graph);
	  }
	}
	break;
      case KEYvalue_decl:
	{
	  Default def = value_decl_default(decl);
	  Declaration cdecl;
	  VERTEX sink;
	  sink.attr = decl;
	  sink.node = responsible_node_declaration(decl);
	  sink.modifier = NO_MODIFIER;
	  record_condition_dependencies(&sink,cond,aug_graph);
	  switch (Default_KEY(def)) {
	  case KEYno_default: break;
	  case KEYsimple:
	    /* XXX: I don't know I have to do it both ways.
	     */
	    record_expression_dependencies(&sink,cond,dependency,NO_MODIFIER,
					   simple_value(def),aug_graph);
	    sink.node = 0;
	    record_expression_dependencies(&sink,cond,dependency,NO_MODIFIER,
					   simple_value(def),aug_graph);
	    if ((cdecl = constructor_call_p(simple_value(def))) != NULL) {
	      FIBERSET fs = fiberset_for(decl,FIBERSET_NORMAL_FINAL);
	      FIBERSET rfs = fiberset_for(decl,FIBERSET_REVERSE_FINAL);
	      Declaration pdecl = constructor_decl_phylum(cdecl);
	      Declaration fdecl;
	      /*
	      printf("Looking at %s which instantiates %s\n",decl_name(decl),
		     decl_name(cdecl));
	      printf("Normal: ");
	      print_fiberset(fs,stdout);
	      printf("\nReverse: ");
	      print_fiberset(rfs,stdout);
	      printf("\n");
	      */
	      /* add fiber dependencies for fields */
	      for (fdecl = NEXT_FIELD(pdecl);
		   fdecl != NULL;
		   fdecl = NEXT_FIELD(fdecl)) {
		Declaration rfdecl = reverse_field(fdecl);
		VERTEX source;
		MODIFIER dot_mod;
		MODIFIER nodot_mod;
		source.node = NULL;
		source.attr = decl;
		dot_mod.field = rfdecl;
		nodot_mod.field = fdecl;
		dot_mod.next = nodot_mod.next = NO_MODIFIER;
		sink.modifier = &nodot_mod;
		source.modifier = &dot_mod;
		DEPENDENCY new_kind = fiber_dependency;
		if (decl_is_circular(fdecl)) {
      new_kind &= ~DEPENDENCY_MAYBE_SIMPLE;
    } else {
      new_kind |= DEPENDENCY_MAYBE_SIMPLE;
    }

		add_edges_to_graph(&source,&sink,cond,new_kind,
				   aug_graph);
	      }
	    }
	    break;
	  case KEYcomposite:
	    record_expression_dependencies(&sink,cond,dependency,NO_MODIFIER,
					   composite_initial(def),aug_graph);
	    break;
	  }
	  if (DECL_IS_SHARED(decl)) {
	    /* add edges for shared info */
	    STATE *s = aug_graph->global_state;
	    Declaration sattr =
	      phylum_shared_info_attribute(s->start_phylum,s);
	    Declaration rdecl = reverse_field(decl);
	    Declaration module = aug_graph->match_rule;
	    VERTEX shared_info_fiber, this_decl;
	    MODIFIER dot_mod;
	    MODIFIER nodot_mod;
	    shared_info_fiber.node = module;
	    shared_info_fiber.attr = sattr;
	    this_decl.node = 0;
	    this_decl.attr = decl;
	    this_decl.modifier = NO_MODIFIER;
	    dot_mod.field = rdecl;
	    nodot_mod.field = decl;
	    dot_mod.next = nodot_mod.next = NO_MODIFIER;

	    /* add edges to collect value */
	    shared_info_fiber.modifier = &dot_mod;
	    add_edges_to_graph(&shared_info_fiber,&this_decl,
			       cond,dependency,aug_graph);

	    /* add edges to broadcast value */
	    shared_info_fiber.modifier = &nodot_mod;
	    add_edges_to_graph(&this_decl,&shared_info_fiber,
			       cond,dependency,aug_graph);
	  }
	}
	break;
      case KEYassign:
	{ Expression lhs=assign_lhs(decl);
	  Expression rhs=assign_rhs(decl);
	  record_lhs_dependencies(lhs,cond,dependency,NULL,rhs,aug_graph);
	}
	break;
      case KEYif_stmt:
	{
	  Expression test = if_stmt_cond(decl);
	  VERTEX sink;
	  sink.node = 0;
	  sink.attr = decl;
	  sink.modifier = NO_MODIFIER;
	  record_condition_dependencies(&sink,cond,aug_graph);

	  record_expression_dependencies(&sink,cond,control_dependency,
					 NO_MODIFIER, test, aug_graph);
	}
	break;
      case KEYcase_stmt:
	{
	  Match m;
	  VERTEX sink;
	  sink.node = 0;
	  sink.modifier = NO_MODIFIER;
	  for (m=first_Match(case_stmt_matchers(decl)); m; m=MATCH_NEXT(m)) {
	    Expression test = Match_info(m)->match_test;
	    sink.attr = (Declaration)m;
	    record_condition_dependencies(&sink,cond,aug_graph);

	    for (; test != 0; test = Expression_info(test)->next_expr) {
	      record_expression_dependencies(&sink,cond,control_dependency,
					     NO_MODIFIER, test, aug_graph);
	    }
	  }
	}
	break;
      default:
	printf("%d: don't handle this kind yet\n",tnode_line_number(decl));
	break;
      }
    }
    break;
  case KEYExpression:
    {
      Expression e = (Expression) node;
      Declaration fdecl;
      
      if ((fdecl = local_call_p(e)) != NULL) {
	Declaration proxy = Expression_info(e)->funcall_proxy;
	CONDITION *cond;
	void *parent = tnode_parent(node);

	while (ABSTRACT_APS_tnode_phylum(parent) != KEYDeclaration) {
	  parent = tnode_parent(parent);
	}
	cond = &Declaration_info((Declaration)parent)->decl_cond;

	/* connect result to conditions */
	{
	  Declaration rd = some_function_decl_result(fdecl);
	  VERTEX sink;
	  sink.node = proxy;
	  sink.attr = rd;
	  sink.modifier = NO_MODIFIER;
	  record_condition_dependencies(&sink,cond,aug_graph);
	}

	/* connect formals and actuals */
	{
	  Type ft = some_function_decl_type(fdecl);
	  Declaration f = first_Declaration(function_type_formals(ft));
	  Expression a = first_Actual(funcall_actuals(e));
	  for (; f != NULL; f = DECL_NEXT(f), a = EXPR_NEXT(a)) {
	    VERTEX sink;
	    sink.node = proxy;
	    sink.attr = f;
	    sink.modifier = NO_MODIFIER;
	    record_expression_dependencies(&sink,cond,dependency,
					   NO_MODIFIER,a,aug_graph);
	    record_condition_dependencies(&sink,cond,aug_graph);
	  }
	}

	/* connect shared info */
	{
	  STATE *s = aug_graph->global_state;
	  Declaration rnode = responsible_node_declaration(e);
	  Declaration rnodephy = node_decl_phylum(rnode);
	  Declaration lattr = phylum_shared_info_attribute(rnodephy,s);
	  Declaration rattr = phylum_shared_info_attribute(fdecl,s);
	  VERTEX source, sink;
	  source.node = rnode;
	  source.attr = lattr;
	  source.modifier = NO_MODIFIER;
	  sink.node = proxy;
	  sink.attr = rattr;
	  sink.modifier = NO_MODIFIER;
	  add_edges_to_graph(&source,&sink,cond,dependency,aug_graph);
	  record_condition_dependencies(&sink,cond,aug_graph);
	}
      }
    }
    break;
  }
  return vaug_graph;
}


/*** AUXILIARY FUNCTIONS FOR SUMMARY INFORMATION AND FOR PHYLA ***/

PHY_GRAPH* summary_graph_for(STATE *state, Declaration pdecl)
{
  int i;
  for (i=0; i < state->phyla.length; ++i) {
    if (state->phyla.array[i] == pdecl) {
      return &state->phy_graphs[i];
    }
  }
  fatal_error("could not find summary graph for %s",decl_name(pdecl));
  /*NOTREACHED*/
  return 0;
}

ATTRSET attrset_for(STATE *s, Declaration phylum) {
  return (ATTRSET)get(s->phylum_attrset_table,phylum);
}

void add_attrset_for(STATE *s, Declaration phylum, Declaration attr) {
  ATTRSET attrset = (ATTRSET)HALLOC(sizeof(struct attrset));
  attrset->rest = get(s->phylum_attrset_table,phylum);
  attrset->attr = attr;
  set(s->phylum_attrset_table,phylum,attrset);
}


/*** INITIALIZATION ***/

static void *mark_local(void *ignore, void *node) {
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration_info((Declaration)node)->decl_flags |= DECL_LOCAL_FLAG;
  }
  return node;
}

static void init_node_phy_graph2(Declaration node, Type ty, STATE *state) { 
  switch (Type_KEY(ty)) {
  default:
    fprintf(stderr,"%d: cannot handle type: ",tnode_line_number(ty));
    print_Type(ty,stderr);
    fputc('\n',stderr);
    fatal_error("Abort");
  case KEYtype_use:
    { Use u = type_use_use(ty);
      Declaration phylum=Use_info(u)->use_decl;
      if (phylum == NULL)
	fatal_error("%d: unbound type",tnode_line_number(ty));
      switch (Declaration_KEY(phylum)) {
      case KEYphylum_decl:
	Declaration_info(node)->node_phy_graph =
	  summary_graph_for(state,phylum);
	break;
      case KEYtype_decl:
	break;
      case KEYtype_renaming:
	init_node_phy_graph2(node,type_renaming_old(phylum),state);
	break;
      default:
	aps_error(node,"could not find type for summary graph %s",decl_name(phylum));
	break;
      }
    }
    break;
  case KEYremote_type:
    init_node_phy_graph2(node,remote_type_nodetype(ty),state);
    break;
  }
}


static void init_node_phy_graph(Declaration node, STATE *state) {
  Type ty=infer_formal_type(node);
  init_node_phy_graph2(node,ty,state);
}

void print_aug_graph(AUG_GRAPH *aug_graph, FILE *stream);

static void init_augmented_dependency_graph(AUG_GRAPH *aug_graph, 
					    Declaration tlm,
					    STATE *state)
{
  Block body;
  aug_graph->match_rule = tlm;
  aug_graph->global_state = state;

  switch (Declaration_KEY(tlm)) {
  default: fatal_error("%d:unknown top-level-match",tnode_line_number(tlm));
  case KEYmodule_decl: /* representing shared instances. */
    aug_graph->syntax_decl = tlm;
    aug_graph->lhs_decl = tlm;
    aug_graph->first_rhs_decl = NULL;
    { int i;
      for (i=0; i < state->phyla.length; ++i) {
	if (state->phyla.array[i] == state->start_phylum) break;
      }
      if (i == state->phyla.length)
	fatal_error("%d: Cannot find start phylum summary graph",
		    tnode_line_number(tlm));
      Declaration_info(tlm)->node_phy_graph = &state->phy_graphs[i];
      Declaration_info(tlm)->decl_flags |= DECL_RHS_FLAG;
    }
    body = module_decl_contents(tlm);
    break;
  case KEYsome_function_decl:
    aug_graph->syntax_decl = tlm;
    aug_graph->lhs_decl = tlm;
    aug_graph->first_rhs_decl = NULL;
    { int i;
      for (i=0; i < state->phyla.length; ++i) {
	if (state->phyla.array[i] == tlm) break;
      }
      if (i == state->phyla.length)
	fatal_error("%d: Cannot find summary phylum graph",
		    tnode_line_number(tlm));
      Declaration_info(tlm)->node_phy_graph = &state->phy_graphs[i];
    }
    body = some_function_decl_body(tlm);
    Declaration_info(tlm)->decl_flags |= DECL_LHS_FLAG;
    { Type ftype=some_function_decl_type(tlm);
      Declaration formal=first_Declaration(function_type_formals(ftype));
      Declaration result=first_Declaration(function_type_return_values(ftype));
      for (; formal != NULL; formal=Declaration_info(formal)->next_decl) {
	Declaration_info(formal)->decl_flags |= ATTR_DECL_INH_FLAG;
      }
      for (; result != NULL; result=Declaration_info(result)->next_decl) {
	Declaration_info(result)->decl_flags |= ATTR_DECL_SYN_FLAG;
      }
    }
    break;
  case KEYtop_level_match:
    body = matcher_body(top_level_match_m(tlm));
    { Pattern pat=matcher_pat(top_level_match_m(tlm));
      switch (Pattern_KEY(pat)) {
      default: fatal_error("%d:improper top-level-match",
			   tnode_line_number(tlm));
      case KEYand_pattern:
	switch (Pattern_KEY(and_pattern_p1(pat))) {
	default: fatal_error("%d:improper top-level-match",
			     tnode_line_number(tlm));
	case KEYpattern_var:
	  aug_graph->lhs_decl = pattern_var_formal(and_pattern_p1(pat));
	  Declaration_info(aug_graph->lhs_decl)->decl_flags |= DECL_LHS_FLAG;
	  init_node_phy_graph(aug_graph->lhs_decl,state);
	  break;
	}
	pat = and_pattern_p2(pat);
      }
      switch (Pattern_KEY(pat)) {
      default: fatal_error("%d:misformed pattern",tnode_line_number(pat));
      case KEYpattern_call:
	switch (Pattern_KEY(pattern_call_func(pat))) {
	default:
	  fatal_error("%d:unknown pattern function",tnode_line_number(pat));
	case KEYpattern_use:
	  { Declaration decl =
	      Use_info(pattern_use_use(pattern_call_func(pat)))->use_decl;
	    if (decl == NULL) fatal_error("%d:unbound pfunc",
					  tnode_line_number(pat));
	    aug_graph->syntax_decl = decl;
	  }
	}
	{ Declaration last_rhs = NULL;
	  Pattern next_pat;
	  for (next_pat = first_PatternActual(pattern_call_actuals(pat));
	       next_pat != NULL;
	       next_pat = Pattern_info(next_pat)->next_pattern_actual) {
	    switch (Pattern_KEY(next_pat)) {
	    default:
	      aps_error(next_pat,"too complex a pattern");
	      break;
	    case KEYpattern_var:
	      { Declaration next_rhs = pattern_var_formal(next_pat);
		Declaration_info(next_rhs)->decl_flags |= DECL_RHS_FLAG;
		init_node_phy_graph(next_rhs,state);
		if (last_rhs == NULL) {
		  aug_graph->first_rhs_decl = next_rhs;
		} else {
		  Declaration_info(last_rhs)->next_decl = next_rhs;
		}
		last_rhs = next_rhs;
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

  /* initialize the if_rules vector */
  { int num_if_rules = 0;
    CONDITION cond;
    traverse_Declaration(count_if_rules,&num_if_rules,tlm);
    if (num_if_rules > 32)
      fatal_error("Can handle up to 32 conditionals (got %d)",num_if_rules);

    VECTORALLOC(aug_graph->if_rules,void*,num_if_rules);
    traverse_Declaration(get_if_rules,aug_graph->if_rules.array,tlm);
    /* now initialize the decl_cond on every Declaration */
    cond.positive = 0;
    cond.negative = 0;
    traverse_Declaration(init_decl_cond,&cond,tlm);
  }

  /* initialize the instances vector */
  { 
    int i, n = aug_graph->if_rules.length;
    aug_graph->instances.length = n; /* add ifs */
    aug_graph->instances.array = NULL;
    traverse_Declaration(get_instances,aug_graph,tlm);
    VECTORALLOC(aug_graph->instances,INSTANCE,aug_graph->instances.length);
    for (i=0; i < n; ++i) {
      assign_instance(aug_graph->instances.array, i,
		      (Declaration)aug_graph->if_rules.array[i], /* kludge */
		      0,0);
    }
    traverse_Declaration(get_instances,aug_graph,tlm);
  }

  /* allocate the edge set array */
  { int n = aug_graph->instances.length;
    int i;
    aug_graph->graph = (EDGESET *)HALLOC(n*n*sizeof(EDGESET));
    for (i=n*n-1; i >= 0; --i) {
      aug_graph->graph[i] = NULL;
    }
  }

  /* initialize the edge set array */
  traverse_Block(get_edges,aug_graph,body);

  /* handle defaults */
  switch (Declaration_KEY(tlm)) {
  default: fatal_error("%d:unknown top-level-match",tnode_line_number(tlm));
  case KEYmodule_decl:
    break;
  case KEYsome_function_decl:
    {
      Type ftype=some_function_decl_type(tlm);
      /* int saved = analysis_debug;
       * analysis_debug = -1;
       */
      traverse_Type(get_edges,aug_graph,ftype);
      /* analysis_debug = saved;
       * print_aug_graph(aug_graph,0);
       */
    }
  case KEYtop_level_match:
    break;
  }

  if (aug_graph->first_rhs_decl != NULL) {
    VERTEX source;
    CONDITION cond;
    Declaration decl;
    source.node = aug_graph->lhs_decl;
    source.attr = phylum_shared_info_attribute
      (node_decl_phylum(aug_graph->lhs_decl),state);
    source.modifier = NO_MODIFIER;
    cond.positive = 0; cond.negative = 0;
    for (decl = aug_graph->first_rhs_decl;
	 decl != NULL;
	 decl = DECL_NEXT(decl)) {
      Declaration phy = node_decl_phylum(decl);
      if (phy != NULL) {
	VERTEX sink;
	sink.node = decl;
	sink.attr = phylum_shared_info_attribute(phy,state);
	sink.modifier = NO_MODIFIER;
	add_edges_to_graph(&source,&sink,&cond,dependency,aug_graph);
      }
    }
  }
  
  aug_graph->next_in_aug_worklist = NULL;
  aug_graph->schedule = (int *)HALLOC(aug_graph->instances.length*sizeof(int));
}

static void init_summary_dependency_graph(PHY_GRAPH *phy_graph,
					  Declaration phylum,
					  STATE *state)
{
  ATTRSET attrset=attrset_for(state,phylum);
  int count=0;
  phy_graph->phylum = phylum;
  phy_graph->global_state = state;
  { ATTRSET as=attrset;
    for (; as != NULL; as=as->rest) {
      FIBERSET fs;
      ++count; /* base attribute */
      for (fs = fiberset_for(as->attr,FIBERSET_NORMAL_FINAL); fs != NULL; fs=fs->rest) {
	++count;
      }
      for (fs = fiberset_for(as->attr,FIBERSET_REVERSE_FINAL); fs != NULL; fs=fs->rest) {
	++count;
      }
    }
  }
  VECTORALLOC(phy_graph->instances,INSTANCE,count);
  { int index=0;
    ATTRSET as=attrset;
    INSTANCE *array=phy_graph->instances.array;
    for (; as != NULL; as=as->rest) {
      FIBERSET fs;
      assign_instance(array,index++,as->attr,NULL,NULL); /* base attribute */
      for (fs=fiberset_for(as->attr,FIBERSET_NORMAL_FINAL); fs != NULL; fs=fs->rest) {
	assign_instance(array,index++,as->attr,fs->fiber,NULL);
      }
      for (fs=fiberset_for(as->attr,FIBERSET_REVERSE_FINAL); fs != NULL; fs=fs->rest) {
	assign_instance(array,index++,as->attr,fs->fiber,NULL);
      }
    }
  }
  { int total = count*count;
    int i;
    phy_graph->mingraph = (DEPENDENCY *)HALLOC(total*sizeof(DEPENDENCY));
    for (i=0; i < total; ++i) {
      phy_graph->mingraph[i] = no_dependency;
    }
  }
  phy_graph->next_in_phy_worklist = NULL;
  phy_graph->summary_schedule = (int *)HALLOC(count*sizeof(int));
}

static void init_analysis_state(STATE *s, Declaration module) {
  Declarations type_formals = module_decl_type_formals(module);
  Declarations decls = block_body(module_decl_contents(module));
  s->module = module;
  s->phylum_attrset_table = new_table();

  /* mark all local declarations such */
  traverse_Declaration(mark_local,module,module);

  /* get phyla (imported only) */
  { Declaration tf=first_Declaration(type_formals);
    Declarations edecls = NULL;
    while (tf != NULL && !TYPE_FORMAL_IS_EXTENSION(tf)) {
      tf = Declaration_info(tf)->next_decl;
    }
    if (tf != NULL) {
      Signature sig = some_type_formal_sig(tf);
      switch (Signature_KEY(sig)) {
      default:
	fatal_error("%d:cannot handle the signature of extension",
		    tnode_line_number(tf));
	break;
      case KEYsig_inst:
	/*! ignore is_input and is_var */
	{ Class cl = sig_inst_class(sig);
	  switch (Class_KEY(cl)) {
	  default: fatal_error("%d: bad class",tnode_line_number(cl));
	    break;
	  case KEYclass_use:
	    { Declaration d = Use_info(class_use_use(cl))->use_decl;
	      if (d == NULL) fatal_error("%d: class not found",
					 tnode_line_number(cl));
	      switch (Declaration_KEY(d)) {
	      default: fatal_error("%d: bad class_decl %s",
				   tnode_line_number(cl),
				   symbol_name(def_name(declaration_def(d))));
		break;
	      case KEYsome_class_decl:
		edecls = block_body(some_class_decl_contents(d));
		break;
	      }
	    }
	    break;
	  }
	}
	break;
      }
    }
    { int phyla_count = 0;
      if (edecls == NULL) {
	aps_error(module,"no extension to module %s",
		  symbol_name(def_name(declaration_def(module))));
      } else {
	Declaration edecl = first_Declaration(edecls);
	/*DEBUG fprintf(stderr,"got an extension!\n"); */
	for (; edecl != NULL; edecl = Declaration_info(edecl)->next_decl) {
	  switch (Declaration_KEY(edecl)) {
	  case KEYphylum_decl:
	    if (def_is_public(phylum_decl_def(edecl))) ++phyla_count;
	    if (DECL_IS_START_PHYLUM(edecl)) s->start_phylum = edecl;
	    break;
	  }
	}
      }
      if (s->start_phylum == NULL)
	fatal_error("no root_phylum indicated");
      /* we count functions and procedures as *both* phyla
       * and match rules (for convenience).  Here we count them as phyla:
       */
      { Declaration decl = first_Declaration(decls);
	for (; decl != NULL; decl = DECL_NEXT(decl)) {
	  switch (Declaration_KEY(decl)) {
	  case KEYsome_function_decl:
	    ++phyla_count;
	    break;
	  }
	}
      }
      VECTORALLOC(s->phyla,Declaration,phyla_count);
      phyla_count = 0;
      if (edecls != NULL) {
	Declaration edecl;
	for (edecl = first_Declaration(edecls);
	     edecl != NULL; edecl = Declaration_info(edecl)->next_decl) {
	  switch (Declaration_KEY(edecl)) {
	  case KEYphylum_decl:
	    if (def_is_public(phylum_decl_def(edecl)))  {
	      s->phyla.array[phyla_count++] = edecl;
	    }
	    break;
	  }
	}
      }
      { Declaration decl = first_Declaration(decls);
	for (; decl != NULL; decl = DECL_NEXT(decl)) {
	  switch (Declaration_KEY(decl)) {
	  case KEYsome_function_decl:
	    s->phyla.array[phyla_count++] = decl;
	    break;
	  }
	}
      }
    }
  }

  /* get match rules */
  { int match_rule_count = 0;
    Declaration decl = first_Declaration(decls);
    if (decl == NULL) aps_error(module,"empty module");
    for (; decl != NULL; decl = Declaration_info(decl)->next_decl) {
      switch (Declaration_KEY(decl)) {
      case KEYsome_function_decl:
      case KEYtop_level_match: ++match_rule_count; break;
	/*DEBUG
      case KEYdeclaration:
	fprintf(stderr,"Found decl: %s\n",
		symbol_name(def_name(declaration_def(decl))));
	break;
      default:
	fprintf(stderr,"Found something\n");
	break;*/
      }
    }
    VECTORALLOC(s->match_rules,Declaration,match_rule_count);
    match_rule_count=0;
    for (decl = first_Declaration(decls);
	 decl != NULL;
	 decl = Declaration_info(decl)->next_decl) {
      switch (Declaration_KEY(decl)) {
      case KEYsome_function_decl:
      case KEYtop_level_match:
	s->match_rules.array[match_rule_count++] = decl;
	break;
      }
    }
  }
  
  /* perform fibering */
  fiber_module(s->module,s);
  add_fibers_to_state(s);

  /* initialize attrset_table */
  { Declaration decl = first_Declaration(decls);
    for (; decl != NULL; decl = Declaration_info(decl)->next_decl) {
      switch (Declaration_KEY(decl)) {
      case KEYattribute_decl:
	if (!ATTR_DECL_IS_SYN(decl) && !ATTR_DECL_IS_INH(decl) &&
	    !FIELD_DECL_P(decl)) {
	  aps_error(decl,"%s not declared either synthesized or inherited",
		    decl_name(decl));
	  Declaration_info(decl)->decl_flags |= ATTR_DECL_SYN_FLAG;
	}
	{ Type ftype = attribute_decl_type(decl);
	  Declaration formal = first_Declaration(function_type_formals(ftype));
	  Type ntype = formal_type(formal);
	  switch (Type_KEY(ntype)) {
	  case KEYtype_use:
	    { Declaration phylum=Use_info(type_use_use(ntype))->use_decl;
	      if (phylum == NULL)
		fatal_error("%d: unknown phylum",tnode_line_number(ntype));
	      add_attrset_for(s,phylum,decl);
	    }
	    break;
	  }
	}
	break;
      case KEYsome_function_decl:
	/* The parameters are inherited attributes
	 * and the results are synthesized ones.
	 */
	{ Type ftype = some_function_decl_type(decl);
	  Declaration d;
	  for (d = first_Declaration(function_type_formals(ftype));
	       d != NULL;
	       d = DECL_NEXT(d)) {
	    Declaration_info(d)->decl_flags |= ATTR_DECL_INH_FLAG;
	    add_attrset_for(s,decl,d);
	  }
	  for (d = first_Declaration(function_type_return_values(ftype));
	       d != NULL;
	       d = DECL_NEXT(d)) {
	    Declaration_info(d)->decl_flags |= ATTR_DECL_SYN_FLAG;
	    add_attrset_for(s,decl,d);
	  }
	}
	break;
      }
    }
  }
  /* add special shared_info attributes */
  { int i;
    for (i=0; i < s->phyla.length; ++i) {
      Declaration phy = s->phyla.array[i];
      add_attrset_for(s,phy,phylum_shared_info_attribute(phy,s));
    }
  }

  /* initialize graphs */
  s->aug_graphs = (AUG_GRAPH *)HALLOC(s->match_rules.length*sizeof(AUG_GRAPH));
  s->phy_graphs = (PHY_GRAPH *)HALLOC(s->phyla.length*sizeof(PHY_GRAPH));
  { int i;
    for (i=0; i < s->match_rules.length; ++i) {
      init_augmented_dependency_graph(&s->aug_graphs[i],
				      s->match_rules.array[i],s);
    }
    init_augmented_dependency_graph(&s->global_dependencies,module,s);
    for (i=0; i < s->phyla.length; ++i) {
      init_summary_dependency_graph(&s->phy_graphs[i],
				    s->phyla.array[i],s);
    }
  } 
}


/*** ANALYSIS ***/

static void synchronize_dependency_graphs(AUG_GRAPH *aug_graph,
					  int start,
					  PHY_GRAPH *phy_graph) {
  int n=aug_graph->instances.length;
  int max;
  int phy_n;
  int i,j;

  if (phy_graph == NULL) {
    /* a semantic child of a constructor */
    return;
  }

  phy_n = phy_graph->instances.length;

  /* discover when the instances for this node end.
   */
  max = start + phy_n;
  
  for (i=start; i < max; ++i) {
    INSTANCE *source = &aug_graph->instances.array[i];
    INSTANCE *phy_source = &phy_graph->instances.array[i-start];
    if (!fibered_attr_equal(&source->fibered_attr,
			    &phy_source->fibered_attr)) {
      print_instance(source,stderr);
      fputs(" != ",stderr);
      print_instance(phy_source,stderr);
      fputc('\n',stderr);
      fatal_error("instances %s:%d vs %s:%d in different order",
		  aug_graph_name(aug_graph),i,
		  phy_graph_name(phy_graph),i-start);
    }
    for (j=start; j < max; ++j) {
      INSTANCE *sink = &aug_graph->instances.array[j];
      int aug_index = i*n + j;
      int sum_index = (i-start)*phy_n + (j-start);
      DEPENDENCY kind=edgeset_kind(aug_graph->graph[aug_index]);
      if (!AT_MOST(dependency_indirect(kind),
		   phy_graph->mingraph[sum_index])) {
	kind = dependency_indirect(kind); //! more precisely DNC artificial
	kind = dependency_join(kind,phy_graph->mingraph[sum_index]);
	if (kind == phy_graph->mingraph[sum_index]) {
	  fatal_error("kind computation broken");
	}
	if (analysis_debug & SUMMARY_EDGE) {
	  printf("Adding to summary edge %d: ",kind);
	  print_instance(source,stdout);
	  printf(" -> ");
	  print_instance(sink,stdout);
	  printf("\n");
	}
	if (analysis_debug & TWO_EDGE_CYCLE) {
	  if (phy_graph->mingraph[(j-start)*phy_n+(i-start)]) {
	    printf("Found summary two edge cycle: ");
	    print_instance(source,stdout);
	    printf(" <-> ");
	    print_instance(sink,stdout);
	    printf("\n");
	  }
	}
	phy_graph->mingraph[sum_index] = kind;
	/*?? put on a worklist somehow ? */
      } else if (!AT_MOST(phy_graph->mingraph[sum_index],
			  edgeset_lowerbound(aug_graph->graph[aug_index]))) {
	CONDITION cond;
	cond.positive=0; cond.negative=0;
	kind = dependency_join(kind,phy_graph->mingraph[sum_index]);
	if (analysis_debug & SUMMARY_EDGE_EXTRA) {
	  printf("Possibly adding summary edge %d: ",kind);
	  print_instance(source,stdout);
	  printf(" -> ");
	  print_instance(sink,stdout);
	  printf("\n");
	}
	add_edge_to_graph(source,sink,&cond,kind,aug_graph);
      }
    }
  }
}

static void augment_dependency_graph_for_node(AUG_GRAPH *aug_graph,
					      Declaration node) {
  int start=Declaration_info(node)->instance_index;
  PHY_GRAPH *phy_graph = Declaration_info(node)->node_phy_graph;

  synchronize_dependency_graphs(aug_graph,start,phy_graph);
}

/** Augment the dependencies between edges associated with a procedure call,
 * or a function call.
 * @see traverse_Declaration
 */
void *augment_dependency_graph_func_calls(void *paug_graph, void *node) {
  AUG_GRAPH *aug_graph = (AUG_GRAPH *)paug_graph;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  default:
    break;
  case KEYExpression:
    {
      Expression e = (Expression)node;
      Declaration fdecl = 0;
      if ((fdecl = local_call_p(e)) != NULL &&
	  Declaration_KEY(fdecl) == KEYfunction_decl) {
	Declaration proxy = Expression_info(e)->funcall_proxy;
	if (proxy == NULL)
	  fatal_error("missing funcall proxy");
	augment_dependency_graph_for_node(aug_graph,proxy);
      }
    }
    break;
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
      case KEYsome_function_decl:
      case KEYtop_level_match:
	/* don't look inside (unless its what we're doing the analysis for) */
	if (aug_graph->match_rule != node) return NULL;
	break;
      case KEYassign:
	{ Declaration pdecl = proc_call_p(assign_rhs(decl));
	  if (pdecl != NULL) {
	    augment_dependency_graph_for_node(aug_graph,decl);
	  }
	}
	break;
      }
    }
    break;
  }
  return paug_graph;
}

/* copy in (and out!) summary dependencies */
void augment_dependency_graph(AUG_GRAPH *aug_graph) {
  Declaration rhs_decl;
  switch (Declaration_KEY(aug_graph->match_rule)) {
  default:
    fatal_error("unexpected match rule");
    break;
  case KEYmodule_decl:
    augment_dependency_graph_for_node(aug_graph,aug_graph->match_rule);
    break;
  case KEYsome_function_decl:
    augment_dependency_graph_for_node(aug_graph,aug_graph->match_rule);
    break;
  case KEYtop_level_match:
    augment_dependency_graph_for_node(aug_graph,aug_graph->lhs_decl);
    for (rhs_decl = aug_graph->first_rhs_decl;
	 rhs_decl != NULL;
	 rhs_decl = Declaration_info(rhs_decl)->next_decl) {
      augment_dependency_graph_for_node(aug_graph,rhs_decl);
    }
    break;
  }
  /* find procedure calls */
  traverse_Declaration(augment_dependency_graph_func_calls,
		       aug_graph,aug_graph->match_rule);
}

void close_using_edge(AUG_GRAPH *aug_graph, EDGESET edge) {
  int i,j;
  int source_index = edge->source->index;
  int sink_index = edge->sink->index;
  int n=aug_graph->instances.length;

  if (analysis_debug & CLOSE_EDGE) {
    printf("Closing with ");
    print_edge(edge,stdout);
  }

  for (i=0; i < n; ++i) {
    EDGESET e;
    /* first: instance[i]->source */
    for (e = aug_graph->graph[i*n+source_index];
	 e != NULL;
	 e = e->rest) {
      add_transitive_edge_to_graph(e->source,edge->sink,
				   &e->cond,&edge->cond,
				   e->kind,edge->kind,
				   aug_graph);
    }
    /* then sink->instance[i] */
    for (e = aug_graph->graph[sink_index*n+i];
	 e != NULL;
	 e = e->rest) {
      add_transitive_edge_to_graph(edge->source,e->sink,
				   &edge->cond,&e->cond,
				   edge->kind,e->kind,
				   aug_graph);
    }
  }
}

/* A very slow check, hence optional.
 * O(n^3*2^c) where 'n' is the number of instances.
 * (for reasonable non-toy examples, n can be > 100).
 * The 2^c term rarely contributes much.
 * Activate with -D0 (zero)
 */
void assert_closed(AUG_GRAPH *aug_graph) {
  int n = aug_graph->instances.length;
  int n2 = n*n;
  int i;

  assert(aug_graph->worklist_head == NULL);

  for (i=0; i < n2; ++i) {
    EDGESET es = aug_graph->graph[i];
    for (; es != NULL; es = es->rest) {
      close_using_edge(aug_graph,es);
      assert(aug_graph->worklist_head == NULL);
    }
  }
}

/* return whether any changes were noticed */
BOOL close_augmented_dependency_graph(AUG_GRAPH *aug_graph) {
  augment_dependency_graph(aug_graph);
  if (aug_graph->worklist_head == NULL) {
    if (analysis_debug & DNC_ITERATE)
      printf("Worklist is empty\n");
    if (analysis_debug & ASSERT_CLOSED) assert_closed(aug_graph);
    return FALSE;
  }

  while (aug_graph->worklist_head != NULL) {
    while (aug_graph->worklist_head != NULL) {
      EDGESET edge=aug_graph->worklist_head;
      aug_graph->worklist_head = edge->next_in_edge_worklist;
      if (aug_graph->worklist_tail == edge) {
	if (edge->next_in_edge_worklist != NULL)
	  fatal_error("worklist out of whack!");
	aug_graph->worklist_tail = NULL;
      }
      edge->next_in_edge_worklist = NULL;
      close_using_edge(aug_graph,edge);
    }
    
    augment_dependency_graph(aug_graph);
  }

  if (analysis_debug & ASSERT_CLOSED) assert_closed(aug_graph);
  return TRUE;
}

/**
 * Ensure that the summary dependency graph is transitively closed.
 * <bf>!! Profiling indicates that about 23% of the time
 * to analyze cool-semant (with fibering) is taken up in this function!</bf>
 * The remaining <em>hot</em> functions are
 * <dl>
 * <dt>close_using_edge<dd> 14%
 * <dt>dependency_trans<dd> 13%
 * <dt>synchronize_dependency_graphs<dd> 9%
 * <dt>schedule_rest<dd> 5%
 * </dl>
 * @return whether any changes were noticed 
 */
BOOL close_summary_dependency_graph(PHY_GRAPH *phy_graph) {
  int i,j,k;
  int n = phy_graph->instances.length;
  BOOL any_changed = FALSE;
  BOOL changed;
  do {
    changed = FALSE;
    /* no worklists, just a dumb in-place matrix squaring */
    for (i=0; i < n; ++i) {
      for (j=0; j < n; ++j) {
	DEPENDENCY ij=phy_graph->mingraph[i*n+j];
	if (ij != max_dependency) {
	  /* maybe could be made stronger */
	  for (k=0; k<n; ++k) {
	    DEPENDENCY ik = phy_graph->mingraph[i*n+k];
	    DEPENDENCY kj = phy_graph->mingraph[k*n+j];
	    DEPENDENCY tmpij = dependency_trans(ik,kj);
	    if (!AT_MOST(tmpij,ij)) {
	      ij=dependency_join(tmpij,ij);
	      changed = TRUE;
	      any_changed = TRUE;
	      phy_graph->mingraph[i*n+j] = ij;
	      if (ij == max_dependency) break;
	    }
	  }
	}
      }
    }
  } while (changed);
  return any_changed;
}

DEPENDENCY analysis_state_cycle(STATE *s) {
  int i,j;
  DEPENDENCY kind = no_dependency;
  /** test for cycles **/
  for (i=0; i < s->phyla.length; ++i) {
    PHY_GRAPH *phy_graph = &s->phy_graphs[i];
    int n = phy_graph->instances.length;
    for (j=0; j < n; ++j) {
      DEPENDENCY k1 = phy_graph->mingraph[j*n+j];
      kind = dependency_join(kind,k1);
    }
  }
  for (i=0; i < s->match_rules.length; ++i) {
    AUG_GRAPH *aug_graph = &s->aug_graphs[i];
    int n = aug_graph->instances.length;
    for (j=0; j < n; ++j) {
      DEPENDENCY k1 = edgeset_kind(aug_graph->graph[j*n+j]);
      kind = dependency_join(kind,k1);
    }
  }
  return kind;
}

void dnc_close(STATE*s) {
  int i,j;
  BOOL changed;

  for (i=0; ; ++i) {
    if (analysis_debug & DNC_ITERATE) {
      printf("*** AFTER %d ITERATIONS ***\n",i);
      print_analysis_state(s,stdout);
    }
    changed = FALSE;
    for (j=0; j < s->match_rules.length; ++j) {
      if (analysis_debug & DNC_ITERATE) {
	printf("Checking rule %d\n",j);
      }
      changed |= close_augmented_dependency_graph(&s->aug_graphs[j]);
    }
    changed |= close_augmented_dependency_graph(&s->global_dependencies);
    for (j=0; j < s->phyla.length; ++j) {
      if (analysis_debug & DNC_ITERATE) {
	printf("Checking phylum %s\n",
	       symbol_name(def_name(declaration_def(s->phyla.array[j]))));
      }
      changed |= close_summary_dependency_graph(&s->phy_graphs[j]);
    }
    if (!changed) break;
  }
}

STATE *compute_dnc(Declaration module) {
  STATE *s=(STATE *)HALLOC(sizeof(STATE));
  Declaration_info(module)->analysis_state = s;
  init_analysis_state(s,module);
  dnc_close(s);
  if (analysis_debug & (DNC_ITERATE|DNC_FINAL)) {
    printf("*** FINAL DNC STATE ***\n");
    print_analysis_state(s,stdout);
    print_cycles(s,stdout);
  }
  return s;
}


/*** DEBUGGING OUTPUT ***/

void print_attrset(ATTRSET s, FILE *stream) {
  if (stream == 0) stream = stdout;
  fputc('{',stream);
  while (s != NULL) {
    fputs(symbol_name(def_name(declaration_def(s->attr))),stream);
    s=s->rest;
    if (s != NULL) fputc(',',stream);
  }
  fputc('}',stream);
}

void print_dep_vertex(VERTEX *v, FILE *stream)
{
  INSTANCE fake;
  MODIFIER* mod = v->modifier;
  fake.node = v->node;
  fake.fibered_attr.attr = v->attr;
  fake.fibered_attr.fiber = 0;
  print_instance(&fake,stream);
  while (mod != NO_MODIFIER) {
    fputs("#",stream);
    fputs(decl_name(mod->field),stream);
    mod = mod->next;
  }
}
 
void print_instance(INSTANCE *i, FILE *stream) {
  if (stream == 0) stream = stdout;
  if (i->node != NULL) {
    if (ABSTRACT_APS_tnode_phylum(i->node) != KEYDeclaration) {
      fprintf(stream,"%d:?<%d>",tnode_line_number(i->node),
	      ABSTRACT_APS_tnode_phylum(i->node));
    } else if (Declaration_KEY(i->node) == KEYnormal_assign) {
      Declaration pdecl = proc_call_p(normal_assign_rhs(i->node));
      fprintf(stream,"%s(...)@%d",decl_name(pdecl),
	      tnode_line_number(i->node));
    } else if (Declaration_KEY(i->node) == KEYpragma_call) {
      fprintf(stream,"%s(...):%d",symbol_name(pragma_call_name(i->node)),
	      tnode_line_number(i->node));
    } else {
      fputs(symbol_name(def_name(declaration_def(i->node))),stream);
    }
    fputc('.',stream);
  }
  if (i->fibered_attr.attr == NULL) {
    fputs("(nil)",stream);
  } else if (ABSTRACT_APS_tnode_phylum(i->fibered_attr.attr) == KEYMatch) {
    fprintf(stream,"<match@%d>",tnode_line_number(i->fibered_attr.attr));
  } else switch(Declaration_KEY(i->fibered_attr.attr)) {
  case KEYcollect_assign: {
    Expression lhs = collect_assign_lhs(i->node);
    Declaration field = field_ref_p(lhs);
    fprintf(stream,"[%d:?.",tnode_line_number(i->fibered_attr.attr));
    fputs(symbol_name(def_name(declaration_def(field))),stream);
    fputs(":>?]",stream);
  }
  case KEYif_stmt:
  case KEYcase_stmt:
    fprintf(stream,"<cond@%d>",tnode_line_number(i->fibered_attr.attr));
    break;
  default:
    fputs(symbol_name(def_name(declaration_def(i->fibered_attr.attr))),stream);
  }
  if (i->fibered_attr.fiber != NULL) {
    fputc('$',stream);
    print_fiber(i->fibered_attr.fiber,stream);
  }
}

void print_edge_helper(DEPENDENCY kind, CONDITION *cond, FILE* stream) {
  if (stream == 0) stream = stdout;
  if ((kind & DEPENDENCY_MAYBE_SIMPLE) == DEPENDENCY_MAYBE_SIMPLE) fputc('s', stream);
  else {
    fputc('X', stream);
  }
  switch (kind) {
  default: fprintf(stream,"?%d",kind); break;
  case no_dependency: fputc('!',stream); break;
  case indirect_control_fiber_dependency:
  case indirect_fiber_dependency: fputc('?',stream); /* fall through */
  case control_fiber_dependency:
  case fiber_dependency: fputc('(',stream); break;
  case indirect_control_dependency:
  case indirect_dependency: fputc('?',stream); /* fall through */
  case control_dependency:
  case dependency: break;
  }
  if (cond != NULL) print_condition(cond,stream);
  if ((kind & DEPENDENCY_NOT_JUST_FIBER) == 0) {
    fputc(')',stream);
  }
}

void print_edge(EDGESET e, FILE *stream) {
  if (stream == 0) stream = stdout;
  print_instance(e->source,stream);
  fputs("->",stream);
  print_instance(e->sink,stream);
  fputc(':',stream);
  print_edge_helper(e->kind,&e->cond,stream);
  fputc('\n',stream);
}
  
void print_edgeset(EDGESET e, FILE *stream) {
  if (stream == 0) stream = stdout;
  if (e != NULL) {
    EDGESET tmp=e;
    print_instance(e->source,stream);
    fputs("->",stream);
    print_instance(e->sink,stream);
    fputc(':',stream);
    while (tmp != NULL) {
      if (tmp->source != e->source) {
	fputs("!!SOURCE=",stream);
	print_instance(tmp->source,stream);
      }
      if (tmp->sink != e->sink) {
	fputs("!!SINK=",stream);
	print_instance(tmp->sink,stream);
      }
      print_edge_helper(tmp->kind,&tmp->cond,stream);
      tmp = tmp->rest;
      if (tmp != NULL) fputc(',',stream);
    }
    fputc('\n',stream);
  }
}

const char *aug_graph_name(AUG_GRAPH *aug_graph) {
  switch (Declaration_KEY(aug_graph->match_rule)) {
  case KEYtop_level_match:
    { Pattern pat=matcher_pat(top_level_match_m(aug_graph->match_rule));
      switch (Pattern_KEY(pat)) {
      case KEYand_pattern:
	pat = and_pattern_p2(pat);
      }
      switch (Pattern_KEY(pat)) {
      case KEYpattern_call:
	switch (Pattern_KEY(pattern_call_func(pat))) {
	case KEYpattern_use:
	  { Declaration decl =
	      Use_info(pattern_use_use(pattern_call_func(pat)))->use_decl;
	    if (decl != NULL)
	      return symbol_name(def_name(declaration_def(decl)));
	    else
	      return "unbound pattern use";
	  }
	default:
	  return "unknown pattern function";
	}
      default:
	return "unknown pattern";
      }
    }
    break;
  case KEYfunction_decl:
  case KEYprocedure_decl:
    return symbol_name(def_name(declaration_def(aug_graph->match_rule)));
  case KEYmodule_decl:
    return "global dependencies";
  default:
    return "unknown production";
  }
}

void print_aug_graph(AUG_GRAPH *aug_graph, FILE *stream) {
  if (stream == 0) stream = stdout;
  fputs("Augmented dependency graph for ",stream);
  switch (Declaration_KEY(aug_graph->match_rule)) {
  case KEYtop_level_match:
    { Pattern pat=matcher_pat(top_level_match_m(aug_graph->match_rule));
      switch (Pattern_KEY(pat)) {
      case KEYand_pattern:
	pat = and_pattern_p2(pat);
      }
      switch (Pattern_KEY(pat)) {
      case KEYpattern_call:
	switch (Pattern_KEY(pattern_call_func(pat))) {
	case KEYpattern_use:
	  print_Use(pattern_use_use(pattern_call_func(pat)),stdout);
#ifdef UNDEF
	  { Declaration decl =
	      Use_info(pattern_use_use(pattern_call_func(pat)))->use_decl;
	    if (decl != NULL)
	      fputs(symbol_name(def_name(declaration_def(decl))),stream);
	    else
	      fputs("unbound pattern use",stream);
	  }
#endif
	break;
	default:
	  fputs("unknown pattern function",stream);
	}
	break;
      default:
	fputs("unknown pattern",stream);
      }
    }
    break;
  case KEYfunction_decl:
  case KEYprocedure_decl:
    fputs(symbol_name(def_name(declaration_def(aug_graph->match_rule))),
	  stream);
    break;
  case KEYmodule_decl:
    fputs("global dependencies",stream);
    break;
  default:
    fputs("unknown production",stream);
  }
  fputs("\n",stream);
  { int n = aug_graph->instances.length;
    int max = n*n;
    int i;
    for (i=0; i < n; ++i) {
      if (i == 0) printf(" (");
      else printf(",");
      print_instance(&aug_graph->instances.array[i],stream);
    }
    printf(")\n");
    for (i=0; i < max; ++i) {
      print_edgeset(aug_graph->graph[i],stream);
    }
  }
  fputc('\n',stream);
}

const char *phy_graph_name(PHY_GRAPH *phy_graph) {
  return symbol_name(def_name(declaration_def(phy_graph->phylum)));
}

void print_phy_graph(PHY_GRAPH *phy_graph, FILE *stream) {
  int i=0;
  int j=0;
  int n=phy_graph->instances.length;
  
  if (stream == 0) stream = stdout;
  fprintf(stream,"\nSummary dependency graph for %s\n",
	  symbol_name(def_name(declaration_def(phy_graph->phylum))));
  for (i=0; i < n; ++i) {
    print_instance(&phy_graph->instances.array[i],stream);
    fputs(" -> ",stream);
    for (j=0; j < n; ++j) {
      DEPENDENCY kind= phy_graph->mingraph[i*n+j];
      if (kind == no_dependency) continue;
      if (kind == fiber_dependency) fputc('(',stream);
      print_instance(&phy_graph->instances.array[j],stream);
      if (kind == fiber_dependency) fputc(')',stream);
      fputc(' ',stream);
    }
    fputc('\n',stream);
  }
}

void print_analysis_state(STATE *s, FILE *stream) {
  int i;
  if (stream == 0) stream = stdout;
  fprintf(stream,"Analysis state for %s\n",
	  symbol_name(def_name(declaration_def(s->module))));
  print_aug_graph(&s->global_dependencies,stream);
  for (i=0; i < s->match_rules.length; ++i) {
    print_aug_graph(&s->aug_graphs[i],stream);
  }
  for (i=0; i < s->phyla.length; ++i) {
    print_phy_graph(&s->phy_graphs[i],stream);
  }
}

void print_cycles(STATE *s, FILE *stream) {
  int i,j;
  if (stream == 0) stream = stdout;
  /** test for cycles **/
  for (i=0; i < s->phyla.length; ++i) {
    BOOL fiber_cycle = FALSE;
    PHY_GRAPH *phy_graph = &s->phy_graphs[i];
    int n = phy_graph->instances.length;
    for (j=0; j < n; ++j) {
      switch (phy_graph->mingraph[j*n+j]) {
      case no_dependency: break;
      case indirect_control_fiber_dependency:
      case control_fiber_dependency:
      case indirect_fiber_dependency:
      case fiber_dependency:
      case indirect_circular_dependency:
	fprintf(stream,"fiber ");
	/* fall through */
      default:
	fprintf(stream,"summary cycle involving %s.",
		symbol_name(def_name(declaration_def(phy_graph->phylum))));
	print_instance(&phy_graph->instances.array[j],stdout);
	fprintf(stream,"\n");
	break;
      }
    }
  }
  for (i=0; i < s->match_rules.length; ++i) {
    AUG_GRAPH *aug_graph = &s->aug_graphs[i];
    int n = aug_graph->instances.length;
    for (j=0; j < n; ++j) {
      switch (edgeset_kind(aug_graph->graph[j*n+j])) {
      case no_dependency: break;
      case indirect_control_fiber_dependency:
      case control_fiber_dependency:
      case indirect_fiber_dependency:
      case fiber_dependency:
      case indirect_circular_dependency:
	fprintf(stream,"fiber ");
	/* fall through */
      default:
	fprintf(stream,"%d local cycle for %s involving ", (edgeset_kind(aug_graph->graph[j*n+j])),
		aug_graph_name(aug_graph));
	print_instance(&aug_graph->instances.array[j],stdout);
	fprintf(stream,"\n");
	break;
      }
    }
  }
}
