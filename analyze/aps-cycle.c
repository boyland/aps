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


#define UP_DOWN_DIRECTION(v, direction) (direction ? v : !v)
#define IS_JUST_FIBER_DEPENDENCY(d) (((d) & DEPENDENCY_MAYBE_SIMPLE))

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

static void merge_sets(int index1, int index2) {
  if (parent_index[index1] == -1) /* non normally cyclic */
    parent_index[index1] = get_set(index2);
  else
    parent_index[get_set(index1)] = get_set(index2);
}

typedef VECTOR(int) SETS;

static void get_fiber_cycles(STATE *s) {
  int i,j,k,l;
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


  // Hold on to phylum cycles
  for (i = 0; i < s->phyla.length; i++) {
    int phylum_index = phylum_instance_start[i];
    PHY_GRAPH* phy = &s->phy_graphs[i];

    int num_cycles = 0;
    CYCLE** cycles = (CYCLE**)alloca(sizeof(CYCLE*) * s->cycles.length);
    for (j = 0; j < s->cycles.length; j++) {
      CYCLE* cyc = &s->cycles.array[j];
      int count = 0;
      for (k = 0; k < cyc->instances.length; ++k) {
        INSTANCE* in1 = &cyc->instances.array[k];
        for (l = 0; l < phy->instances.length; l++) {
          INSTANCE* in2 = &phy->instances.array[l];
          if (INSTANCE_EQUAL(in1, in2)) {
            count++;
          }
        }
      }

      // If there is any cycle involving instances of this phylum graph
      if (count > 0) {
        CYCLE* phylum_cycle = (CYCLE*)HALLOC(sizeof(CYCLE));
        cycles[j] = phylum_cycle;
        phylum_cycle->internal_info = j;
        VECTORALLOC(phylum_cycle->instances, INSTANCE, count);

        CYCLE* generic_cycle = &s->cycles.array[j];
        num_cycles++;
        count = 0;

        if (cycle_debug & PRINT_UP_DOWN) {
          printf("Cycle (%d) involving phylum graph: %s\n", j,
                 decl_name(phy->phylum));
        }

        int count = 0;
        for (k = 0; k < generic_cycle->instances.length; ++k) {
          INSTANCE* in1 = &generic_cycle->instances.array[k];
          for (l = 0; l < phy->instances.length; l++) {
            INSTANCE* in2 = &phy->instances.array[l];
            if (INSTANCE_EQUAL(in1, in2)) {
              phylum_cycle->instances.array[count++] = *in1;

              if (cycle_debug & PRINT_UP_DOWN) {
                printf("  ");
                print_instance(in1, stdout);
                printf("\n");
              }
            }
          }
        }

        // Determine the combined dependencies between attributes in the cycle
        DEPENDENCY dep = no_dependency;
        for (k = 0; k < phylum_cycle->instances.length; k++) {
          INSTANCE* source = &phylum_cycle->instances.array[k];
          for (l = 0; l < phylum_cycle->instances.length; l++) {
            INSTANCE* target = &phylum_cycle->instances.array[l];
            dep |= phy->mingraph[source->index * phy->instances.length +
                                 target->index];
          }
        }

        phylum_cycle->acc_dep = dep;
      }
    }

    if (num_cycles > 0) {
      if (cycle_debug & PRINT_UP_DOWN) {
        printf(" => Phylum graph %s has %d cycles.\n\n", decl_name(phy->phylum),
               num_cycles);
      }

      VECTORALLOC(phy->cycles, CYCLE, num_cycles);
      for (j = 0; j < num_cycles; j++) {
        phy->cycles.array[j] = *cycles[j];
      }
    }
  }

  // Hold on to augmented dependency graph cycles
  for (i = 0; i <= s->match_rules.length; ++i) {
    int constructor_index = constructor_instance_start[i];
    AUG_GRAPH* aug_graph = (i == s->match_rules.length)
                               ? &s->global_dependencies
                               : &s->aug_graphs[i];

    int num_cycles = 0;
    CYCLE** cycles = (CYCLE**)alloca(sizeof(CYCLE*) * s->cycles.length);
    for (j = 0; j < s->cycles.length; j++) {
      CYCLE* cyc = &s->cycles.array[j];
      int count = 0;
      for (k = 0; k < cyc->instances.length; ++k) {
        INSTANCE* in1 = &cyc->instances.array[k];
        for (l = 0; l < aug_graph->instances.length; l++) {
          INSTANCE* in2 = &aug_graph->instances.array[l];
          if (INSTANCE_EQUAL(in1, in2)) {
            count++;
          }
        }
      }

      // If there is any cycle involving instances of this augmented dependency
      // graph
      if (count > 0) {
        CYCLE* aug_graph_cycle = (CYCLE*)HALLOC(sizeof(CYCLE));
        cycles[num_cycles] = aug_graph_cycle;
        aug_graph_cycle->internal_info = j;
        VECTORALLOC(aug_graph_cycle->instances, INSTANCE, count);

        CYCLE* generic_cycle = &s->cycles.array[j];
        num_cycles++;
        count = 0;

        if (cycle_debug & PRINT_UP_DOWN) {
          printf("Cycle (%d) involving augmented dependency graph: %s\n", j,
                 decl_name(aug_graph->syntax_decl));
        }

        int count = 0;
        for (k = 0; k < generic_cycle->instances.length; ++k) {
          INSTANCE* in1 = &generic_cycle->instances.array[k];
          for (l = 0; l < aug_graph->instances.length; l++) {
            INSTANCE* in2 = &aug_graph->instances.array[l];
            if (INSTANCE_EQUAL(in1, in2)) {
              aug_graph_cycle->instances.array[count++] = *in1;

              if (cycle_debug & PRINT_UP_DOWN) {
                printf("  ");
                print_instance(in1, stdout);
                printf("\n");
              }
            }
          }
        }

        // Determine the combined dependencies between attributes in the cycle
        DEPENDENCY dep = no_dependency;
        for (k = 0; k < aug_graph->instances.length; k++) {
          INSTANCE* source = &aug_graph_cycle->instances.array[k];
          for (l = 0; l < aug_graph_cycle->instances.length; l++) {
            INSTANCE* target = &aug_graph_cycle->instances.array[l];
            dep |= get_edgeset_combine_dependencies(
                aug_graph->graph[source->index * aug_graph->instances.length +
                                 target->index]);
          }
        }

        aug_graph_cycle->acc_dep = dep;
      }
    }

    if (num_cycles > 0) {
      if (cycle_debug & PRINT_UP_DOWN) {
        printf(" => Augmented dependency graph %s has %d cycles.\n\n",
               decl_name(aug_graph->syntax_decl), num_cycles);
      }

      VECTORALLOC(aug_graph->cycles, CYCLE, num_cycles);
      for (j = 0; j < num_cycles; j++) {
        aug_graph->cycles.array[j] = *cycles[j];
      }
    }
  }
}

/*** determining strongly connected sets of attributes ***/

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

  // DO NOT merge cycles involving phylum and augmented graph together.
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


/*** Break cycles and redo dependency graphs ***/

bool instance_is_up(INSTANCE *i) {
  return (fibered_attr_direction(&i->fibered_attr)) == instance_outward;
}

/**
 * Removes edgeset between two instances at the indices.
 * @param index1 source index
 * @param index2 sink index
 * @param n width of matrix
 * @param array instance array
 * @param aug_graph augmented dependency graph
 */
static void remove_edgeset(int index1, int index2, int n, INSTANCE *array, AUG_GRAPH *aug_graph)
{
  INSTANCE *attr1 = (&array[index1]);
  INSTANCE *attr2 = (&array[index2]);
  if (cycle_debug & DEBUG_UP_DOWN) {
    printf("  Removing up/down: ");
    print_instance(attr1, stdout);
    printf(" -> ");
    print_instance(attr2, stdout);
    printf("\n");
  }
  free_edgeset(aug_graph->graph[index1 * n + index2], aug_graph);
  aug_graph->graph[index1 * n + index2] = NULL;
}

/**
 * Add edge between two instances to reflect the up/down construction.
 * @param index1 source index
 * @param index2 sink index
 * @param n width of matrix
 * @param array instance array
 * @param dep dependency to join
 * @param cond condition to join
 * @param aug_graph augmented dependency graph
 */
static void add_up_down_edge(int index1, int index2, int n, INSTANCE *array, DEPENDENCY dep, CONDITION *cond, AUG_GRAPH *aug_graph)
{
  INSTANCE *attr1 = &array[index1];
  INSTANCE *attr2 = &array[index2];

  if (cycle_debug & DEBUG_UP_DOWN)
  {     
    printf("  Adding up/down: ");
    print_instance(attr1, stdout);
    printf(" -> ");
    print_instance(attr2, stdout);
    printf("\n");
  }
  add_edge_to_graph(attr1, attr2, cond, dep, aug_graph);
}

/**
 * Combines dependencies for edgeset
 * @param es edgeset
 * @return combined dependencies given an edgeset
 */
DEPENDENCY get_edgeset_combine_dependencies(EDGESET es)
{
  DEPENDENCY acc_dependency = no_dependency;
  for (; es != NULL; es = es->rest)
  {
    acc_dependency |= es->kind;
  }

  return acc_dependency;
}

/**
 * Combines dependencies for edgeset
 * @param es
 * @param acc_dependency
 * @param acc_cond
 */
static void edgeset_combine_dependencies(EDGESET es, DEPENDENCY* acc_dependency, CONDITION* acc_cond)
{
  for (; es != NULL; es = es->rest)
  {
    *acc_dependency |= es->kind;
    acc_cond->positive |= es->cond.positive;
    acc_cond->negative |= es->cond.negative;
  }
}

static bool edge_can_be_deleted(int index1, int index2, INSTANCE *array, DEPENDENCY dep)
{
  INSTANCE *attr1 = (&array[index1]);
  INSTANCE *attr2 = (&array[index2]);

  if (index1 != index2 &&
      (dep & DEPENDENCY_MAYBE_DIRECT) &&
      (dep & DEPENDENCY_NOT_JUST_FIBER) &&
      (fibered_attr_direction(&attr1->fibered_attr)) == instance_local)
  {
    if (cycle_debug & DEBUG_UP_DOWN)
    {
      printf(" preserved critical: ");
      print_instance(attr1, stdout);
      printf(" -> ");
      print_instance(attr2, stdout);
      printf("\n");
    }
    return false;
  }

  // Kill the edge
  return true;
}

/**
 * In phylum graph (and in aug graph)
 * For every instance i:
 *  If the instance is NOT in the cycle:
 *    "OR" the condition/kind for the dependency from i to any instance of the cycle.
 *    If result is not False, 0
 *      Create a dependency from i to *every* instance in the cycle with this condition and kind
 *    
 *    "OR" the condition/kind for any instance of the cycle to i
 *    If result is not False, 0
 *      Create a dependency from *every* instance in the cycle to i with this condition and kind
 * 
 *  If the instance is in the cycle and an UP attr:
 *    "OR" the condition/kind for the dependency from i to any instance of the cycle
 *    If the result is not False, 0
 *      Create a dependency from i to all the DOWN instances in the cycle
 *    Remove all dependencies from this attribute to any other UP instance in the same cycle
 *
 *  If the instance is in the cycle and an DOWN attr:
 *    Remove all dependencies from this attribute to any other instance in the same cycle
 * @param s analysis STATE
 * @param direction true: UP_DOWN and false DOWN_UP
 */
static void add_up_down_attributes(STATE *s, bool direction)
{
  int i, j, k, l, m;
  DEPENDENCY acc_dependency;
  CONDITION acc_cond;

  // Forall cycles in the graph
  for (i = 0; i < s->cycles.length; i++)
  {
    CYCLE *cyc = &s->cycles.array[i];
    if (cycle_debug & DEBUG_UP_DOWN) printf("Breaking Cycle #%d\n",i);

    if (!(cyc->acc_dep & DEPENDENCY_MAYBE_SIMPLE))
    {
      if (cycle_debug & DEBUG_UP_DOWN)
      {
        printf("Skipped breaking Cycle #%d\n",i);
      }
      continue;
    }

    // Forall phylum in the phylum_graph
    for (j = 0; j < s->phyla.length; j++)
    {
      PHY_GRAPH *phy = &s->phy_graphs[j];
      int n = phy->instances.length;
      int phylum_index = phylum_instance_start[j];
      INSTANCE *array = phy->instances.array;

      for (k = 0; k < n; k++)
      {
        INSTANCE *instance = &array[k];

        // If instance is not in the cycle
        if (parent_index[k + phylum_index] != cyc->internal_info)
        {
          acc_dependency = no_dependency;

          // Forall instances in the cycle
          for (l = 0; l < n; l++)
          {
            // If dependency is not to self and is in cycle
            if (parent_index[l + phylum_index] == cyc->internal_info)
            {
              acc_dependency |= phy->mingraph[k * n + l];
            }
          }

          // If any dependency
          if (acc_dependency)
          {
            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // If edge is not to self and it is in the cycle
              if (parent_index[l + phylum_index] == cyc->internal_info)
              {
                phy->mingraph[k * n + l] = acc_dependency;
              }
            }
          }

          acc_dependency = no_dependency;

          // Forall instances in the cycle
          for (l = 0; l < n; l++)
          {
            // If dependency is not to self and is in cycle
            if (parent_index[l + phylum_index] == cyc->internal_info)
            {
              acc_dependency |= phy->mingraph[l * n + k];
            }
          }

          // If any dependency
          if (acc_dependency)
          {
            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // If edge is not to self and it is in the cycle
              if (parent_index[l + phylum_index] == cyc->internal_info)
              {
                phy->mingraph[l * n + k] = acc_dependency;
              }
            }
          }
        }
        // Instance is in the cycle
        else
        {
          // UP attribute
          if (direction && UP_DOWN_DIRECTION(instance_is_up(instance), direction))
          {
            acc_dependency = no_dependency;

            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // If dependency is not to self and is in cycle
              if (parent_index[l + phylum_index] == cyc->internal_info)
              {
                acc_dependency |= phy->mingraph[k * n + l];
              }
            }

            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // Make sure it is in the cycle
              if (parent_index[l + phylum_index] == cyc->internal_info)
              {
                // Make sure it is a DOWN attribute
                if (acc_dependency && UP_DOWN_DIRECTION(!instance_is_up(&array[l]), direction))
                {
                  phy->mingraph[k * n + l] = acc_dependency;
                }
                else if (edge_can_be_deleted(k, l, array, phy->mingraph[k * n + l]))
                {
                  phy->mingraph[k * n + l] = no_dependency;
                }
              }
            }
          }
          // DOWN attribute
          else
          {
            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // Make sure it is in the cycle
              if (parent_index[l + phylum_index] == cyc->internal_info)
              {
                if (edge_can_be_deleted(k, l, array, phy->mingraph[k * n + l]))
                {
                  // Remove edges between instance and all others in the same cycle
                  phy->mingraph[k * n + l] = no_dependency;
                }
              }
            }
          }
        }
      }
    }

    // Forall edges in the augmented dependency graph
    for (j = 0; j <= s->match_rules.length; j++)
    {
      AUG_GRAPH *aug_graph =
          (j == s->match_rules.length) ? &s->global_dependencies : &s->aug_graphs[j];
      int n = aug_graph->instances.length;
      int constructor_index = constructor_instance_start[j];
      INSTANCE *array = aug_graph->instances.array;

      if (cycle_debug & DEBUG_UP_DOWN)
      {
        printf("\naug decl is: %s\n", decl_name(aug_graph->syntax_decl));
      }
      
      for (k = 0; k < n; k++)
      {
        INSTANCE *instance = &array[k];

	if (cycle_debug & DEBUG_UP_DOWN) {
	  printf("> aug node #%d (",k);
	  print_instance(&array[k],stdout);
	  printf(") %s\n",
		 parent_index[k + constructor_index] == cyc->internal_info ?
		 (instance_is_up(instance) ? "UP" : "DOWN") :
		 "not in cycle");
	}

        // If instance is not in the cycle
        if (parent_index[k + constructor_index] != cyc->internal_info)
        {
          acc_dependency = no_dependency;
          acc_cond.positive = 0;
          acc_cond.negative = 0;

          // Forall instances in the cycle
          for (l = 0; l < n; l++)
          {
            // If dependency is not to self and is in cycle
            if (parent_index[l + constructor_index] == cyc->internal_info && aug_graph->graph[k * n + l] != NULL)
            {
              edgeset_combine_dependencies(aug_graph->graph[k * n + l], &acc_dependency, &acc_cond);
            }
          }

          // If any dependency
          if (acc_dependency)
          {
            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // If edge is not to self and it is in the cycle
              if (parent_index[l + constructor_index] == cyc->internal_info)
              {
                // printf("k -> l Adding Not In cycle -> In Cycle: ");
                add_up_down_edge(k, l, n, array, acc_dependency, &acc_cond, aug_graph);
              }
            }
          }

          acc_dependency = no_dependency;
          acc_cond.positive = 0;
          acc_cond.negative = 0;

          // Forall instances in the cycle
          for (l = 0; l < n; l++)
          {
            // If dependency is not to self and is in cycle
            if (parent_index[l + constructor_index] == cyc->internal_info && aug_graph->graph[l * n + k] != NULL)
            {
              edgeset_combine_dependencies(aug_graph->graph[l * n + k], &acc_dependency, &acc_cond);
            }
          }

          // If any dependency
          if (acc_dependency)
          {
            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // If edge is not to self and it is in the cycle
              if (parent_index[l + constructor_index] == cyc->internal_info)
              {
                // printf("l -> k Adding In Cycle -> Not In Cycle: ");
                add_up_down_edge(l, k, n, array, acc_dependency, &acc_cond, aug_graph);
              }
            }
          }
        }
        // Instance is in the cycle
        else
        {
          // UP attribute
          if (UP_DOWN_DIRECTION(instance_is_up(instance), direction))
          {
            acc_dependency = no_dependency;
            acc_cond.positive = 0;
            acc_cond.negative = 0;

            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // If dependency is not to self and is in cycle
              if (parent_index[l + constructor_index] == cyc->internal_info && aug_graph->graph[k * n + l] != NULL)
              {
                edgeset_combine_dependencies(aug_graph->graph[k * n + l], &acc_dependency, &acc_cond);
              }
            }

            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              DEPENDENCY dep = get_edgeset_combine_dependencies(aug_graph->graph[k * n + l]);
              // Make sure it is in the cycle
              if (parent_index[l + constructor_index] == cyc->internal_info)
              {
                // Make sure it is a DOWN attribute
                if (acc_dependency && UP_DOWN_DIRECTION(!instance_is_up(&array[l]), direction))
                {
                  // printf("k -> l Adding In cycle -> In Cycle: ");
                  add_up_down_edge(k, l, n, array, acc_dependency, &acc_cond, aug_graph);
                }
                else if (edge_can_be_deleted(k, l, array, dep))
                {
                  // printf("k -> l Removing In cycle -> In Cycle: ");
                  remove_edgeset(k, l, n, array, aug_graph);
                }
              }
            }
          }
          // DOWN attribute
          else
          {
            // Forall instances in the cycle
            for (l = 0; l < n; l++)
            {
              // Make sure it is in the cycle
              if (parent_index[l + constructor_index] == cyc->internal_info)
              {
                DEPENDENCY dep = get_edgeset_combine_dependencies(aug_graph->graph[k * n + l]);
                if (edge_can_be_deleted(k, l, array, dep))
                {
                  // Remove edges between instance and all others in the same cycle
                  // printf("k -> l Removing Down In cycle -> In Cycle: ");
                  remove_edgeset(k, l, n, array, aug_graph);
                }
              }
            }
          }
        }
      }
    }
  }
}

static bool is_inside_some_function(Declaration decl, Declaration *func)
{
  void *current = decl;
  Declaration current_decl;
  while (current != NULL && (current = tnode_parent(current)) != NULL)
  {
    switch (ABSTRACT_APS_tnode_phylum(current))
    {
    case KEYDeclaration:
      current_decl = (Declaration)current;
      switch (Declaration_KEY(current_decl))
      {
      case KEYsome_function_decl:
        *func = current_decl;
        return some_function_decl_result(current_decl) == decl;
      }
    }
  }

  return false;
}

/**
 * This function determines whether attribute instance should be considered for circularity check
 * @param instance attribute instance
 * @return true if instance not is not: If, Match and some formal
 * @return false everything else
 */
static bool instance_can_be_considered_for_circularity_check(INSTANCE *instance)
{
  // If and Match statements can show up in a cycle but are never declared circular or non-circular
  if (if_rule_p(instance->fibered_attr.attr))
    return false;

  Declaration node = instance->fibered_attr.attr;
  Declaration func = NULL;

  // The result in a function/procedure may show up in a cycle but are never declared circular or non-circular
  if (is_inside_some_function(node, &func) && some_function_decl_result(func) == node)
    return false;

  if (instance->fibered_attr.fiber != NULL)
    return false;

  // Formals may show up in a cycle as well
  switch (ABSTRACT_APS_tnode_phylum(node))
  {
  case KEYDeclaration:
  {
    switch (Declaration_KEY(node))
    {
    case KEYformal:
      return false;
    }
  }
  }

  return true;
}

/**
 * Ensure that attributes that participate in a cycle are defined circular
 * @param s Analysis state
 */
static void assert_circular_declaration(STATE* s) {
  int i, j, k;

  // Forall phylum in the phylum_graph
  for (i = 0; i < s->phyla.length; i++) {
    PHY_GRAPH* phy = &s->phy_graphs[i];
    int n = phy->instances.length;
    int phylum_index = phylum_instance_start[i];
    INSTANCE* array = phy->instances.array;

    for (j = 0; j < n; j++) {
      INSTANCE* instance = &array[j];
      Declaration node = instance->fibered_attr.attr;

      if (!instance_can_be_considered_for_circularity_check(instance))
        continue;

      bool any_cycle = false;
      bool declared_circular = instance_circular(instance);

      char instance_to_str[BUFFER_SIZE];
      FILE* f = fmemopen(instance_to_str, sizeof(instance_to_str), "w");
      print_instance(instance, f);
      fclose(f);

      for (k = 0; k < s->cycles.length; k++) {
        CYCLE* cyc = &s->cycles.array[k];
        if (parent_index[j + phylum_index] == cyc->internal_info) {
          if (declared_circular) {
            any_cycle = true;
          } else {
            aps_error(node,
                      "Instance (%s) involves in a cycle but it is not "
                      "declared circular.",
                      instance_to_str);
          }
        }
      }

      if (declared_circular && !any_cycle) {
        aps_warning(node,
                    "Instance (%s) is declared circular but does not involve "
                    "in any cycle.",
                    instance_to_str);
      }
    }
  }

  // Forall edges in the augmented dependency graph
  for (i = 0; i <= s->match_rules.length; i++) {
    AUG_GRAPH* aug_graph = (i == s->match_rules.length)
                               ? &s->global_dependencies
                               : &s->aug_graphs[i];
    int n = aug_graph->instances.length;
    int constructor_index = constructor_instance_start[i];
    INSTANCE* array = aug_graph->instances.array;
    for (j = 0; j < n; j++) {
      INSTANCE* instance = &array[j];
      Declaration node = instance->fibered_attr.attr;

      if (!instance_can_be_considered_for_circularity_check(instance))
        continue;

      bool any_cycle = false;
      bool declared_circular = instance_circular(instance);

      char instance_to_str[BUFFER_SIZE];
      FILE* f = fmemopen(instance_to_str, sizeof(instance_to_str), "w");
      print_instance(instance, f);
      fclose(f);

      // Forall cycles in the graph
      for (k = 0; k < s->cycles.length; k++) {
        CYCLE* cyc = &s->cycles.array[k];
        if (parent_index[j + constructor_index] == cyc->internal_info) {
          if (declared_circular) {
            any_cycle = true;
          } else {
            aps_error(node,
                      "Instance (%s) involves in a cycle but it is not "
                      "declared circular.",
                      instance_to_str);
          }
        }
      }

      if (declared_circular && !any_cycle) {
        aps_warning(node,
                    "Instance (%s) is declared circular but does not involve "
                    "in any cycle.",
                    instance_to_str);
      }
    }
  }
}

void break_fiber_cycles(Declaration module,STATE *s,DEPENDENCY dep) {
  void *mark = SALLOC(0);
  init_indices(s);
  make_cycles(s);
  get_fiber_cycles(s);
  assert_circular_declaration(s);

  bool direction = !(dep & DEPENDENCY_NOT_JUST_FIBER);
  add_up_down_attributes(s,direction);
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
