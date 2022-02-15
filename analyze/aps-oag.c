#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jbb.h"
#include "jbb-alloc.h"
#include "aps-ag.h"

int oag_debug;

/**
 * Utility function that schedules a single phase
 * @param phy_graph phylum graph
 * @param ph phase its currently scheduling for
 * @param circular boolean indicating whether this phase is dedicated for circular
 * @return number of nodes scheduled successfully for this phase 
 */
static int schedule_phase(PHY_GRAPH * phy_graph, int phase, BOOL circular) {
  int done = 0;
  int n = phy_graph->instances.length;
  int i, j;

  /* find inherited instances for the phase. */
  for (i = 0; i < n; ++i) {
    INSTANCE * in = & phy_graph->instances.array[i];
    if (instance_direction(in) == instance_inward &&
      instance_circular(in) == circular &&
      phy_graph->summary_schedule[i] == 0) {
      for (j = 0; j < n; ++j) {
        if (phy_graph->summary_schedule[j] == 0 &&
          phy_graph->mingraph[j * n + i] != no_dependency)
          break;
      }
      if (j == n) {
        phy_graph->summary_schedule[i] = -phase;
        done++;
        for (j = 0; j < n; ++j) {
          /* force extra dependencies */
          int sch = phy_graph->summary_schedule[j];
          if (sch != 0 && sch != -phase)
            phy_graph->mingraph[j * n + i] = indirect_control_dependency;
        }
        if (oag_debug & TOTAL_ORDER) {
          printf("%d- ", phase);
          print_instance( in , stdout);
          printf("\n");
        }
      }
    }
  }

  /* now schedule synthesized attributes */
  for (i = 0; i < n; ++i) {
    INSTANCE * in = & phy_graph->instances.array[i];
    if (instance_direction(in) == instance_outward &&
      instance_circular(in) == circular &&
      phy_graph->summary_schedule[i] == 0) {
      for (j = 0; j < n; ++j) {
        if (phy_graph->summary_schedule[j] == 0 &&
          phy_graph->mingraph[j * n + i] != no_dependency)
          break;
      }
      if (j == n) {
        phy_graph->summary_schedule[i] = phase;
        done++;
        for (j = 0; j < n; ++j) {
          /* force extra dependencies */
          int sch = phy_graph->summary_schedule[j];
          if (sch != 0 && sch != phase)
            phy_graph->mingraph[j * n + i] = indirect_control_dependency;
        }
        if (oag_debug & TOTAL_ORDER) {
          printf("%d+ ", phase);
          print_instance( in , stdout);
          printf("\n");
        }
      }
    }
  }

  return done;
}

/**
 * Utility function that calculates ph (phase) for each attribute of a phylum
 */
void schedule_summary_dependency_graph(PHY_GRAPH* phy_graph) {
  int n = phy_graph->instances.length;
  int phase = 0;
  int done = 0;
  BOOL cont = true;
  
  int i, j;
  for (i = 0; i < n; ++i)
    phy_graph->summary_schedule[i] = 0;

  size_t circular_phase_size = (n+1) * sizeof(BOOL);
  BOOL* circular_phase = (BOOL*)HALLOC(circular_phase_size);
  memset(circular_phase, false, circular_phase_size);

  // Hold on to the flag indicating whether phase is circular or not
  phy_graph->cyclic_flags = circular_phase;

  int count_non_circular = 0, count_circular = 0;
  do {
    phase++;

    // Schedule non-circular attributes in this phase
    count_non_circular = schedule_phase(phy_graph, phase, false);
    if (count_non_circular) {
      done += count_non_circular;
      circular_phase[phase] = false;
      continue;
    }

    // Schedule circular attributes in this phase
    count_circular = schedule_phase(phy_graph, phase, true);
    if (count_circular) {
      done += count_circular;
      circular_phase[phase] = true;
      continue;
    }

  } while (count_non_circular || count_circular);

  if (done < n) {
    if (cycle_debug & PRINT_CYCLE) {
      for (i = 0; i < n; ++i) {
        INSTANCE* in = & phy_graph->instances.array[i];
        int s = phy_graph->summary_schedule[i];
        print_instance( in , stdout);
        switch (instance_direction( in )) {
          case instance_local:
            printf(" (a local attribute!) ");
            break;
          case instance_inward:
            printf(" inherited ");
            break;
          case instance_outward:
            printf(" synthesized ");
            break;
          default:
            printf(" (garbage direction!) ");
            break;
        }
        if (s != 0) {
          if (s < 0) printf(": phase -%d\n", -s);
          else printf(":phase +%d\n", s);
        } else {
          printf(" depends on ");
          for (j = 0; j < n; ++j) {
            if (phy_graph->summary_schedule[j] == 0 &&
              phy_graph->mingraph[j * n + i] != no_dependency) {
              INSTANCE* in2 = & phy_graph->instances.array[j];
              print_instance(in2, stdout);
              if (phy_graph->mingraph[j * n + i] == fiber_dependency)
                printf("(?)");
              putc(' ', stdout);
            }
          }
          putc('\n', stdout);
        }
      }
    }

    fatal_error("Cycle detected when scheduling phase %d for %s",
		  phase,decl_name(phy_graph->phylum));
  }
}

CONDITION instance_condition(INSTANCE *in)
{
  Declaration ad = in->fibered_attr.attr;
  if (in->node != 0) {
    return Declaration_info(in->node)->decl_cond;
  }
  if (ad == 0) {
    /* attribute removed in cyclic fibering. */
    CONDITION c;
    c.positive = -1;
    c.negative = -1;
    return c;
  }
  switch (ABSTRACT_APS_tnode_phylum(ad)) {
  case KEYMatch:
    return Match_info((Match)ad)->match_cond;
  default:
    return Declaration_info(ad)->decl_cond;
  }
}

CTO_NODE* schedule_rest(AUG_GRAPH *aug_graph,
			CTO_NODE* prev,
			CONDITION cond,
			int remaining)
{
  CTO_NODE* cto_node = 0;
  int i;
  int n = aug_graph->instances.length;
  int needed_condition_bits;
  int sane_remaining = 0;
  INSTANCE* in;

  /* If nothing more to do, we are done. */
  if (remaining == 0) return 0;

  for (i=0; i < n; ++i) {
    INSTANCE *in1 = &aug_graph->instances.array[i];
    int j;

    /* If already scheduled, then ignore. */
    if (aug_graph->schedule[i] != 0) continue;

    ++sane_remaining;

    /* Look for a predecessor edge */
    for (j=0; j < n; ++j) {
      INSTANCE *in2 = &aug_graph->instances.array[j];
      int index = j*n+i;
      EDGESET edges = 0;

      if (aug_graph->schedule[j] != 0) {
	/* j is scheduled already, so we can ignore dependencies */
	continue;
      }

      /* Look at all dependencies from j to i */
      for (edges = aug_graph->graph[index]; edges != 0; edges=edges->rest) {
	CONDITION merged;
	merged.positive = cond.positive | edges->cond.positive;
	merged.negative = cond.negative | edges->cond.negative;

	/* if the merge condition is impossible, ignore this edge */
	if (merged.positive & merged.negative) continue;

	if (oag_debug & PROD_ORDER_DEBUG) {
	  int i=n-remaining;
	  for (; i > 0; --i) printf("  ");
	  if (aug_graph->schedule[j] == 0)
	    printf("! ");
	  else
	    printf("? ");
	  print_edge(edges,stdout);
	}

	/* If j not scheduled, then i cannot be considered */
	break; /* leave edges != 0 */
      }

      /* If a remaining edge, then i cannot be considered */
      if (edges != 0) break;
    }

    /* If we got through all predecessors, we can stop */
    if (j == n) break;
  }

  if (i == n) {
    fflush(stdout);
    if (sane_remaining != remaining) {
      fprintf(stderr,"remaining out of sync %d != %d\n",
	      sane_remaining, remaining);
    }
    fprintf(stderr,"Cannot make conditional total order!\n");
    for (i=0; i < n; ++i) {
      INSTANCE *in1 = &aug_graph->instances.array[i];
      int j;

      if (aug_graph->schedule[i] != 0) continue;

      fprintf(stderr,"  ");
      print_instance(in1,stderr);
      fprintf(stderr," requires:\n");

      for (j=0; j < n; ++j) {
	INSTANCE *in2 = &aug_graph->instances.array[j];
	int index = j*n+i;
	EDGESET edges = 0;
	
	if (aug_graph->schedule[j] != 0) {
	  /* j is scheduled already, so we can ignore dependencies */
	  continue;
	}

	/* Look at all dependencies from j to i */
	for (edges = aug_graph->graph[index]; edges != 0; edges=edges->rest) {
	  CONDITION merged;
	  merged.positive = cond.positive | edges->cond.positive;
	  merged.negative = cond.negative | edges->cond.negative;
	  
	  /* if the merge condition is impossible, ignore this edge */
	  if (merged.positive & merged.negative) continue;
	  break; /* leave edges != 0 */
	}

	if (edges != 0) {
	  fputs("    ",stderr);
	  print_instance(in2,stderr);
	  fputs("\n",stderr);
	}
      }
    }
    fatal_error("Cannot make conditional total order!");
  }

  in = &aug_graph->instances.array[i];

  /* check to see if makes sense
   * (No need to schedule something that
   * occurs only in a different condition branch.)
   */
  {
    CONDITION icond = instance_condition(in);
    if ((cond.positive|icond.positive)&
	(cond.negative|icond.negative)) {
      if (oag_debug & PROD_ORDER) {
	int i=n-remaining;
	for (; i > 0; --i) printf("  ");
	print_instance(in,stdout);
	puts(" (ignored)");
      }
      aug_graph->schedule[i] = 1;
      cto_node = schedule_rest(aug_graph,prev,cond,remaining-1);
      aug_graph->schedule[i] = 0;
      return cto_node;
    }
  }

  if (oag_debug & PROD_ORDER) {
    int i=n-remaining;
    for (; i > 0; --i) printf("  ");
    print_instance(in,stdout);
    putchar('\n');
  }

  cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
  cto_node->cto_prev = prev;
  cto_node->cto_instance = in;

  aug_graph->schedule[i] = 1;
  if (if_rule_p(in->fibered_attr.attr)) {
    int cmask = 1 << (if_rule_index(in->fibered_attr.attr));
    cond.negative |= cmask;
    cto_node->cto_if_false =
      schedule_rest(aug_graph,cto_node,cond,remaining-1);
    cond.negative &= ~cmask;
    cond.positive |= cmask;
    cto_node->cto_if_true =
      schedule_rest(aug_graph,cto_node,cond,remaining-1);
    cond.positive &= ~cmask;
  } else {
    cto_node->cto_next = schedule_rest(aug_graph,cto_node,cond,remaining-1);
  }
  aug_graph->schedule[i] = 0;

  return cto_node;
}

#define CONDITION_IS_IMPOSSIBLE(cond) ((cond).positive & (cond).negative)
#define MERGED_CONDITION_IS_IMPOSSIBLE(cond1, cond2) (((cond1).positive|(cond2).positive) & ((cond1).negative|(cond2).negative))

/**
 * Utility function to print indent with single space character
 * @param indent indent count
 * @param stream output stream
 */ 
static void print_indent(int count, FILE *stream)
{
  while (count-- > 0) fprintf(stream, " ");
}

/**
 * Utility function to print the static schedule
 * @param cto CTO node
 * @param indent current indent count
 * @param stream output stream
 */ 
static void print_total_order(CTO_NODE *cto, int indent, FILE *stream)
{
  if (cto == NULL) return;

  print_indent(indent, stream);
  if (cto->cto_instance == NULL)
  {
    fprintf(stream, "visit marker");
    if (cto->child_decl != NULL) fprintf(stream, " (%s) ", decl_name(cto->child_decl));
  }
  else
  {
    print_instance(cto->cto_instance, stream);
  }
  fprintf(stream, " <%d,%d>", cto->child_phase.ph, cto->child_phase.ch);
  fprintf(stream, "\n");
  indent++;

  if (cto->cto_if_true != NULL)
  {
    print_indent(indent-1, stream);
    fprintf(stream, "(true)\n");
    print_total_order(cto->cto_if_true, indent, stdout);
    fprintf(stream, "\n");
    print_indent(indent-1, stream);
    fprintf(stream, "(false)\n");
  }

  print_total_order(cto->cto_next, indent, stdout);
}

/**
 * Utility function to print debug info before raising fatal error
 * @param prev CTO node
 * @param stream output stream
 */ 
static void print_error_debug(AUG_GRAPH *aug_graph, CHILD_PHASE* instance_groups, CTO_NODE* prev, FILE *stream)
{
  fprintf(stderr, "Instances (%s):\n", decl_name(aug_graph->syntax_decl));

  int i;
  for (i = 0; i < aug_graph->instances.length; i++)
  {
    print_instance(&aug_graph->instances.array[i], stream);
    fprintf(stream, "<%d, %d>\n", instance_groups[i].ph, instance_groups[i].ch);
  }

  fprintf(stream, "Schedule so far:\n");
  // For debugging purposes, traverse all the way back
  while (prev != NULL && prev->cto_prev != NULL) prev = prev->cto_prev;

  print_total_order(prev, 0, stream);
}

/**
 * Utility function to determine if instance group is local or not
 * @param group instance group <ph,ch>
 * @return boolean indicating if instance group is local
 */ 
static bool group_is_local(CHILD_PHASE* group)
{
  return group->ph == 0 && group->ch == 0;
}

/**
 * Utility function to determine if instance is local or not
 * @param aug_graph Augmented dependency graph
 * @param instance_groups array of <ph,ch>
 * @param i instance index to test
 * @return boolean indicating if instance is local
 */ 
static bool instance_is_local(AUG_GRAPH *aug_graph, CHILD_PHASE* instance_groups, const int i)
{
  CHILD_PHASE group = instance_groups[i];
  return group_is_local(&group);
}

/**
 * Utility function to check if two child phase are equal
 * @param group_key1 first child phase struct
 * @param group_key2 second child phase struct
 * @return boolean indicating if two child phase structs are equal
 */ 
static bool child_phases_are_equal(CHILD_PHASE* group_key1, CHILD_PHASE* group_key2)
{
  return group_key1->ph == group_key2->ph && group_key1->ch == group_key2->ch;
}

/**
 * Returns true if two attribute instances belong to the same group
 * @param aug_graph Augmented dependency graph
 * @param cond current condition
 * @param instance_groups array of <ph,ch>
 * @param i instance index to test
 * @return boolean indicating if two attributes are in the same group
 */ 
static bool instances_in_same_group(AUG_GRAPH *aug_graph, CHILD_PHASE* instance_groups, const int i, const int j)
{
  CHILD_PHASE *group_key1 = &instance_groups[i];
  CHILD_PHASE *group_key2 = &instance_groups[j];

  // Both are locals
  if (instance_is_local(aug_graph, instance_groups, i) && instance_is_local(aug_graph, instance_groups, j))
  {
    return i == j;
  }
  // Anything else
  else
  {
    return child_phases_are_equal(group_key1, group_key2);
  }
}

/**
 * Given a geneirc instance index it returns boolean indicating if its ready to be scheduled or not
 * @param aug_graph Augmented dependency graph
 * @param cond current condition
 * @param instance_groups array of <ph,ch>
 * @param i group index
 * @param j instance index to test
 * @return boolean indicating whether instance is ready to be scheduled
 */
static bool instance_ready_to_go(AUG_GRAPH *aug_graph, CHILD_PHASE* instance_groups, const CONDITION cond, const int i, const int j)
{
  int k;
  EDGESET edges;
  int n = aug_graph->instances.length;
  
  for (k = 0; k < n; k++)
  {
    // Already scheduled then ignore
    if (aug_graph->schedule[k] != 0) continue;

    // If from the same group then ignore
    if (instances_in_same_group(aug_graph, instance_groups, i, k)) continue;

    int index = k * n + j;  // k (source) >--> j (sink) edge

    /* Look at all dependencies from j to i */
    for (edges = aug_graph->graph[index]; edges != NULL; edges=edges->rest)
    {
      /* If the merge condition is impossible, ignore this edge */
      if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, edges->cond)) continue;

      if (oag_debug & DEBUG_ORDER)
      {
        // Can't continue with scheduling if a dependency with a "possible" condition has not been scheduled yet
        printf("This edgeset was not ready to be scheduled because of:\n");
        print_edgeset(edges, stdout);
        printf("\n");
      }

      return false;
    }
  }

  return true;
}

/**
 * Given a generic instance index it returns boolean indicating if its ready to be scheduled or not
 * @param aug_graph Augmented dependency graph
 * @param cond current condition
 * @param instance_groups array of <ph,ch>
 * @param i instance index to test
 * @return boolean indicating whether group is ready to be scheduled
 */
static bool group_ready_to_go(AUG_GRAPH *aug_graph, CHILD_PHASE* instance_groups, const CONDITION cond, const int i)
{
  if (oag_debug & DEBUG_ORDER)
  {
    printf("Checking group readyness of: ");
    print_instance(&aug_graph->instances.array[i], stdout);
    printf("\n");
  }

  int n = aug_graph->instances.length;
  CHILD_PHASE group = instance_groups[i];
  int j;

  for (j = 0; j < n; j++)
  {
    CHILD_PHASE current_group = instance_groups[j];

    // Instance in the same group but cannot be considered
    if (instances_in_same_group(aug_graph, instance_groups, i, j))
    {
      if (aug_graph->schedule[j] != 0) continue;  // already scheduled

      if (!instance_ready_to_go(aug_graph, instance_groups, cond, i, j))
      {
        return false;
      }
    }
  }

  return true;
}

/**
 * Simple function to check if there is more to schedule in the group of index
 * @param aug_graph Augmented dependency graph
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param parent_group parent group key
 * @return boolean indicating if there is more in this group that needs to be scheduled
 */
static bool is_there_more_to_schedule_in_group(AUG_GRAPH *aug_graph, CHILD_PHASE* instance_groups, CHILD_PHASE *parent_group)
{
  int n = aug_graph->instances.length;
  int i;
  for (i = 0; i < n; i++)
  {
    // Instance in the same group but cannot be considered
    CHILD_PHASE *group_key = &instance_groups[i];

    // Check if in the same group
    if (child_phases_are_equal(parent_group, group_key))
    {
      if (aug_graph->schedule[i] == 0) return true;
    }
  }

  return false;
}

/**
 * Simple function to check if there is more to schedule in the group of index
 * @param aug_graph Augmented dependency graph
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param parent_group parent group key
 * @param min_ch min value for ch
 * @return boolean indicating if there is more in this group that needs to be scheduled
 */
static BOOL is_there_more_to_schedule_in_phase(AUG_GRAPH *aug_graph, CHILD_PHASE* instance_groups, short phase, short min_ch)
{
  int n = aug_graph->instances.length;
  int i;
  for (i = 0; i < n; i++)
  {
    // Instance in the same group but cannot be considered
    CHILD_PHASE *group_key = &instance_groups[i];

    // Check if in the same group
    if (abs(phase) == abs(group_key->ph) && group_key->ch >= min_ch)
    {
      if (aug_graph->schedule[i] == 0) return TRUE;
    }
  }

  return FALSE;
}

// Signature of function
static CTO_NODE* schedule_visits(AUG_GRAPH *aug_graph, CTO_NODE* prev, CONDITION cond, CHILD_PHASE* instance_groups, int remaining, CHILD_PHASE *group);
static CTO_NODE* schedule_visits_group(AUG_GRAPH *aug_graph, CTO_NODE* prev, CONDITION cond, CHILD_PHASE* instance_groups, int remaining, CHILD_PHASE *group);

/**
 * Function that throws an error if locals are scheduled out of order
 * @param aug_graph Augmented dependency graph
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 */
static void assert_locals_order(AUG_GRAPH *aug_graph, CHILD_PHASE* instance_groups)
{
  int n = aug_graph->instances.length;
  int i, j;

  for (i = 0; i < n; i++)
  {
    if (instance_is_local(aug_graph, instance_groups, i) && aug_graph->schedule[i])
    {
      for (j = 0; j < i; j++)
      {
        if (instance_is_local(aug_graph, instance_groups, j) && !aug_graph->schedule[j])
        {
          fprintf(stderr, "Scheduled local:\n\t");
          print_instance(&aug_graph->instances.array[i], stderr);
          fprintf(stderr, "\nBefore scheduling:\n\t");
          print_instance(&aug_graph->instances.array[j], stderr);
          fprintf(stderr, "\n");

          fatal_error("Scheduling local attribute instances in an out of order fashion.");
        }
      }
    }
  }
}

/**
 * Utility function to greedy schedule as many locals as possible
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_locals(AUG_GRAPH *aug_graph, CTO_NODE* prev, CONDITION cond, CHILD_PHASE* instance_groups, int remaining, CHILD_PHASE* prev_group)
{
  int i;
  int n = aug_graph->instances.length;
  int sane_remaining = 0;
  CTO_NODE* cto_node = prev;

  for (i = 0; i < n; i++)
  {
    INSTANCE *instance = &aug_graph->instances.array[i];
    CHILD_PHASE *group = &instance_groups[i];

    // Already scheduled OR instance is not local then ignore
    if (aug_graph->schedule[i] != 0 || !instance_is_local(aug_graph, instance_groups, i))
    {
      continue;
    }

    sane_remaining++;

    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = instance;
    cto_node->child_phase = instance_groups[i];

    aug_graph->schedule[i] = 1; // instance has been scheduled (and will not be considered for scheduling in the recursive call)

    if (if_rule_p(instance->fibered_attr.attr))
    {
      int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
      cond.negative |= cmask;
      cto_node->cto_if_false = schedule_locals(aug_graph, cto_node, cond, instance_groups, remaining-1, prev_group);
      cond.negative &= ~cmask;
      cond.positive |= cmask;
      cto_node->cto_if_true = schedule_locals(aug_graph, cto_node, cond, instance_groups, remaining-1, prev_group);
      cond.positive &= ~cmask;
    }
    else
    {
      cto_node->cto_next = schedule_locals(aug_graph, cto_node, cond, instance_groups, remaining-1, prev_group);
    }

    aug_graph->schedule[i] = 0; // Release it

    return cto_node;
  }

  // Fall back to normal scheduler
  return schedule_visits(aug_graph, prev, cond, instance_groups, remaining /* no change */, prev_group);
}

/**
 * Utility function to handle transitions between groups
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_transitions(AUG_GRAPH *aug_graph, CTO_NODE* prev, CONDITION cond, CHILD_PHASE* instance_groups, int remaining, CHILD_PHASE *group)
{
  // If we find ourselves scheduling a <-ph,ch>, we need to (after putting in all the
  // instances in that group), we need to schedule the visit of the child (add a CTO
  // node with a null instance but with <ph,ch) and then ALSO schedule immediately
  // all the syn attributes of that child's phase. (<+ph,ch> group, if any).
  if (group->ph < 0 && group->ch > -1)
  {
    // Visit marker
    CTO_NODE *cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = -group->ph;
    cto_node->child_phase.ch = group->ch;
    cto_node->child_decl = aug_graph->children[group->ch];
    cto_node->cto_next = schedule_visits_group(aug_graph, cto_node, cond, instance_groups, remaining, &(cto_node->child_phase));
    return cto_node;
  }

  // If we find ourselves scheduling a <+ph,-1>, this means that after we put all these ones in the schedule
  // (which we should do as a group NOW), this visit phase is over. And we should mark it with a <ph,-1> marker in the CTO.
  if (group->ph > 0 && group->ch == -1 && FALSE)
  {
    // Visit marker
    CTO_NODE *cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = group->ph;
    cto_node->child_phase.ch = -1;
    cto_node->cto_next = schedule_visits(aug_graph, cto_node, cond, instance_groups, remaining /* no change */, group);

    return cto_node;
  }

  // If we find ourselves scheduling <-ph,-1> and there is nothing else to do in this group then work on the first child if any
  if (group->ph < 0 && group->ch == -1)
  {
    CHILD_PHASE *child_group = (CHILD_PHASE*)HALLOC(sizeof(CHILD_PHASE));
    child_group->ph = group->ph;
    child_group->ch = 0;

    // This check is needed to prevent duplicate visit marker because we should not trigger
    // group scheduler if there is nothing to be done in that group, doing so will have
    // unwanted consequences.
    if (is_there_more_to_schedule_in_group(aug_graph, instance_groups, child_group))
    {
      return schedule_visits_group(aug_graph, prev, cond, instance_groups, remaining, child_group);
    }
  }

  // Don't leave this phase without completing everything
  // Probe whether there is more child attribute instance in this phase ready to be scheduled
  if (is_there_more_to_schedule_in_phase(aug_graph, instance_groups, group->ph, group->ch + 1))
  {
    CHILD_PHASE *child_group = (CHILD_PHASE*)HALLOC(sizeof(CHILD_PHASE));
    child_group->ph = -abs(group->ph); // inherited attribute
    child_group->ch = group->ch + 1;   // start from the next child

    if (is_there_more_to_schedule_in_group(aug_graph, instance_groups, child_group))
    {
      return schedule_visits_group(aug_graph, prev, cond, instance_groups, remaining, child_group);
    }
  }

  // Try to schedule local if any before falling back and call schedule_visits
  return schedule_locals(aug_graph, prev, cond, instance_groups, remaining /* no change */, group);
}

/**
 * Utility function to handle visit end marker
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @return head of linked list
 */
static CTO_NODE* schedule_visit_end(AUG_GRAPH *aug_graph, CTO_NODE* prev, CHILD_PHASE* group)
{
  CTO_NODE* cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
  cto_node->cto_prev = prev;
  cto_node->cto_instance = NULL;
  cto_node->child_phase.ph = abs(group->ph);
  cto_node->child_phase.ch = -1;
  return cto_node;
}

/**
 * Recursive scheduling function
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_visits_group(AUG_GRAPH *aug_graph, CTO_NODE* prev, CONDITION cond, CHILD_PHASE* instance_groups, int remaining, CHILD_PHASE *group)
{
  int i;
  int n = aug_graph->instances.length;
  int sane_remaining = 0;
  CTO_NODE* cto_node = prev;
  
  /* If nothing more to do, we are done. */
  if (remaining == 0)
  {
    return schedule_visit_end(aug_graph, prev, group);
  }

  /* Outer condition is impossible, its a dead-end branch */
  if (CONDITION_IS_IMPOSSIBLE(cond)) return NULL;

  for (i = 0; i < n; i++)
  {
    INSTANCE *instance = &aug_graph->instances.array[i];
    CHILD_PHASE *instance_group = &instance_groups[i];

    // Already scheduled then ignore
    if (aug_graph->schedule[i] != 0) continue;

    sane_remaining++;

    // Check if everything is in the same group, don't check for dependencies
    // Locals will never end-up in this function
    if (instance_group->ph == group->ph && instance_group->ch == group->ch)
    {
      cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
      cto_node->cto_prev = prev;
      cto_node->cto_instance = instance;
      cto_node->child_phase = *group;

      aug_graph->schedule[i] = 1; // instance has been scheduled (and will not be considered for scheduling in the recursive call)

      assert_locals_order(aug_graph, instance_groups);

      if (if_rule_p(instance->fibered_attr.attr))
      {
        int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
        cond.negative |= cmask;
        cto_node->cto_if_false = schedule_visits(aug_graph, cto_node, cond, instance_groups, remaining-1, group);
        cond.negative &= ~cmask;
        cond.positive |= cmask;
        cto_node->cto_if_true = schedule_visits(aug_graph, cto_node, cond, instance_groups, remaining-1, group);
        cond.positive &= ~cmask;
      }
      else
      {
        cto_node->cto_next = schedule_visits_group(aug_graph, cto_node, cond, instance_groups, remaining-1, group);
      }

      aug_graph->schedule[i] = 0; // Release it

      return cto_node;
    }
  }

  // Group is finished
  if (!is_there_more_to_schedule_in_group(aug_graph, instance_groups, group))
  {
    return schedule_transitions(aug_graph, cto_node, cond, instance_groups, remaining, group);
  }

  fflush(stdout);
  if (sane_remaining != remaining)
  {
    fprintf(stderr,"remaining out of sync %d != %d\n", sane_remaining, remaining);
  }

  print_error_debug(aug_graph, instance_groups, prev, stderr);
  fatal_error("Cannot make conditional total order!");

  return NULL;
}

/**
 * Recursive scheduling function
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param remaining count of remaining instances to schedule
 * @return head of linked list
 */
static CTO_NODE* schedule_visits(AUG_GRAPH *aug_graph, CTO_NODE* prev, CONDITION cond, CHILD_PHASE* instance_groups, int remaining, CHILD_PHASE *prev_group)
{
  int i;
  int n = aug_graph->instances.length;
  int sane_remaining = 0;
  CTO_NODE* cto_node = NULL;

  bool just_started = remaining == n;
  
  /* If nothing more to do, we are done. */
  if (remaining == 0)
  {
    return schedule_visit_end(aug_graph, prev, prev_group);
  }

  /* Outer condition is impossible, its a dead-end branch */
  if (CONDITION_IS_IMPOSSIBLE(cond)) return NULL;

  for (i = 0; i < n; i++)
  {
    INSTANCE *instance = &aug_graph->instances.array[i];
    CHILD_PHASE *group = &instance_groups[i];

    // Already scheduled then ignore
    if (aug_graph->schedule[i] != 0) continue;

    sane_remaining++;

    // If edgeset condition is not impossible then go ahead with scheduling
    if (group_ready_to_go(aug_graph, instance_groups, cond, i))
    {
      // If it is local then continue scheduling
      if (instance_is_local(aug_graph, instance_groups, i))
      {
        cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
        cto_node->cto_prev = prev;
        cto_node->cto_instance = instance;
        cto_node->child_phase = *group;

        aug_graph->schedule[i] = 1; // instance has been scheduled (and will not be considered for scheduling in the recursive call)

        assert_locals_order(aug_graph, instance_groups);

        if (if_rule_p(instance->fibered_attr.attr))
        {
          int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
          cond.negative |= cmask;
          cto_node->cto_if_false = schedule_visits(aug_graph, cto_node, cond, instance_groups, remaining-1, group);
          cond.negative &= ~cmask;
          cond.positive |= cmask;
          cto_node->cto_if_true = schedule_visits(aug_graph, cto_node, cond, instance_groups, remaining-1, group);
          cond.positive &= ~cmask;
        }
        else
        {
          cto_node->cto_next = schedule_visits(aug_graph, cto_node, cond, instance_groups, remaining-1, group);
        }

        aug_graph->schedule[i] = 0; // Release it

        return cto_node;
      }
      // Instance is not local then delegate it to group scheduler
      else
      {
        // If phase has changed since previous group then we need a end of phase visit marker
        // This is needed when for example we have <2,-1> and we start <3,0>
        if (abs(group->ph) > abs(prev_group->ph))
        {
          CTO_NODE *cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
          cto_node->cto_prev = prev;
          cto_node->cto_instance = NULL;
          cto_node->child_phase.ph = abs(prev_group->ph);
          cto_node->child_phase.ch = -1;
          cto_node->cto_next = schedule_visits_group(aug_graph, prev, cond, instance_groups, remaining, group);
          return cto_node;
        }

        return schedule_visits_group(aug_graph, prev, cond, instance_groups, remaining, group);
      }
    }
  }

  fflush(stdout);
  if (sane_remaining != remaining)
  {
    fprintf(stderr, "remaining out of sync %d != %d\n", sane_remaining, remaining);
  }

  print_error_debug(aug_graph, instance_groups, prev, stderr);
  fatal_error("Cannot make conditional total order!");

  return NULL;
}

/**
 * Utility function to get children of augmented dependency graph as array of declarations
 * @param aug_graph Augmented dependency graph
 */
static Declaration* get_aug_graph_children(AUG_GRAPH *aug_graph)
{
  Declaration source = aug_graph->match_rule;
  switch (Declaration_KEY(source))
  {
  case KEYtop_level_match:
  {
    int count = 0;
    Declaration formal = top_level_match_first_rhs_decl(source);
    while (formal != NULL)
    {
      count++;
      formal = DECL_NEXT(formal);
    }

    int i = 0;
    Declaration* result = (Declaration*)HALLOC(sizeof(Declaration) * count);
    formal = top_level_match_first_rhs_decl(source);
    while (formal != NULL)
    {
      result[i++] = formal;
      formal = DECL_NEXT(formal);
    }

    return result;
  }
  case KEYsome_class_decl:
  {
    int count = 0;
    Declaration child = first_Declaration(block_body(some_class_decl_contents(source)));
    while (child != NULL)
    {
      if (Declaration_KEY(child) == KEYvalue_decl)
      {
        count++;
      }
      child = DECL_NEXT(child);
    }

    int i = 0;
    Declaration* result = (Declaration*)HALLOC(sizeof(Declaration) * count);
    child = first_Declaration(block_body(some_class_decl_contents(source)));
    while (child != NULL)
    {
      if (Declaration_KEY(child) == KEYvalue_decl)
      {
        result[i++] = child;
      }
      child = DECL_NEXT(child);
    }

    return result;
  }
  case KEYsome_function_decl:
  {
    int count = 0;
    Declaration formal = first_Declaration(function_type_formals(some_function_decl_type(source)));
    while (formal != NULL)
    {
      count++;
      formal = DECL_NEXT(formal);
    }

    int i = 0;
    Declaration* result = (Declaration*)HALLOC(sizeof(Declaration) * count);
    formal = first_Declaration(function_type_formals(some_function_decl_type(source)));
    while (formal != NULL)
    {
      result[i++] = formal;
      formal = DECL_NEXT(formal);
    }
    
    return result;
  }
  }

  fatal_error("Failed to create children array for augment dependency graph: %s", decl_name(aug_graph->syntax_decl));

  return NULL;
}

/**
 * Return phase (synthesized) or -phase (inherited)
 * for fibered attribute, given the phylum's summary dependence graph.
 */
int attribute_schedule(PHY_GRAPH *phy_graph, FIBERED_ATTRIBUTE* key)
{
  int n = phy_graph->instances.length;
  for (int i=0; i < n; ++i) {
    FIBERED_ATTRIBUTE* fa = &(phy_graph->instances.array[i].fibered_attr);
    if (fa->attr == key->attr && fa->fiber == key->fiber)
      return phy_graph->summary_schedule[i];
  }
  fatal_error("Could not find summary schedule for instance");
  return 0;
}

void schedule_augmented_dependency_graph(AUG_GRAPH *aug_graph) {
  int n = aug_graph->instances.length;
  CONDITION cond;
  int i, j;

  (void)close_augmented_dependency_graph(aug_graph);

  /** Now schedule graph: we need to generate a conditional total order. */

  if (oag_debug & PROD_ORDER) {
    printf("Scheduling conditional total order for %s\n",
	   aug_graph_name(aug_graph));
  }
  if (oag_debug & DEBUG_ORDER) {
    for (int i=0; i <= n; ++i) {
      INSTANCE *in = &(aug_graph->instances.array[i]);
      print_instance(in,stdout);
      printf(": ");
      Declaration ad = in->fibered_attr.attr;
      Declaration chdecl;
    
      int j = 0, ch = -1;
      for (chdecl = aug_graph->first_rhs_decl; chdecl != 0; chdecl=DECL_NEXT(chdecl)) {
	if (in->node == chdecl) ch = j;
	++j;
      }
      if (in->node == aug_graph->lhs_decl || ch >= 0) {
	PHY_GRAPH *npg = Declaration_info(in->node)->node_phy_graph;
	int ph = attribute_schedule(npg,&(in->fibered_attr));
	printf("<%d,%d>\n",ph,ch);
      } else {
	printf("local\n");
      }
    }
  }

  size_t instance_groups_size = n * sizeof(CHILD_PHASE);
  CHILD_PHASE* instance_groups = (CHILD_PHASE*) alloca(instance_groups_size);
  memset(instance_groups, (int)0, instance_groups_size);

  for (i = 0; i < n; i++)
  {
    INSTANCE *in = &(aug_graph->instances.array[i]);
    Declaration ad = in->fibered_attr.attr;
    Declaration chdecl;
  
    int j = 0, ch = -1;
    for (chdecl = aug_graph->first_rhs_decl; chdecl != 0; chdecl=DECL_NEXT(chdecl))
    {
      if (in->node == chdecl) ch = j;
      ++j;
    }

    if (in->node == aug_graph->lhs_decl || ch >= 0)
    {
      PHY_GRAPH *npg = Declaration_info(in->node)->node_phy_graph;
      int ph = attribute_schedule(npg,&(in->fibered_attr));
      instance_groups[i].ph = (short) ph;
      instance_groups[i].ch = (short) ch;
    }
  }

  // if (oag_debug & DEBUG_ORDER)
  {
    printf("\nInstances %s:\n", decl_name(aug_graph->syntax_decl));
    for (i = 0; i < n; i++)
    {
      INSTANCE *in = &(aug_graph->instances.array[i]);
      CHILD_PHASE group = instance_groups[i];
      print_instance(in, stdout);
      printf(": ");
      
      if (!group.ph && !group.ch)
      {
        printf("local\n");
      }
      else
      {
        printf("<%d,%d>\n", group.ph, group.ch);
      }
    }
  }

  // Find children of augmented graph: this will be used as argument to visit calls
  aug_graph->children = get_aug_graph_children(aug_graph);

  /* we use the schedule array as temp storage */
  for (i=0; i < n; ++i) {
    aug_graph->schedule[i] = 0; /* This means: not scheduled yet */
  }

  cond.negative = 0;
  cond.positive = 0;

  CHILD_PHASE next_group = { -1, -1 };
  // It is safe to assume inherited attribute of parents have no dependencies and should be scheduled right away
  aug_graph->total_order = schedule_visits_group(aug_graph, NULL, cond, instance_groups, n, &next_group);

  if (aug_graph->total_order == NULL)
  {
    fatal_error("Failed to create total order.");
  }

  // if (oag_debug & DEBUG_ORDER)
  {
    printf("\nSchedule\n");
    printf("syntax_decl %s\n", decl_name(aug_graph->syntax_decl));
    print_total_order(aug_graph->total_order, 0, stdout);
  }
}

void compute_oag(Declaration module, STATE *s) {
  int j;
  for (j=0; j < s->phyla.length; ++j) {
    schedule_summary_dependency_graph(&s->phy_graphs[j]);

    /* now perform closure */
    {
      int saved_analysis_debug = analysis_debug;
      int j;

      if (oag_debug & TYPE_3_DEBUG) {
	analysis_debug |= TWO_EDGE_CYCLE;
      }

      if (analysis_debug & DNC_ITERATE) {
	printf("\n**** After OAG schedule for phylum %d:\n\n",j);
      }

      if (analysis_debug & ASSERT_CLOSED) {
	for (j=0; j < s->match_rules.length; ++j) {
	  printf("Checking rule %d\n",j);
	  assert_closed(&s->aug_graphs[j]);
	}
      }

      dnc_close(s);

      analysis_debug = saved_analysis_debug;
    }
    if (analysis_debug & DNC_ITERATE) {
      printf ("\n*** After closure after schedule OAG phylum %d\n\n",j);
      print_analysis_state(s,stdout);
      print_cycles(s,stdout);
    }
      
    if (analysis_state_cycle(s)) break;
  }

  if (!analysis_state_cycle(s)) {
    for (j=0; j < s->match_rules.length; ++j) {
      schedule_augmented_dependency_graph(&s->aug_graphs[j]);
    }
    schedule_augmented_dependency_graph(&s->global_dependencies);
  }

  if (analysis_debug & (DNC_ITERATE|DNC_FINAL)) {
    printf("*** FINAL OAG ANALYSIS STATE ***\n");
    print_analysis_state(s,stdout);
    print_cycles(s,stdout);
  }
}
