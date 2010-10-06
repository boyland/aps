/*
 * APS-CYCLE
 *
 * Routines for breaking cycles involving only fibers.
 */

#include <stdio.h>
#include <jbb.h>
#include "jbb-alloc.h"
#include "aps-ag.h"

int cycle_debug = 0;

/* We use a union-find algorithm to detect strongly connected components.
 * We use a dynamically allocated array to hold the pointers,
 * and two others to find where the instances for each dependency graph
 * (both for productions and for phyla).
 */

static int num_instances;
static int *parent_index; /* initializes to pi[i] = i */
static int *constructor_instance_start;
static int *phylum_instance_start;

static void init_indices(STATE *s) {
  int num = 0;
  int i = 0;
  constructor_instance_start = (int *)SALLOC(sizeof(int)*(s->match_rules.length+1));
  phylum_instance_start = (int *)SALLOC(sizeof(int)*s->phyla.length);
  for (i = 0; i < s->phyla.length; ++i) {
    phylum_instance_start[i] = num;
    num += s->phy_graphs[i].instances.length;
  }
  for (i = 0; i < s->match_rules.length; ++i) {
    constructor_instance_start[i] = num;
    num += s->aug_graphs[i].instances.length;
  }
  constructor_instance_start[i] = num;
  num += s->global_dependencies.instances.length;
  num_instances = num;
  parent_index = SALLOC(sizeof(int)*num);
  for (i=0; i < num; ++i) {
    parent_index[i] = i;
  }
}

static int get_set(int index) {
  int pindex= parent_index[index];
  if (index == -1 || pindex == -1) fatal_error("mixup");
  if (pindex == index) return index;
  else {
    int s = get_set(pindex);
    parent_index[index] = s;
    return s;
  }
}

static int merge_sets(int index1, int index2) {
  if (parent_index[index1] == -1) /* non normally cyclic */
    parent_index[index1] = get_set(index2);
  else
    parent_index[get_set(index1)] = get_set(index2);
}

typedef VECTOR(int) SETS;

static void get_fiber_cycles(STATE *s) {
  int i,j,k;
  int num_sets = 0;
  for (i=0; i < num_instances; ++i) {
    if (parent_index[i] == i) ++num_sets;
  }
  VECTORALLOC(s->cycles,CYCLE,num_sets);
  num_sets=0;
  for (i=0; i < num_instances; ++i) {
    if (parent_index[i] == i) {
      INSTANCE *iarray;
      int count = 0;
      CYCLE *cyc = &s->cycles.array[num_sets++];
      cyc->internal_info = i;
      for (j=0; j < num_instances; ++j) {
	if (parent_index[j] >= 0 && get_set(j) == i) ++count;
      }
      VECTORALLOC(cyc->instances,INSTANCE,count);
      iarray = cyc->instances.array;
      count = 0;
      for (j=0; j < s->phyla.length; ++j) {
	int phylum_index = phylum_instance_start[j];
	PHY_GRAPH *phy = &s->phy_graphs[j];
	for (k = 0; k < phy->instances.length; ++k) {
	  if (parent_index[phylum_index+k] == i) {
	    iarray[count++] = phy->instances.array[k];
	  }
	}
      }
      for (j=0; j <= s->match_rules.length; ++j) {
	int constructor_index = constructor_instance_start[j];
	AUG_GRAPH *aug_graph =
	  (j == s->match_rules.length) ?
	    &s->global_dependencies :
	      &s->aug_graphs[j];
	for (k = 0; k < aug_graph->instances.length; ++k) {
	  if (parent_index[constructor_index+k] == i) {
	    iarray[count++] = aug_graph->instances.array[k];
	  }
	}
      }
      if (count != cyc->instances.length) {
	fatal_error("Counted %d instances in cycle, now have %d\n",
		    cyc->instances.length,count);
      }
    }
  }
  if (cycle_debug & PRINT_UP_DOWN) {
    printf("%d independent fiber cycle%s found\n",num_sets,num_sets>1?"s":"");
    for (i=0; i < s->cycles.length; ++i) {
      CYCLE *cyc = &s->cycles.array[i];
      printf("Cycle %d:\n",i);
      for (j = 0; j < cyc->instances.length; ++j) {
	printf("  ");
	print_instance(&cyc->instances.array[j],stdout);
	printf("\n");
      }
    }
  }
}


/*** determing strongly connected sets of attributes ***/

static void make_augmented_cycles_for_node(AUG_GRAPH *aug_graph,
					   int constructor_index,
					   Declaration node)
{
  int start=Declaration_info(node)->instance_index;
  int n=aug_graph->instances.length;
  int max=n;
  int phy_n;
  int i,j;
  int phylum_index;
  STATE *s = aug_graph->global_state;
  PHY_GRAPH *phy_graph = Declaration_info(node)->node_phy_graph;

  if (phy_graph == NULL) {
    /* a semantic child of a constructor */
    /* printf("No summary graph for %s\n",decl_name(node)); */
    return;
  }

  phy_n = phy_graph->instances.length;
  phylum_index = phylum_instance_start[phy_graph - s->phy_graphs];

  /* discover when the instances for this node end.
   */
  for (i=start; i < max; ++i) {
    if (aug_graph->instances.array[i].node != node) {
      /* DEBUG
       *print_instance(&aug_graph->instances.array[i],stdout);
       *printf(" does not refer to %s\n",
       *     symbol_name(def_name(declaration_def(node))));
       */
      max = i;
      break;
    }
  }

  for (i=start; i < max; ++i) {
    int phy_i = i-start;
    if (phy_graph->mingraph[phy_i*phy_n+phy_i] != no_dependency)
      merge_sets(constructor_index+i,phylum_index+phy_i);
  }
}


void *make_augmented_cycles_func_calls(void *paug_graph, void *node) {
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
	/* need to figure out constructor index */
	int i;
	for (i=0; i<aug_graph->global_state->match_rules.length; ++i)
	  if (aug_graph == &aug_graph->global_state->aug_graphs[i])
	    break;
	if (proxy == NULL)
	  fatal_error("missing funcall proxy");
	make_augmented_cycles_for_node(aug_graph,
				       constructor_instance_start[i],
				       proxy);
      }
    }
    break;
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
      default:
	break;
      case KEYsome_function_decl:
      case KEYtop_level_match:
	/* don't look inside (unless its what we're doing the analysis for) */
	if (aug_graph->match_rule != node) return NULL;
	break;
      case KEYassign:
	{ Declaration pdecl = proc_call_p(assign_rhs(decl));
	  if (pdecl != NULL) {
	    /* need to figure out constructor index */
	    int i;
	    for (i=0; i<aug_graph->global_state->match_rules.length; ++i)
	      if (aug_graph == &aug_graph->global_state->aug_graphs[i])
		break;
	    /* if we never take the break, we must have the global
	     * dependencies augmented dependency graph.
	     */
	    make_augmented_cycles_for_node(aug_graph,
					   constructor_instance_start[i],
					   decl);
	  }
	}
	break;
      }
    }
    break;
  }
  return paug_graph;
}

static void make_augmented_cycles(AUG_GRAPH *aug_graph, int constructor_index)
{
  Declaration rhs_decl;
  switch (Declaration_KEY(aug_graph->match_rule)) {
  default:
    fatal_error("unexpected match rule");
    break;
  case KEYmodule_decl:
    make_augmented_cycles_for_node(aug_graph,constructor_index,
				   aug_graph->match_rule);
    break;
  case KEYsome_function_decl:
    make_augmented_cycles_for_node(aug_graph,constructor_index,
				   aug_graph->match_rule);
    break;
  case KEYtop_level_match:
    make_augmented_cycles_for_node(aug_graph,constructor_index,
				   aug_graph->lhs_decl);
    for (rhs_decl = aug_graph->first_rhs_decl;
	 rhs_decl != NULL;
	 rhs_decl = Declaration_info(rhs_decl)->next_decl) {
      make_augmented_cycles_for_node(aug_graph,constructor_index,rhs_decl);
    }
    break;
  }
  /* find procedure calls */
  traverse_Declaration(make_augmented_cycles_func_calls,
		       aug_graph,aug_graph->match_rule);
  
}

static void make_cycles(STATE *s) {
  int i,j,k;
  /* summary cycles */
  for (i = 0; i < s->phyla.length; ++i) {
    PHY_GRAPH *phy = &s->phy_graphs[i];
    int n = phy->instances.length;
    int phylum_index = phylum_instance_start[i];
    for (j = 0; j < n; ++j) {
      if (phy->mingraph[j*n+j] == no_dependency) {
	/* not in a cycle */
	parent_index[phylum_index+j] = -1;
      } else {
	/* in a cycle */
	for (k = 0; k < n; ++k) {
	  if (k != j &&
	      phy->mingraph[j*n+k] != no_dependency &&
	      phy->mingraph[k*n+j] != no_dependency) {
	    merge_sets(phylum_index+j,phylum_index+k);
	  }
	}
      }
    }
  }
  /* augmented dependency graph */
  for (i = 0; i <= s->match_rules.length; ++i) {
    AUG_GRAPH *aug_graph =
      (i == s->match_rules.length) ?
	&s->global_dependencies :
	  &s->aug_graphs[i];
    int n = aug_graph->instances.length;
    int constructor_index = constructor_instance_start[i];
    for (j = 0; j < n; ++j) {
      if (edgeset_kind(aug_graph->graph[j*n+j]) == no_dependency) {
	/* not in a cycle */
	parent_index[constructor_index+j] = -1;
      } else {
	/* in a cycle */
	for (k = 0; k < n; ++k) {
	  if (k != j &&
	      edgeset_kind(aug_graph->graph[j*n+k]) != no_dependency &&
	      edgeset_kind(aug_graph->graph[k*n+j]) != no_dependency) {
	    merge_sets(constructor_index+j,constructor_index+k);
	  }
	}
      }
    }
    make_augmented_cycles(aug_graph,constructor_index);
  }
}


/*** Add new instances and redo dependency graphs ***/

SYMBOL make_up_down_name(const char *n, int num,BOOL up) {
  char name[80];
  sprintf(name,"%s[%s]-%d",up?"UP":"DOWN",n,num);
  /* printf("Creating symbol %s\n",name); */
  return intern_symbol(name);
}

static char danger[1000];

static const char *phylum_to_string(Declaration d)
{
  switch (Declaration_KEY(d)) {
  default:
    return decl_name(d);
  case KEYpragma_call:
    sprintf(danger,"%s:%d",symbol_name(pragma_call_name(d)),
	    tnode_line_number(d));
    return danger;
  case KEYif_stmt:
    sprintf(danger,"if:%d",tnode_line_number(d));
    return danger;
  }
}

static void add_up_down_attributes(STATE *s) {
  int i,j,k,l;
  CONDITION cond;
  cond.positive=0;
  cond.negative=0;
  for (i=0; i < s->cycles.length; ++i) {
    CYCLE *cyc = &s->cycles.array[i];
    for (j=0; j < s->phyla.length; ++j) {
      int found = 0;
      PHY_GRAPH *phy = &s->phy_graphs[j];
      int n = phy->instances.length;
      int phylum_index = phylum_instance_start[j];
      INSTANCE *array = phy->instances.array;
      int upindex, downindex;
      Declaration upattr =
	attribute_decl(def(make_up_down_name(phylum_to_string(s->phyla.array[j]),
					     i,TRUE),
			   FALSE,FALSE),
		       no_type(), /* sloppy */
		       direction(FALSE,FALSE,FALSE),no_default());
      Declaration downattr =
	attribute_decl(def(make_up_down_name(phylum_to_string(s->phyla.array[j]),
					     i,FALSE),
			   FALSE,FALSE),
		       no_type(), /* sloppy */
		       direction(FALSE,FALSE,FALSE),no_default());
      Declaration_info(upattr)->decl_flags =
	ATTR_DECL_SYN_FLAG|DECL_LOCAL_FLAG|SHARED_DECL_FLAG|UP_DOWN_FLAG;
      Declaration_info(downattr)->decl_flags =
	ATTR_DECL_INH_FLAG|DECL_LOCAL_FLAG|SHARED_DECL_FLAG|UP_DOWN_FLAG;
      for (k=0; k < n; ++k) {
	if (parent_index[k+phylum_index] == cyc->internal_info) {
	  switch (++found) {
	  case 1:
	    array[k].fibered_attr.attr = upattr;
	    Declaration_info(upattr)->instance_index = upindex = k;
	    break;
	  case 2:
	    array[k].fibered_attr.attr = downattr;
	    Declaration_info(downattr)->instance_index = downindex = k;
	    break;
	  default:
	    array[k].fibered_attr.attr = NULL;
	    break;
	  }
	  array[k].fibered_attr.fiber = NULL;
	}
      }
      if (found > 0) {
	if (found < 2) fatal_error("Not enough attributes to make cycle");
	/* redo dependencies */
	for (k=0; k < n; ++k) {
	  BOOL kcycle = parent_index[k+phylum_index] == cyc->internal_info;
	  if (k != downindex) {
	    for (l=0; l < n; ++l) {
	      BOOL lcycle = parent_index[l+phylum_index] == cyc->internal_info;
	      if (l != upindex &&
		  phy->mingraph[k*n+l] != no_dependency) {
		if (kcycle || lcycle) {
		  phy->mingraph[k*n+l] = no_dependency;
		  if (!lcycle) {
		    phy->mingraph[downindex*n+l] = fiber_dependency;
		  } else if (!kcycle) {
		    phy->mingraph[k*n+upindex] = fiber_dependency;
		  }
		}
	      }
	    }
	  }
	}
	phy->mingraph[upindex*n+upindex] = no_dependency;
	phy->mingraph[upindex*n+downindex] = fiber_dependency; /* set! */
	phy->mingraph[downindex*n+upindex] = no_dependency;
	phy->mingraph[downindex*n+downindex] = no_dependency;
      }
    } /* for phyla */
    for (j = 0; j <= s->match_rules.length; ++j) {
      AUG_GRAPH *aug_graph =
	(j == s->match_rules.length) ?
	  &s->global_dependencies :
	    &s->aug_graphs[j];
      int n = aug_graph->instances.length;
      int constructor_index = constructor_instance_start[j];
      INSTANCE *array = aug_graph->instances.array;
      int cycle_type = 0;
      int upindex = -1, downindex = -1;
      int start = 0;
      int found = 0;
      Declaration lastnode = NULL;
      Declaration updecl, downdecl;
      for (k=0; k < n; ++k) {
	if (parent_index[k+constructor_index] == cyc->internal_info) {
	  ++found;
	  if (array[k].node == NULL)
	    cycle_type |= CYC_LOCAL;
	  else if (DECL_IS_LHS(array[k].node))
	    cycle_type |= CYC_ABOVE;
	  else if (DECL_IS_RHS(array[k].node))
	    cycle_type |= CYC_BELOW;
	  else
	    fatal_error("Cannot classify node: %s",phylum_to_string(array[k].node));
	}
      }
      if (cycle_type == CYC_BELOW) {
	/* special case if all cycles are below:
	 * a different cycle for every node.
	 */
	if (found != 2)
	  fatal_error("Cannot handle cycle below case in general yet!");
	found = 0;
	for (k=0; k < n; ++k) {
	  if (array[k].node != lastnode) {
	    start = k;
	    lastnode = array[k].node;
	  }	  
	  if (parent_index[k+constructor_index] == cyc->internal_info) {
	    PHY_GRAPH *phy_graph =
	      Declaration_info(array[k].node)->node_phy_graph;
	    array[k].fibered_attr.attr =
	      phy_graph->instances.array[k-start].fibered_attr.attr;
	    switch (++found) {
	    case 1: upindex = k; break;
	    case 2: downindex = k; break;
	    default: break;
	    }
	    array[k].fibered_attr.fiber = NULL;
	  }
	}
	if (found != 2)
	  fatal_error("Counted twice gets different numbers!");
	if (downindex != upindex) {
	  free_edgeset(aug_graph->graph[upindex*n+upindex],aug_graph);
	  add_edge_to_graph(&array[upindex],&array[downindex],
			    &cond,fiber_dependency,aug_graph);
	  free_edgeset(aug_graph->graph[downindex*n+upindex],aug_graph);
	  free_edgeset(aug_graph->graph[downindex*n+downindex],aug_graph);
	  aug_graph->graph[upindex*n+upindex] =
	    aug_graph->graph[downindex*n+upindex] =
	      aug_graph->graph[downindex*n+downindex] =
		NULL;
	}
      } else if (cycle_type != 0) {
	found = 0;
	if (cycle_type == CYC_BELOW)
	  fatal_error("Below only: no locals to attach to!");
	if (!(cycle_type & CYC_ABOVE)) {
	  updecl =
	    value_decl(def(make_up_down_name(aug_graph_name(aug_graph),
					     i,TRUE),
			   FALSE,FALSE),
		       no_type(), /* sloppy */
		       direction(FALSE,FALSE,FALSE),no_default());
	  downdecl =
	    value_decl(def(make_up_down_name(aug_graph_name(aug_graph),
					     i,FALSE),
			   FALSE,FALSE),
		       no_type(), /* sloppy */
		       direction(FALSE,FALSE,FALSE),no_default());
	  Declaration_info(updecl)->decl_flags = DECL_LOCAL_FLAG;
	  Declaration_info(downdecl)->decl_flags = DECL_LOCAL_FLAG;
	}
	for (k=0; k < n; ++k) {
	  if (array[k].node != lastnode) {
	    start = k;
	    lastnode = array[k].node;
	  }
	  if (parent_index[k+constructor_index] == cyc->internal_info) {
	    /* printf("in cycle: ");
	     * print_instance(&array[k],stdout);
	     * printf("\n");
	     */
	    if (array[k].node == NULL) { /* a local */
	      switch (++found) {
	      case 1:      
		array[k].fibered_attr.attr = updecl;
		Declaration_info(updecl)->instance_index = upindex = k;
		break;
	      case 2:
		array[k].fibered_attr.attr = downdecl;
		Declaration_info(downdecl)->instance_index = downindex = k;
		break;
	      default:
		array[k].fibered_attr.attr = NULL;
		break;
	      }
	    } else {
	      PHY_GRAPH *phy_graph =
		Declaration_info(array[k].node)->node_phy_graph;
	      array[k].fibered_attr.attr =
		phy_graph->instances.array[k-start].fibered_attr.attr;
	      if (found < 2 && ! DECL_IS_RHS(array[k].node))
		switch (++found) {
		case 1: upindex = k; break;
		case 2: downindex = k; break;
		}
	    }
	    array[k].fibered_attr.fiber = NULL;
	  }
	}
	if (found > 0) {
	  if (found == 1) downindex = upindex;
	  /*
	  printf("up = ");
	  print_instance(&array[upindex],stdout);
	  printf(" down = ");
	  print_instance(&array[downindex],stdout);
	  printf("\n");
	  */
	  /* redo dependencies */
	  for (k = 0; k < n; ++k)
	    if (parent_index[k+constructor_index] == cyc->internal_info)
	      for (l = 0; l < n; ++l)
		if (parent_index[l+constructor_index] == cyc->internal_info) {
		  free_edgeset(aug_graph->graph[k*n+l],aug_graph);
		  aug_graph->graph[k*n+l] = NULL;
		}
	  for (k=0; k < n; ++k) {
	    BOOL kcycle =
	      parent_index[k+constructor_index] == cyc->internal_info;
	    if (k != downindex) {
	      for (l=0; l < n; ++l) {
		BOOL lcycle =
		  parent_index[l+constructor_index] == cyc->internal_info;
		if (l != upindex &&
		    edgeset_kind(aug_graph->graph[k*n+l]) != no_dependency) {
		  if (kcycle || lcycle) {
		    free_edgeset(aug_graph->graph[k*n+l],aug_graph);
		    aug_graph->graph[k*n+l] = NULL;
		    if (!lcycle) {
		      add_edge_to_graph(&array[downindex],&array[l],
					&cond,fiber_dependency,aug_graph);
		    } else if (!kcycle) {
		      add_edge_to_graph(&array[k],&array[upindex],
					&cond,fiber_dependency,aug_graph);
		    }
		  }
		}
	      }
	    }
	  }
	  /* fix up dependencies for cycle attributes */
	  for (k=0; k < n; ++k) {
	    if (parent_index[k+constructor_index] == cyc->internal_info &&
		k != downindex && k != downindex &&
		array[k].node != NULL &&
		DECL_IS_RHS(array[k].node)) {
	      Declaration attr = array[k].fibered_attr.attr;
	      if (attr != NULL) {
		if (ATTR_DECL_IS_SYN(attr)) {
		  add_edge_to_graph(&array[k],&array[upindex],&cond,
				    fiber_dependency,aug_graph);
		} else {
		  add_edge_to_graph(&array[downindex],&array[k],&cond,
				    fiber_dependency,aug_graph);
		}
	      }
	    }
	  }
	  if (downindex != upindex) {
	    free_edgeset(aug_graph->graph[upindex*n+upindex],aug_graph);
	    add_edge_to_graph(&array[upindex],&array[downindex],
			      &cond,fiber_dependency,aug_graph);
	    free_edgeset(aug_graph->graph[downindex*n+upindex],aug_graph);
	    free_edgeset(aug_graph->graph[downindex*n+downindex],aug_graph);
	    aug_graph->graph[upindex*n+upindex] =
	      aug_graph->graph[downindex*n+upindex] =
		aug_graph->graph[downindex*n+downindex] =
		  NULL;
	  }
	}
      }
    }
  }
}


void break_fiber_cycles(Declaration module,STATE *s) {
  void *mark = SALLOC(0);
  init_indices(s);
  make_cycles(s);
  get_fiber_cycles(s);
  add_up_down_attributes(s);
  release(mark);
  {
    int saved_analysis_debug = analysis_debug;
    
    if (cycle_debug & DEBUG_UP_DOWN) {
      analysis_debug |= TWO_EDGE_CYCLE;
    }

    if (analysis_debug & DNC_ITERATE) {
      printf("\n**** After introduction of up/down attributes:\n\n");
    }

    dnc_close(s);

    analysis_debug = saved_analysis_debug;
  }

  if (analysis_debug & (DNC_ITERATE|DNC_FINAL)) {
    printf("\n****** After closure of up/down attributes:\n\n");
    print_analysis_state(s,stdout);
  }
}


/**** PRINTING ****/

void print_fiber_cycles(STATE *s) {
}
  


