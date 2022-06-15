#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aps-ag.h"
#include "jbb-alloc.h"
#include "jbb.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define IS_VISIT_MARKER(node) (node->cto_instance == NULL)

// <ph,ch> representing the parent inherited attributes
CHILD_PHASE parent_inherited_group = {-1, -1};

// Utility struct to keep of track of information needed to handle group
// scheduling
struct total_order_state {
  // Max parent phase
  short max_parent_ph;
  // One-d array, max child phase indexed by child index
  short* max_child_ph;
  // Tuple <ph,ch> indexed by instance number
  CHILD_PHASE* instance_groups;
  // One-d array, indexed by instance number
  bool* schedule;
  // Children Declaration array
  VECTOR(Declaration)
  children;
  // One-d array indicating child visit for a phase happened or not
  bool** child_visit_markers;

  // One-d array indicating child visit for a phase happened or not
  bool* parent_visit_markers;

  bool** child_inh_investigations;

  bool* parent_synth_investigations;
};

typedef struct total_order_state TOTAL_ORDER_STATE;

static CTO_NODE* group_schedule_circular(AUG_GRAPH* aug_graph,
                                         SCC_COMPONENT* comp,
                                         const int comp_index,
                                         CTO_NODE* prev,
                                         CONDITION cond,
                                         TOTAL_ORDER_STATE* state,
                                         const int remaining,
                                         CHILD_PHASE* group,
                                         const short parent_ph);

static CTO_NODE* group_schedule_noncircular(AUG_GRAPH* aug_graph,
                                            CTO_NODE* prev,
                                            CONDITION cond,
                                            TOTAL_ORDER_STATE* state,
                                            const int remaining,
                                            CHILD_PHASE* group,
                                            const short parent_ph);

static CTO_NODE* greedy_schedule_circular(AUG_GRAPH* aug_graph,
                                          SCC_COMPONENT* comp,
                                          const int comp_index,
                                          CTO_NODE* prev,
                                          CONDITION cond,
                                          TOTAL_ORDER_STATE* state,
                                          const int remaining,
                                          CHILD_PHASE* prev_group,
                                          const short parent_ph);

static CTO_NODE* greedy_schedule_noncircular(AUG_GRAPH* aug_graph,
                                             CTO_NODE* prev,
                                             CONDITION cond,
                                             TOTAL_ORDER_STATE* state,
                                             int remaining,
                                             CHILD_PHASE* prev_group,
                                             const short parent_ph);

static CTO_NODE* schedule_visit_end(AUG_GRAPH* aug_graph,
                                    SCC_COMPONENT* comp,
                                    const int comp_index,
                                    CTO_NODE* prev,
                                    CONDITION cond,
                                    TOTAL_ORDER_STATE* state,
                                    const int remaining,
                                    const short parent_ph,
                                    const bool circular);

static void find_scc_to_schedule(AUG_GRAPH* aug_graph,
                                 CTO_NODE* prev,
                                 CONDITION cond,
                                 TOTAL_ORDER_STATE* state,
                                 const int remaining,
                                 CHILD_PHASE* group,
                                 const short parent_ph,
                                 SCC_COMPONENT** comp,
                                 int* comp_index);

static CTO_NODE* schedule_transition(AUG_GRAPH* aug_graph,
                                     SCC_COMPONENT* comp,
                                     const int comp_index,
                                     CTO_NODE* prev,
                                     CONDITION cond,
                                     TOTAL_ORDER_STATE* state,
                                     int remaining,
                                     CHILD_PHASE* group,
                                     const short parent_ph,
                                     const bool circular);
/**
 * Utility function that checks whether instance belongs to any phylum cycle
 * or not
 * @param phy_graph phylum graph
 * @param in attribute instance
 * @return -1 if instance does not belong to any cycle or index of phylum
 * cycle
 */
static int instance_in_phylum_cycle(PHY_GRAPH* phy_graph, INSTANCE* in) {
  int i, j;
  for (i = 0; i < phy_graph->cycles.length; i++) {
    CYCLE* cyc = &phy_graph->cycles.array[i];
    for (j = 0; j < cyc->instances.length; j++) {
      INSTANCE* other = &cyc->instances.array[j];
      if (in->index == other->index) {
        return i;
      }
    }
  }

  return -1;
}

/**
 * Utility function that schedules a single phase of all circular attributes
 * @param phy_graph phylum graph
 * @param ph phase its currently scheduling for
 * @return number of nodes scheduled successfully for this phase
 */
static int schedule_phase_circular(PHY_GRAPH* phy_graph, int phase) {
  int n = phy_graph->instances.length;
  int i, j, k;

  for (i = 0; i < phy_graph->cycles.length; i++) {
    CYCLE* cyc = &phy_graph->cycles.array[i];

    // This cycle is already scheduled
    if (phy_graph->summary_schedule[cyc->instances.array[0].index])
      continue;

    size_t temp_schedule_size = n * sizeof(int);
    int* temp_schedule = (int*)alloca(temp_schedule_size);
    memcpy(temp_schedule, phy_graph->summary_schedule, temp_schedule_size);

    // Temporarily mark all attributes in this cycle as scheduled
    for (j = 0; j < cyc->instances.length; j++) {
      INSTANCE* in = &cyc->instances.array[j];
      temp_schedule[in->index] = 1;
    }

    bool cycle_ready = true;

    for (j = 0; j < cyc->instances.length; j++) {
      INSTANCE* in = &cyc->instances.array[j];
      for (k = 0; k < n; k++) {
        // If there is an attribute that is not scheduled but it should
        // come first then this cycle is not ready
        if (!temp_schedule[k] &&
            phy_graph->mingraph[k * n + in->index] != no_dependency) {
          cycle_ready = false;
        }
      }
    }

    // If all attributes in this cycle are ready
    if (cycle_ready) {
      for (j = 0; j < cyc->instances.length; j++) {
        INSTANCE* in = &cyc->instances.array[j];
        // -phase for inherited attributes
        // +phase for synthesized attributes
        phy_graph->summary_schedule[in->index] =
            instance_direction(in) == instance_inward ? -phase : phase;

        if (oag_debug & TOTAL_ORDER) {
          printf("%+d ", phy_graph->summary_schedule[in->index]);
          print_instance(in, stdout);
          printf("\n");
        }
      }

      // Force extra dependencies between instances that are already
      // scheduled and attributes in this cycle
      for (j = 0; j < cyc->instances.length; j++) {
        INSTANCE* in = &cyc->instances.array[j];
        for (k = 0; k < n; k++) {
          int sch = phy_graph->summary_schedule[k];
          // Add edge from already scheduled instances to instances in
          // the cycle (not to self)
          if (sch != 0 && in->index != k) {
            phy_graph->mingraph[k * n + in->index] =
                indirect_control_dependency;
          }
        }
      }

      return cyc->instances.length;
    }
  }

  return 0;
}

/**
 * Utility function that schedules a single phase of all non-circular
 * attributes
 * @param phy_graph phylum graph
 * @param ph phase its currently scheduling for
 * @return number of nodes scheduled successfully for this phase
 */
static int schedule_phase_non_circular(PHY_GRAPH* phy_graph, int phase) {
  int done = 0;
  int n = phy_graph->instances.length;
  int i, j;

  /* find inherited instances for the phase. */
  for (i = 0; i < n; i++) {
    INSTANCE* in = &phy_graph->instances.array[i];
    if (instance_direction(in) == instance_inward &&
        instance_in_phylum_cycle(phy_graph, in) == -1 &&
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
          printf("%+d ", phase);
          print_instance(in, stdout);
          printf("\n");
        }
      }
    }
  }

  /* now schedule synthesized attributes */
  for (i = 0; i < n; i++) {
    INSTANCE* in = &phy_graph->instances.array[i];
    if (instance_direction(in) == instance_outward &&
        instance_in_phylum_cycle(phy_graph, in) == -1 &&
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
          printf("%+d ", phase);
          print_instance(in, stdout);
          printf("\n");
        }
      }
    }
  }

  return done;
}

/**
 * Utility function that calculates ph (phase) for each attribute of a phylum
 * @param phy_graph phylum graph to schedule
 */
static void schedule_summary_dependency_graph(PHY_GRAPH* phy_graph) {
  int n = phy_graph->instances.length;
  int phase = 1;
  int done = 0;
  BOOL cont = true;

  if (oag_debug & TOTAL_ORDER) {
    printf("Scheduling order for %s\n", decl_name(phy_graph->phylum));
  }

  // Nothing is scheduled
  memset(phy_graph->summary_schedule, 0, n * sizeof(int));

  int i, j;
  int phase_count = n + 1;
  size_t phase_size_bool = phase_count * sizeof(BOOL);
  size_t phase_size_int = phase_count * sizeof(int);

  BOOL* circular_phase = (BOOL*)HALLOC(phase_size_bool);
  memset(circular_phase, false, phase_size_bool);

  BOOL* empty_phase = (BOOL*)HALLOC(phase_size_bool);
  memset(empty_phase, false, phase_size_bool);

  // Hold on to the flag indicating whether phase is circular or not
  phy_graph->cyclic_flags = circular_phase;
  phy_graph->empty_phase = empty_phase;

  int count_non_circular = 0, count_circular = 0;
  bool cycle_happened = false;
  do {
    // Schedule non-circular attributes in this phase
    count_non_circular = schedule_phase_non_circular(phy_graph, phase);
    if (count_non_circular) {
      done += count_non_circular;
      circular_phase[phase] = false;
      phy_graph->max_phase = phase;
      phase++;
      cycle_happened = false;

      if (oag_debug & TOTAL_ORDER) {
        printf("^^^ non-circular\n");
      }
      continue;
    } else if (cycle_happened || phase == 1) {
      if (oag_debug & TOTAL_ORDER) {
        printf("^^^ empty non-circular phase: %d\n", phase);
      }

      // Add an empty phase between circular and non-circular
      circular_phase[phase] = false;
      // Mark this phase as empty
      empty_phase[phase] = true;
      phy_graph->max_phase = phase;
      phase++;
      cycle_happened = false;
    }

    // Schedule circular attributes in this phase
    count_circular = schedule_phase_circular(phy_graph, phase);
    if (count_circular) {
      done += count_circular;
      circular_phase[phase] = true;
      cycle_happened = true;
      phy_graph->max_phase = phase;
      phase++;

      if (oag_debug & TOTAL_ORDER) {
        printf("^^^ circular\n");
      }
      continue;
    } else {
      cycle_happened = false;
    }
  } while (count_non_circular || count_circular);

  if (oag_debug & TOTAL_ORDER) {
    printf("\n");
  }

  if (done < n) {
    if (cycle_debug & PRINT_CYCLE) {
      printf("Failed to schedule phylum graph for: %s\n",
             decl_name(phy_graph->phylum));
      for (i = 0; i < n; ++i) {
        INSTANCE* in = &phy_graph->instances.array[i];
        int s = phy_graph->summary_schedule[i];
        print_instance(in, stdout);
        switch (instance_direction(in)) {
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
          printf(": phase %+d\n", s);
        } else {
          printf(" depends on ");
          for (j = 0; j < n; ++j) {
            if (phy_graph->summary_schedule[j] == 0 &&
                phy_graph->mingraph[j * n + i] != no_dependency) {
              INSTANCE* in2 = &phy_graph->instances.array[j];
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

    fatal_error("Cycle detected when scheduling phase %d for %s", phase,
                decl_name(phy_graph->phylum));
  }
}

/**
 * Utility function to print indent with single space character
 * @param indent indent count
 * @param stream output stream
 */
static void print_indent(int count, FILE* stream) {
  while (count-- >= 0) {
    fprintf(stream, "  ");
  }
}

/**
 * Utility function to print the static schedule
 * @param cto CTO node
 * @param indent current indent count
 * @param stream output stream
 */
static void print_total_order(CTO_NODE* cto,
                              int component_index,
                              int indent,
                              FILE* stream) {
  if (cto == NULL) {
    if (component_index != -1) {
      if (oag_debug & DEBUG_ORDER_VERBOSE) {
        fprintf(stream, " Finished scheduling SCC #%d\n\n", component_index);
      }
    }
    return;
  }
  bool extra_newline = false;
  bool print_group = true;

  if (cto->component != component_index) {
    if (component_index != -1) {
      if (oag_debug & DEBUG_ORDER_VERBOSE) {
        fprintf(stream, " Finished scheduling SCC #%d\n\n", component_index);
      }
    }

    component_index = cto->component;
    if (component_index != -1) {
      if (oag_debug & DEBUG_ORDER_VERBOSE) {
        fprintf(stream, " Started scheduling SCC #%d\n", component_index);
      }
    }
  }

  if (cto->cto_instance == NULL) {
    print_indent(indent, stream);
    if (cto->child_phase.ch != -1) {
      fprintf(stream, " ");
    }
    fprintf(stream, "visit marker (%d) ", cto->visit);
    if (cto->child_decl != NULL) {
      fprintf(stream, " (%s) ", decl_name(cto->child_decl));
    } else {
      extra_newline = true;
    }
  } else {
    print_indent(indent, stream);
    print_instance(cto->cto_instance, stream);
    CONDITION cond = instance_condition(cto->cto_instance);
    fprintf(stream, " (%d)", cto->visit);

    if (if_rule_p(cto->cto_instance->fibered_attr.attr)) {
      print_group = false;
    }
  }

  if (print_group) {
    fprintf(stream, " <%+d,%+d>", cto->child_phase.ph, cto->child_phase.ch);
  }

  fprintf(stream, "\n");
  if (extra_newline) {
    fprintf(stream, "\n");
  }

  if (cto->cto_if_true != NULL) {
    print_indent(indent + 1, stream);
    fprintf(stream, "(true)\n");
    print_total_order(cto->cto_if_true, component_index, indent + 2, stdout);
    print_indent(indent + 1, stream);
    fprintf(stream, "(false)\n");
    indent += 2;
  }

  print_total_order(cto->cto_next, component_index, indent, stdout);
}

/**
 * Utility function to print debug info before raising fatal error
 * @param prev CTO node
 * @param stream output stream
 */
static void print_schedule_error_debug(AUG_GRAPH* aug_graph,
                                       TOTAL_ORDER_STATE* state,
                                       CTO_NODE* prev,
                                       CONDITION cond,
                                       FILE* stream) {
  fprintf(stderr, "Instances (%s):\n", aug_graph_name(aug_graph));

  int i, j, k;
  int n = aug_graph->instances.length;
  for (i = 0; i < n; i++) {
    print_instance(&aug_graph->instances.array[i], stream);
    fprintf(stream, " <%+d,%+d> (%s)\n", state->instance_groups[i].ph,
            state->instance_groups[i].ch,
            state->schedule[i] ? "scheduled" : "not-scheduled");
  }

  fprintf(stderr, "\nNon-scheduled SCCs (%s):\n", aug_graph_name(aug_graph));

  for (i = 0; i < aug_graph->components.length; i++) {
    SCC_COMPONENT* comp = &aug_graph->components.array[i];

    if (state->schedule[comp->array[0]])
      continue;

    printf("Starting SCC #%d\n", i);
    for (j = 0; j < comp->length; j++) {
      INSTANCE* in = &aug_graph->instances.array[comp->array[j]];
      print_instance(&aug_graph->instances.array[in->index], stream);
      fprintf(stream, " <%+d,%+d>\n", state->instance_groups[in->index].ph,
              state->instance_groups[i].ch);
    }
    printf("Finished SCC #%d\n\n", i);
  }

  fprintf(stream, "\nSchedule so far for %s:\n", aug_graph_name(aug_graph));
  // For debugging purposes, traverse all the way back
  while (prev != NULL && prev->cto_prev != NULL) {
    prev = prev->cto_prev;
  }

  print_total_order(prev, -1, 0, stream);

  printf("Relevant dependencies from non-scheduled instances: \n");
  for (i = 0; i < n; i++) {
    if (state->schedule[i])
      continue;

    for (j = 0; j < n; j++) {
      if (state->schedule[j])
        continue;

      if (i == j)
        continue;

      EDGESET es = aug_graph->graph[j * n + i];

      if (es != NULL) {
        printf("Investigating why ");
        print_instance(&aug_graph->instances.array[i], stdout);
        printf(" has not been scheduled: \n");

        print_edgeset(aug_graph->graph[j * n + i], stdout);
        printf("\n");
      }
    }
  }
}

/**
 * Utility function to determine if instance group is local or not
 * @param group instance group <ph,ch>
 * @return boolean indicating if instance group is local
 */
static bool group_is_local(CHILD_PHASE* group) {
  return group->ph == 0 && group->ch == 0;
}

/**
 * Utility function to determine if instance is local or not
 * @param aug_graph Augmented dependency graph
 * @param instance_groups array of <ph,ch>
 * @param i instance index to test
 * @return boolean indicating if instance is local
 */
static bool instance_is_local(AUG_GRAPH* aug_graph,
                              TOTAL_ORDER_STATE* state,
                              const int i) {
  CHILD_PHASE* group = &state->instance_groups[i];
  return group_is_local(group);
}

/**
 * Utility function to check if two child phase are equal
 * @param group_key1 first child phase struct
 * @param group_key2 second child phase struct
 * @return boolean indicating if two child phase structs are equal
 */
static bool child_phases_are_equal(CHILD_PHASE* group_key1,
                                   CHILD_PHASE* group_key2) {
  return group_key1->ph == group_key2->ph && group_key1->ch == group_key2->ch;
}

/**
 * Returns true if two attribute instances belong to the same group
 * @param aug_graph Augmented dependency graph
 * @param cond current condition
 * @param state state
 * @param i instance index to test
 * @return boolean indicating if two attributes are in the same group
 */
static bool instances_in_same_group(AUG_GRAPH* aug_graph,
                                    TOTAL_ORDER_STATE* state,
                                    const int i,
                                    const int j) {
  CHILD_PHASE* group_key1 = &state->instance_groups[i];
  CHILD_PHASE* group_key2 = &state->instance_groups[j];

  // Both are locals
  if (instance_is_local(aug_graph, state, i) &&
      instance_is_local(aug_graph, state, j)) {
    return i == j;
  }
  // Anything else
  else {
    return child_phases_are_equal(group_key1, group_key2);
  }
}

/**
 * Given a generic instance index it returns boolean indicating if
 * attribute instance is ready to be scheduled or not
 * @param aug_graph Augmented dependency graph
 * @param scc SCC component
 * @param cond current condition
 * @param state state
 * @param group_index group index
 * @param attribute_index instance index to test
 * @return boolean indicating whether instance is ready to be scheduled
 */
static bool instance_ready_to_go(AUG_GRAPH* aug_graph,
                                 TOTAL_ORDER_STATE* state,
                                 const CONDITION cond,
                                 const int group_index,
                                 const int attribute_index) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf("Checking instance readiness of: ");
    print_instance(&aug_graph->instances.array[attribute_index], stdout);
    printf("\n");
  }

  int i;
  EDGESET edges;
  int n = aug_graph->instances.length;

  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];

    // Already scheduled then ignore
    if (state->schedule[in->index])
      continue;

    // If from the same group then ignore
    if (instances_in_same_group(aug_graph, state, group_index, in->index))
      continue;

    int index =
        in->index * n +
        attribute_index;  // in.index (source) >--> attribute_index (sink) edge

    // Look at all dependencies from this attribute instance that is not
    // scheduled to attribute_index
    for (edges = aug_graph->graph[index]; edges != NULL; edges = edges->rest) {
      // If the merge condition is impossible, ignore this edge
      if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, edges->cond))
        continue;

      DEPENDENCY dep = edges->kind;
      dep = dep & ~indirect_control_dependency;
      if (dep & DEPENDENCY_MAYBE_DIRECT) {
        if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
          // Can not continue with scheduling if a dependency with a
          // "possible" condition has not been scheduled yet
          printf(
              "This edgeset (%d) was not ready to be scheduled because of:\n",
              edges->kind);
          print_edgeset(edges, stdout);
          printf("\n");
        }

        return false;
      }
    }
  }

  return true;
}

/**
 * Given a generic instance index it returns boolean indicating if attribute
 * instance in a scc is ready to be scheduled or not
 * @param aug_graph Augmented dependency graph
 * @param comp SCC component
 * @param cond current condition
 * @param state state
 * @param group_index group index to test
 * @return boolean indicating whether group is ready to be scheduled
 */
static bool group_ready_to_go(AUG_GRAPH* aug_graph,
                              TOTAL_ORDER_STATE* state,
                              const CONDITION cond,
                              const int group_index) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf("Checking scc group readiness of: ");
    print_instance(&aug_graph->instances.array[group_index], stdout);
    printf("\n");
  }

  INSTANCE in = aug_graph->instances.array[group_index];
  int i;

  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];

    // already scheduled
    if (state->schedule[in->index])
      continue;

    // Instance in the same group but cannot be considered
    if (instances_in_same_group(aug_graph, state, group_index, in->index)) {
      if (!instance_ready_to_go(aug_graph, state, cond, group_index,
                                in->index)) {
        return false;
      }
    }
  }

  return true;
}

/**
 * Utility function to check if there is more to schedule in the group
 * @param aug_graph Augmented dependency graph
 * @param state state
 * @param parent_group parent group key
 * @return boolean indicating if there is more in this group that needs to be
 * scheduled
 */
static bool is_there_more_to_schedule_in_group(AUG_GRAPH* aug_graph,
                                               TOTAL_ORDER_STATE* state,
                                               CHILD_PHASE* parent_group) {
  int n = aug_graph->instances.length;
  int i;
  for (i = 0; i < n; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];

    // Instance in the same group but cannot be considered
    CHILD_PHASE* group_key = &state->instance_groups[in->index];

    // Check if in the same group
    if (child_phases_are_equal(parent_group, group_key)) {
      if (!state->schedule[in->index]) {
        return true;
      }
    }
  }

  return false;
}

/**
 * Utility function to check if there is more to schedule in the group of
 * scc
 * @param aug_graph Augmented dependency graph
 * @param comp SCC component
 * @param state state
 * @param parent_group parent group key
 * @return boolean indicating if there is more in this group that needs to be
 * scheduled
 */
static bool is_there_more_to_schedule_in_scc_group(AUG_GRAPH* aug_graph,
                                                   SCC_COMPONENT* comp,
                                                   TOTAL_ORDER_STATE* state,
                                                   CHILD_PHASE* parent_group) {
  int n = aug_graph->instances.length;
  int i;
  for (i = 0; i < comp->length; i++) {
    INSTANCE* in = &aug_graph->instances.array[comp->array[i]];

    // Instance in the same group but cannot be considered
    CHILD_PHASE* group_key = &state->instance_groups[in->index];

    // Check if in the same group
    if (child_phases_are_equal(parent_group, group_key)) {
      if (!state->schedule[in->index]) {
        return true;
      }
    }
  }

  return false;
}

/**
 * Utility function to check if there is more to schedule in SCC
 * @param aug_graph Augmented dependency graph
 * @param comp SCC component
 * @param state state
 * @return boolean indicating if there is more in this group that needs to be
 * scheduled
 */
static bool is_there_more_to_schedule_in_scc(AUG_GRAPH* aug_graph,
                                             SCC_COMPONENT* comp,
                                             TOTAL_ORDER_STATE* state) {
  int i;
  for (i = 0; i < comp->length; i++) {
    INSTANCE* in = &aug_graph->instances.array[comp->array[i]];

    // There is still instances that in this component that need to be scheduled
    if (!state->schedule[in->index]) {
      return true;
    }
  }

  return false;
}

/**
 * Utility function to look ahead in the total order
 * @param current CTO_NODE node
 * @param ph phase
 * @param ch child number
 * @param immediate immediately followed by
 * @param visit_marker_only look for visit marker only
 * @param any boolean pointer to set to true if found any
 */
static void followed_by(CTO_NODE* current,
                        const short ph,
                        const short ch,
                        const bool immediate,
                        bool visit_marker_only,
                        bool* any) {
  if (current == NULL)
    return;

  if (current->child_phase.ph == ph && current->child_phase.ch == ch) {
    if ((visit_marker_only && IS_VISIT_MARKER(current)) || !visit_marker_only) {
      *any = true;
    }
  } else if (immediate) {
    return;
  }

  bool if_true_any = false;
  bool if_false_any = false;

  if (group_is_local(&current->child_phase) &&
      if_rule_p(current->cto_instance->fibered_attr.attr)) {
    followed_by(current->cto_if_true, ph, ch, immediate, visit_marker_only,
                &if_true_any);
  }

  followed_by(current->cto_next, ph, ch, immediate, visit_marker_only,
              &if_false_any);

  *any |= (if_true_any || if_false_any);
}

/**
 * Utility function that returns boolean indicating whether there is any <ph,ch>
 * in augmented dependency graph
 * @param current CTO_NODE node
 * @param ph phase
 * @param ch child number
 * @return boolean indicating if there is any
 */
static bool aug_graph_contains_phase(AUG_GRAPH* aug_graph,
                                     TOTAL_ORDER_STATE* state,
                                     const short ph,
                                     const short ch) {
  int i;
  bool exist_parent_inherited_attr_previous_phase = false;
  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];
    CHILD_PHASE* group = &state->instance_groups[in->index];

    if (group->ph == ph && group->ch == ch) {
      return true;
    }
  }

  return false;
}

/**
 * Utility function used by assert_total_order to check sanity of total order
 *  1) After visit marker <ph,-1> there should be no attribute belonging to
 * <ph,-1> or <-ph,-1>
 *  2)  Immediately before visit marker <ph,ch> should be attribute
 * belonging to <-ph,ch>
 *  3) Or immediately after visit marker <ph,ch> should be attribute belonging
 * to <ph,ch>
 * @param current CTO_NODE node
 * @param prev_group <ph,ch> group
 */
static void total_order_sanity_check(AUG_GRAPH* aug_graph,
                                     CTO_NODE* current,
                                     CHILD_PHASE* prev_group,
                                     CHILD_PHASE* prev_parent,
                                     TOTAL_ORDER_STATE* state) {
  if (current == NULL)
    return;

  CHILD_PHASE* current_group = &current->child_phase;

  if (IS_VISIT_MARKER(current)) {
    if (current->child_phase.ch == -1) {
      // End of total order
      if (current->cto_next == NULL)
        return;

      // Boolean indicating whether end of phase visit marker was preceded
      // by parent synthesized attributes <ph,-1> if any
      bool preceded_by_parent_synthesized_current_phase =
          prev_parent->ph > 0 && prev_parent->ch == -1;

      if (aug_graph_contains_phase(aug_graph, state, current_group->ph, -1) &&
          !preceded_by_parent_synthesized_current_phase) {
        fatal_error(
            "[%s] Expected parent synthesized attribute <%+d,%+d> preced the "
            "end of phase visit marker <%+d,%+d>",
            aug_graph_name(aug_graph), current_group->ph, -1, current_group->ph,
            -1);
      }

      // Boolean indicating whether followed by inherited attribute of
      // parent <-(ph+1),-1>
      bool followed_by_parent_inherited_next_phase = false;
      followed_by(current->cto_next, -(current->child_phase.ph + 1), -1,
                  false /* not immediate */, false /* not visit markers only*/,
                  &followed_by_parent_inherited_next_phase);

      if (aug_graph_contains_phase(aug_graph, state, -(current_group->ph + 1),
                                   -1) &&
          !followed_by_parent_inherited_next_phase) {
        fatal_error(
            "[%s] Expected after end of phase visit marker <%+d,%+d> to be "
            "followed by parent inherited attribute of next phase <%+d,%+d>",
            aug_graph_name(aug_graph), current_group->ph, -1,
            current_group->ph + 1, -1);
      }
    } else {
      // Boolean indicating whether visit marker was followed by child
      // synthesized attribute(s) <ph,ch>
      bool followed_by_child_synthesized = false;
      followed_by(current->cto_next, current->child_phase.ph,
                  current->child_phase.ch, true /* immediate */,
                  false /* not visit markers only*/,
                  &followed_by_child_synthesized);

      // Boolean indicating whether visit marker was followed by child
      // inherited attribute(s) <-ph,ch>
      bool preceded_by_child_inherited = prev_group->ph == -current_group->ph &&
                                         prev_group->ch == current_group->ch;

      if (aug_graph_contains_phase(aug_graph, state, current->child_phase.ph,
                                   current->child_phase.ch) &&
          !followed_by_child_synthesized) {
        fatal_error(
            "[%s] After visit marker <%+d,%+d> the phase should be <ph,ch> "
            "(i.e. "
            "<%+d,%+d>).",
            aug_graph_name(aug_graph), current->child_phase.ph,
            current->child_phase.ch, current->child_phase.ph,
            current->child_phase.ch);
      } else if (aug_graph_contains_phase(aug_graph, state,
                                          -current->child_phase.ph,
                                          current->child_phase.ch) &&
                 !preceded_by_child_inherited) {
        fatal_error(
            "[%s] Before visit marker <%+d,%+d> the phase should be <-ph,ch> "
            "(i.e. <%+d,%+d>).",
            aug_graph_name(aug_graph), current->child_phase.ph,
            current->child_phase.ch, -current->child_phase.ph,
            current->child_phase.ch);
      }
    }
  }

  if (current_group->ch == -1) {
    prev_parent = current_group;
  }

  if (group_is_local(&current->child_phase)) {
    // Do not change the current group if instance is local
    current_group = prev_group;

    if (if_rule_p(current->cto_instance->fibered_attr.attr)) {
      total_order_sanity_check(aug_graph, current->cto_if_true,
                               &current->child_phase, prev_parent, state);
    }
  }

  total_order_sanity_check(aug_graph, current->cto_next, current_group,
                           prev_parent, state);
}

/**
 * Helper function to assert child visits are consecutive
 * @param current head of total order linked list
 * @param ph head of total order linked list
 * @param ch head of total order linked list
 */
static void child_visit_consecutive_check(CTO_NODE* current,
                                          short ph,
                                          short ch) {
  if (current == NULL)
    return;

  if (IS_VISIT_MARKER(current) && current->child_phase.ch == ch) {
    if (current->child_phase.ph != ph) {
      fatal_error(
          "Out of order child visits, expected visit(%d,%d) but "
          "received visit(%d,%d)",
          ph, ch, current->child_phase.ph, current->child_phase.ch);
    }

    // Done with this phase. Now check the next phase of this child
    child_visit_consecutive_check(current->cto_next, ph + 1, ch);
    return;
  }

  if (group_is_local(&current->child_phase) &&
      if_rule_p(current->cto_instance->fibered_attr.attr)) {
    child_visit_consecutive_check(current->cto_if_true, ph, ch);
  }

  child_visit_consecutive_check(current->cto_next, ph, ch);
}

/**
 * This function asserts that visits for a particular child are consecutive
 * @param aug_graph Augmented dependency graph
 * @param state state
 * @param head head of total order linked list
 */
static void child_visit_consecutive(AUG_GRAPH* aug_graph,
                                    TOTAL_ORDER_STATE* state,
                                    CTO_NODE* head) {
  int i;
  for (i = 0; i < state->children.length; i++) {
    child_visit_consecutive_check(head, 1, i);
  }
}

/**
 * This function asserts that there is a visit call for each phase of child
 * @param aug_graph Augmented dependency graph
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param head head of total order linked list
 */
static void child_visit_completeness(AUG_GRAPH* aug_graph,
                                     TOTAL_ORDER_STATE* state,
                                     CTO_NODE* head) {
  int i, j, k;
  for (i = 0; i < state->children.length; i++) {
    short max_phase = (short)state->max_child_ph[i];
    for (j = 1; j <= max_phase; j++) {
      bool any = false;
      followed_by(head, j, i, false /* not immediate */,
                  true /* visit markers only */, &any);

      if (!any) {
        // Ensure that there is a child any (synthesized of inherited)
        // attribute for this phase, if there is none then it is okay to
        // have missing child visit because it would be fruitless
        for (k = 0; k < state->children.length; k++) {
          CHILD_PHASE* group = &state->instance_groups[k];
          if (!group_is_local(group) && abs(group->ph) == j && group->ch == i) {
            fatal_error("Missing child %d visit call for phase %d", i, j);
            return;
          }
        }
      }
    }
  }
}

/**
 * This function asserts that no non-circular visit of a child is done in the
 * circular visit of the parent.
 * @param aug_graph Augmented dependency graph
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param cto_node current linked list node
 * @param parent_ph current parent visit phase
 */
static void check_circular_visit(AUG_GRAPH* aug_graph,
                                 TOTAL_ORDER_STATE* state,
                                 CTO_NODE* cto_node,
                                 short parent_ph) {
  if (cto_node == NULL)
    return;

  if (IS_VISIT_MARKER(cto_node)) {
    CHILD_PHASE group = cto_node->child_phase;
    if (group.ch == -1) {
      parent_ph += 1;
    } else if (!group_is_local(&group)) {
      PHY_GRAPH* parent_phy =
          Declaration_info(aug_graph->lhs_decl)->node_phy_graph;
      PHY_GRAPH* child_phy =
          Declaration_info(state->children.array[group.ch])->node_phy_graph;

      if (child_phy != NULL && parent_phy->cyclic_flags[parent_ph] &&
          !child_phy->cyclic_flags[group.ph]) {
        fatal_error(
            "We cannot do a non-circular visit of a child visit(%s) of "
            "<%+d,%+d> "
            "in the circular visit of the parent (phase: %d).",
            decl_name(state->children.array[group.ch]), group.ph, group.ch,
            parent_ph);
      }
    }
  }

  if (cto_node->cto_if_true != NULL) {
    check_circular_visit(aug_graph, state, cto_node->cto_if_true, parent_ph);
  }

  check_circular_visit(aug_graph, state, cto_node->cto_next, parent_ph);
}

/**
 * Function that throws an error if phases are out of order
 * @param aug_graph
 * @param head head of linked list
 * @param state current value of ph being investigated
 */
static void assert_total_order(AUG_GRAPH* aug_graph,
                               TOTAL_ORDER_STATE* state,
                               CTO_NODE* head) {
  // Condition #1: completeness of child visit calls
  child_visit_completeness(aug_graph, state, head);

  // Condition #2: general sanity of total order using visit markers
  total_order_sanity_check(aug_graph, head, &parent_inherited_group,
                           &parent_inherited_group, state);

  // Condition #3: consecutive of child visit calls
  child_visit_consecutive(aug_graph, state, head);

  // Condition #4: ensure no non-circular visit of a child in the circular visit
  // of the parent.
  check_circular_visit(aug_graph, state, head, 1);
}

/**
 * Utility function to check if group in augmented dependency graph is ready to
 * go
 * @param aug_graph Augmented dependency graph
 * @param component_index component index
 * @param cond current CONDITION
 * @param state
 * @param group group to check
 * @return head of linked list
 */
static bool any_instance_in_group_ready_to_go(AUG_GRAPH* aug_graph,
                                              CONDITION cond,
                                              TOTAL_ORDER_STATE* state,
                                              CHILD_PHASE* group) {
  int i;
  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];

    // Already scheduled, ignore
    if (state->schedule[in->index])
      continue;

    if (!child_phases_are_equal(&state->instance_groups[in->index], group))
      continue;

    if (group_ready_to_go(aug_graph, state, cond, in->index)) {
      return true;
    }
  }

  return false;
}

/**
 * Utility function to handle start of scheduling of groups
 * @param aug_graph Augmented dependency graph
 * @param comp SCC component
 * @param comp_index component index
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_transition_start_of_group(AUG_GRAPH* aug_graph,
                                                    SCC_COMPONENT* comp,
                                                    const int comp_index,
                                                    CTO_NODE* prev,
                                                    CONDITION cond,
                                                    TOTAL_ORDER_STATE* state,
                                                    int remaining,
                                                    CHILD_PHASE* group,
                                                    const short parent_ph,
                                                    const bool circular) {
  CTO_NODE* cto_node;

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting schedule_transition_start_of_group (%s) with "
        "(remaining: %d, group: "
        "<%+d,%+d>, parent_ph: %d, circular: %s)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph,
        circular ? "circular" : "non-circular");
  }

  if (group->ch != -1 && abs(group->ph) > 1 &&
      !state->child_visit_markers[group->ch][abs(group->ph) - 1]) {
    PHY_GRAPH* phy =
        Declaration_info(state->children.array[group->ch])->node_phy_graph;

    if (phy->empty_phase[abs(group->ph) - 1]) {
      cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
      cto_node->cto_prev = prev;
      cto_node->cto_instance = NULL;
      cto_node->child_phase.ph = abs(group->ph) - 1;
      cto_node->child_phase.ch = group->ch;
      cto_node->child_decl = state->children.array[group->ch];
      cto_node->visit = parent_ph;
      cto_node->component = comp_index;

      state->child_visit_markers[group->ch][abs(group->ph) - 1] = true;

      if (circular) {
        cto_node =
            group_schedule_circular(aug_graph, comp, comp_index, prev, cond,
                                    state, remaining, group, parent_ph);
      } else {
        cto_node = group_schedule_noncircular(aug_graph, prev, cond, state,
                                              remaining, group, parent_ph);
      }

      state->child_visit_markers[group->ch][abs(group->ph) - 1] = false;
      return cto_node;
    } else {
      CHILD_PHASE* child_inh = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
      child_inh->ph = -(abs(group->ph) - 1);
      child_inh->ch = group->ch;

      if (circular) {
        cto_node =
            group_schedule_circular(aug_graph, comp, comp_index, prev, cond,
                                    state, remaining, child_inh, parent_ph);
      } else {
        cto_node = group_schedule_noncircular(aug_graph, prev, cond, state,
                                              remaining, child_inh, parent_ph);
      }

      return cto_node;
    }
  }

  // If we are starting to schedule child synthesized attribute outside of
  // group scheduler it means child synthesized attribute did not immediately
  // follow by child inherited attribute thus add a visit marker <ph,ch>
  if (group->ph > 0 && group->ch != -1 &&
      !state->child_visit_markers[group->ch][group->ph]) {
    // Mark investigations as done
    if (!state->child_inh_investigations[group->ch][group->ph]) {
      CHILD_PHASE* child_inh = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
      child_inh->ph = -group->ph;
      child_inh->ch = group->ch;

      // Mark investigations as done
      state->child_inh_investigations[group->ch][group->ph] = true;
      if (circular) {
        cto_node =
            group_schedule_circular(aug_graph, comp, comp_index, prev, cond,
                                    state, remaining, child_inh, parent_ph);
      } else {
        cto_node = group_schedule_noncircular(aug_graph, prev, cond, state,
                                              remaining, child_inh, parent_ph);
      }

      // Release investigations
      state->child_inh_investigations[group->ch][group->ph] = false;

      return cto_node;
    }

    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = group->ph;
    cto_node->child_phase.ch = group->ch;
    cto_node->child_decl = state->children.array[group->ch];
    cto_node->visit = parent_ph;
    cto_node->component = comp_index;
    // Mark this child visit as done
    state->child_visit_markers[group->ch][group->ph] = true;
    if (circular) {
      cto_node->cto_next =
          group_schedule_circular(aug_graph, comp, comp_index, prev, cond,
                                  state, remaining, group, parent_ph);
    } else {
      cto_node->cto_next = group_schedule_noncircular(
          aug_graph, prev, cond, state, remaining, group, parent_ph);
    }
    // Release it
    state->child_visit_markers[group->ch][group->ph] = false;
    return cto_node;
  }

  // If parent phase is greater than current parent attribute phase then we
  // have reached the end of previous phase and so add a end of parent phase
  // visit marker <ph,-1>
  if (abs(group->ph) > parent_ph && group->ch == -1 &&
      !state->parent_visit_markers[parent_ph]) {
    state->parent_visit_markers[parent_ph] = true;
    cto_node = schedule_visit_end(aug_graph, comp, comp_index, prev, cond,
                                  state, remaining, parent_ph, circular);
    state->parent_visit_markers[parent_ph] = false;
    return cto_node;
  }

  if (circular) {
    return group_schedule_circular(aug_graph, comp, comp_index, prev, cond,
                                   state, remaining, group, parent_ph);
  } else {
    return group_schedule_noncircular(aug_graph, prev, cond, state, remaining,
                                      group, parent_ph);
  }
}

/**
 * Utility function to handle transitions between groups while scheduling cycles
 * @param aug_graph Augmented dependency graph
 * @param comp SCC component
 * @param comp_index component index
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_transition_end_of_group(AUG_GRAPH* aug_graph,
                                                  SCC_COMPONENT* comp,
                                                  const int comp_index,
                                                  CTO_NODE* prev,
                                                  CONDITION cond,
                                                  TOTAL_ORDER_STATE* state,
                                                  int remaining,
                                                  CHILD_PHASE* group,
                                                  const short parent_ph,
                                                  const bool circular) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting schedule_circular_transition_end_of_group (%s) with "
        "(remaining: %d, group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  CTO_NODE* cto_node;

  // If we find ourselves scheduling a <-ph,ch>, we need to (after putting in
  // all the instances in that group), we need to schedule the visit of the
  // child (add a CTO node with a null instance but with <ph,ch) and then ALSO
  // schedule immediately all the syn attributes of that child's phase.
  // (<+ph,ch> group, if any).
  if (group->ph < 0 && group->ch != -1 &&
      !state->child_visit_markers[group->ch][-group->ph]) {
    // Visit marker
    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = -group->ph;
    cto_node->child_phase.ch = group->ch;
    cto_node->child_decl = state->children.array[group->ch];
    cto_node->visit = parent_ph;
    cto_node->component = comp_index;
    // Mark this child visit as done
    state->child_visit_markers[group->ch][-group->ph] = true;

    if (circular) {
      cto_node->cto_next = group_schedule_circular(
          aug_graph, comp, comp_index, cto_node, cond, state,
          remaining /* no change */, &cto_node->child_phase, parent_ph);
    } else {
      cto_node->cto_next = group_schedule_noncircular(
          aug_graph, cto_node, cond, state, remaining /* no change */,
          &cto_node->child_phase, parent_ph);
    }

    state->child_visit_markers[group->ch][-group->ph] = false;  // Release it
    return cto_node;
  }

  if (group->ph > 0 && group->ch == -1 &&
      !state->parent_visit_markers[group->ph]) {
    state->parent_visit_markers[group->ph] = true;
    state->parent_synth_investigations[group->ph] = true;

    cto_node = schedule_visit_end(aug_graph, comp, comp_index, prev, cond,
                                  state, remaining, parent_ph, circular);

    state->parent_visit_markers[group->ph] = false;
    state->parent_synth_investigations[group->ph] = false;
    return cto_node;
  }

  // Fallback to normal scheduler
  if (circular) {
    return greedy_schedule_circular(aug_graph, comp, comp_index, prev, cond,
                                    state, remaining /* no change */, group,
                                    parent_ph);
  } else {
    return greedy_schedule_noncircular(aug_graph, prev, cond, state,
                                       remaining /* no change */, group,
                                       parent_ph);
  }
}

/**
 * Utility function to handle visit end marker
 * @param aug_graph Augmented dependency graph
 * @param comp SCC component
 * @param prev previous CTO node
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param parent_ph current parent phase being worked on
 * @return head of linked list
 */
static CTO_NODE* schedule_visit_end(AUG_GRAPH* aug_graph,
                                    SCC_COMPONENT* comp,
                                    int comp_index,
                                    CTO_NODE* prev,
                                    CONDITION cond,
                                    TOTAL_ORDER_STATE* state,
                                    const int remaining,
                                    const short parent_ph,
                                    const bool circular) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting schedule_visit_end (%s) with "
        "(remaining: %d parent_ph: %d, circular: %s)\n",
        aug_graph_name(aug_graph), remaining, parent_ph,
        circular ? "circular" : "non-circular");
  }

  PHY_GRAPH* parent_phy = Declaration_info(aug_graph->lhs_decl)->node_phy_graph;

  int ch, ph;
  for (ch = 0; ch < state->children.length; ch++) {
    PHY_GRAPH* child_phy =
        Declaration_info(state->children.array[ch])->node_phy_graph;

    if (child_phy == NULL)
      continue;

    for (ph = 1; ph <= state->max_child_ph[ch]; ph++) {
      if (!state->child_visit_markers[ch][ph] &&
          parent_phy->cyclic_flags[parent_ph] == child_phy->cyclic_flags[ph]) {
        // As soon as we encounter a child visit that is not empty and not
        // scheduler, stop. Otherwise, we would be going too far
        if (!state->child_visit_markers[ch][ph] && !child_phy->empty_phase[ph])
          break;

        CTO_NODE* cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
        cto_node->cto_prev = prev;
        cto_node->child_decl = state->children.array[ch];
        cto_node->cto_instance = NULL;
        cto_node->child_phase.ph = ph;
        cto_node->child_phase.ch = ch;
        cto_node->component = comp_index;
        cto_node->visit = parent_ph;
        state->child_visit_markers[ch][ph] = true;
        cto_node->cto_next = schedule_visit_end(
            aug_graph, comp, comp_index, cto_node, cond, state,
            remaining /* no change*/, parent_ph, circular);
        state->child_visit_markers[ch][ph] = false;

        return cto_node;
      }
    }
  }

  CTO_NODE* cto_node;

  if (state->parent_synth_investigations[parent_ph]) {
    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = parent_ph;
    cto_node->child_phase.ch = comp_index;
    cto_node->visit = parent_ph;
    cto_node->component = comp_index;

    CHILD_PHASE* parent_inh = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
    parent_inh->ch = -1;
    parent_inh->ph = -(parent_ph + 1);

    // Short circut
    if (parent_ph >= parent_phy->max_phase) {
      cto_node->cto_next = NULL;
    } else if (parent_phy->cyclic_flags[parent_ph + 1]) {
      find_scc_to_schedule(aug_graph, prev, cond, state, remaining, parent_inh,
                           parent_ph, &comp, &comp_index);

      cto_node->cto_next =
          group_schedule_circular(aug_graph, comp, comp_index, prev, cond,
                                  state, remaining, parent_inh, parent_ph + 1);
    } else {
      cto_node->cto_next = group_schedule_noncircular(
          aug_graph, prev, cond, state, remaining, parent_inh, parent_ph + 1);
    }

    return cto_node;
  } else {
    CHILD_PHASE* parent_inh = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
    parent_inh->ch = -1;
    parent_inh->ph = parent_ph;

    state->parent_synth_investigations[parent_ph] = true;

    // Short circut
    if (parent_phy->cyclic_flags[parent_ph]) {
      find_scc_to_schedule(aug_graph, prev, cond, state, remaining, parent_inh,
                           parent_ph, &comp, &comp_index);

      cto_node =
          group_schedule_circular(aug_graph, comp, comp_index, prev, cond,
                                  state, remaining, parent_inh, parent_ph);
    } else {
      cto_node = group_schedule_noncircular(aug_graph, prev, cond, state,
                                            remaining, parent_inh, parent_ph);
    }

    state->parent_synth_investigations[parent_ph] = false;

    return cto_node;
  }
}

/**
 * Greedy non-circular group scheduler
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group group currently getting scheduled
 * @param parent_ph current parent phase being worked on
 * @return head of linked list
 */
static CTO_NODE* group_schedule_noncircular(AUG_GRAPH* aug_graph,
                                            CTO_NODE* prev,
                                            CONDITION cond,
                                            TOTAL_ORDER_STATE* state,
                                            const int remaining,
                                            CHILD_PHASE* group,
                                            const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting group_schedule_noncircular (%s) with (remaining: %d, group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  int i, j;
  CTO_NODE* cto_node = prev;

  /* If nothing more to do, we are done. */
  if (remaining == 0) {
    return schedule_visit_end(aug_graph, NULL, -1, prev, cond, state, remaining,
                              parent_ph, false /* non-circular */);
  }

  /* Outer condition is impossible, it's a dead-end branch */
  if (CONDITION_IS_IMPOSSIBLE(cond))
    return NULL;

  for (i = 0; i < aug_graph->components.length; i++) {
    SCC_COMPONENT* comp = &aug_graph->components.array[i];

    // Ignore circular SCCs
    if (comp->length > 1)
      continue;

    // Already scheduled
    if (state->schedule[comp->array[0]])
      continue;

    for (j = 0; j < comp->length; j++) {
      INSTANCE* instance = &aug_graph->instances.array[comp->array[j]];
      CHILD_PHASE* instance_group = &state->instance_groups[instance->index];

      // Already scheduled then ignore
      if (state->schedule[instance->index])
        continue;

      // Check if everything is in the same group, do not check for dependencies
      // Locals will never end-up in this function
      if (child_phases_are_equal(instance_group, group)) {
        cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
        cto_node->cto_prev = prev;
        cto_node->cto_instance = instance;
        cto_node->child_phase.ph = group->ph;
        cto_node->child_phase.ch = group->ch;
        cto_node->visit = parent_ph;
        cto_node->component = -1;

        if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
          printf(
              "-> Scheduled circular via group non-circular scheduler "
              "(instance: ");
          print_instance(instance, stdout);
          printf(
              ", group: <%+d,%+d>, cond: (%+d,%+d), inst_cond: (%+d,%+d))\n\n",
              group->ph, group->ch, cond.positive, cond.negative,
              instance_condition(instance).positive,
              instance_condition(instance).negative);
        }

        // instance has been scheduled (and will not be
        // considered for scheduling in the recursive call)
        state->schedule[instance->index] = true;

        if (if_rule_p(instance->fibered_attr.attr)) {
          int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
          cond.negative |= cmask;
          cto_node->cto_if_false =
              group_schedule_noncircular(aug_graph, cto_node, cond, state,
                                         remaining - 1, group, parent_ph);
          cond.negative &= ~cmask;
          cond.positive |= cmask;
          cto_node->cto_if_true =
              group_schedule_noncircular(aug_graph, cto_node, cond, state,
                                         remaining - 1, group, parent_ph);
          cond.positive &= ~cmask;
        } else {
          cto_node->cto_next =
              group_schedule_noncircular(aug_graph, cto_node, cond, state,
                                         remaining - 1, group, parent_ph);
        }

        // Release it
        state->schedule[instance->index] = false;

        return cto_node;
      }
    }
  }

  // Group scheduling is finished
  if (!is_there_more_to_schedule_in_group(aug_graph, state, group)) {
    return schedule_transition_end_of_group(aug_graph, NULL, -1, cto_node, cond,
                                            state, remaining, group, parent_ph,
                                            false /* noncircular */);
  }

  // Try finding a scc to schedule
  if (remaining > 0) {
    return schedule_transition(aug_graph, NULL, -1, prev, cond, state,
                               remaining, group, parent_ph,
                               false /* non-circular */);
  }

  // All done with scheduling
  return NULL;
}

/**
 * Utility function that greedy schedules non-circular
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* greedy_schedule_noncircular(AUG_GRAPH* aug_graph,
                                             CTO_NODE* prev,
                                             CONDITION cond,
                                             TOTAL_ORDER_STATE* state,
                                             int remaining,
                                             CHILD_PHASE* prev_group,
                                             const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting greedy_schedule_noncircular (%s) with (remaining: %d, "
        "prev_group: "
        "<%d,%d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, prev_group->ph, prev_group->ch,
        parent_ph);
  }

  int i, j;

  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* instance = &aug_graph->instances.array[i];
    CHILD_PHASE* group = &state->instance_groups[instance->index];
    CTO_NODE* cto_node;

    // Already scheduled
    if (state->schedule[instance->index])
      continue;

    // check to see if makes sense (No need to schedule something that occurs
    // only in a different condition branch.)
    if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, instance_condition(instance))) {
      state->schedule[instance->index] = true;
      cto_node = greedy_schedule_noncircular(aug_graph, prev, cond, state,
                                             remaining - 1, group, parent_ph);
      state->schedule[instance->index] = false;
      return cto_node;
    }

    // If edgeset condition is not impossible then go ahead with scheduling
    if (group_ready_to_go(aug_graph, state, cond, instance->index)) {
      if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
        printf(
            "-> Scheduled scc via greedy non-circular scheduler (instance: ");
        print_instance(instance, stdout);
        printf(", group: <%+d,%+d>, cond: (%+d,%+d), inst_cond: (%+d,%+d))\n\n",
               group->ph, group->ch, cond.positive, cond.negative,
               instance_condition(instance).positive,
               instance_condition(instance).negative);
      }

      // If it is local then continue scheduling
      if (instance_is_local(aug_graph, state, instance->index)) {
        cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
        cto_node->cto_prev = prev;
        cto_node->cto_instance = instance;
        cto_node->child_phase.ch = group->ch;
        cto_node->child_phase.ph = group->ph;
        cto_node->visit = parent_ph;
        cto_node->component = -1;
        state->schedule[instance->index] =
            true;  // instance has been scheduled (and will not be
        // considered for scheduling in the recursive call)

        if (if_rule_p(instance->fibered_attr.attr)) {
          int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
          cond.negative |= cmask;
          cto_node->cto_if_false =
              greedy_schedule_noncircular(aug_graph, cto_node, cond, state,
                                          remaining - 1, group, parent_ph);
          cond.negative &= ~cmask;
          cond.positive |= cmask;
          cto_node->cto_if_true =
              greedy_schedule_noncircular(aug_graph, cto_node, cond, state,
                                          remaining - 1, group, parent_ph);
          cond.positive &= ~cmask;
        } else {
          cto_node->cto_next =
              greedy_schedule_noncircular(aug_graph, cto_node, cond, state,
                                          remaining - 1, group, parent_ph);
        }

        state->schedule[instance->index] = false;  // Release it

        return cto_node;
      } else {
        // Instance is not local then delegate it to circular group scheduler
        return schedule_transition_start_of_group(
            aug_graph, NULL, -1, prev, cond, state, remaining, group, parent_ph,
            false /* non-circular*/);
      }
    }
  }

  // Fallback to scc scheduler
  return schedule_transition(aug_graph, NULL, -1, prev, cond, state, remaining,
                             prev_group, parent_ph, false /* non-circular*/);
}

/**
 * Greedy circular group scheduler
 * @param aug_graph Augmented dependency graph
 * @param comp SCC component
 * @param comp_index component index
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group group currently getting scheduled
 * @return head of linked list
 */
static CTO_NODE* group_schedule_circular(AUG_GRAPH* aug_graph,
                                         SCC_COMPONENT* comp,
                                         const int comp_index,
                                         CTO_NODE* prev,
                                         CONDITION cond,
                                         TOTAL_ORDER_STATE* state,
                                         const int remaining,
                                         CHILD_PHASE* group,
                                         const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting group_schedule_circular (%s) with (remaining: %d, group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  int i;
  CTO_NODE* cto_node = prev;

  /* If nothing more to do, we are done. */
  if (remaining == 0) {
    return schedule_visit_end(aug_graph, NULL, -1, prev, cond, state, remaining,
                              parent_ph, true /* circular */);
  }

  /* Outer condition is impossible, it's a dead-end branch */
  if (CONDITION_IS_IMPOSSIBLE(cond))
    return NULL;

  for (i = 0; i < comp->length; i++) {
    INSTANCE* instance = &aug_graph->instances.array[comp->array[i]];
    CHILD_PHASE* instance_group = &state->instance_groups[instance->index];

    // Already scheduled then ignore
    if (state->schedule[instance->index])
      continue;

    // Check if everything is in the same group, do not check for dependencies
    // Locals will never end-up in this function
    if (child_phases_are_equal(instance_group, group)) {
      cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
      cto_node->cto_prev = prev;
      cto_node->cto_instance = instance;
      cto_node->child_phase.ph = group->ph;
      cto_node->child_phase.ch = group->ch;
      cto_node->visit = parent_ph;
      cto_node->component = comp_index;

      if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
        printf(
            "-> Scheduled circular via group circular scheduler (instance: ");
        print_instance(instance, stdout);
        printf(", group: <%+d,%+d>, cond: (%+d,%+d), inst_cond: (%+d,%+d))\n\n",
               group->ph, group->ch, cond.positive, cond.negative,
               instance_condition(instance).positive,
               instance_condition(instance).negative);
      }

      state->schedule[instance->index] =
          true;  // instance has been scheduled (and will not be
      // considered for scheduling in the recursive call)

      if (if_rule_p(instance->fibered_attr.attr)) {
        int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
        cond.negative |= cmask;
        cto_node->cto_if_false =
            group_schedule_circular(aug_graph, comp, comp_index, cto_node, cond,
                                    state, remaining - 1, group, parent_ph);
        cond.negative &= ~cmask;
        cond.positive |= cmask;
        cto_node->cto_if_true =
            group_schedule_circular(aug_graph, comp, comp_index, cto_node, cond,
                                    state, remaining - 1, group, parent_ph);
        cond.positive &= ~cmask;
      } else {
        cto_node->cto_next =
            group_schedule_circular(aug_graph, comp, comp_index, cto_node, cond,
                                    state, remaining - 1, group, parent_ph);
      }

      state->schedule[instance->index] = false;  // Release it

      return cto_node;
    }
  }

  // Group scheduling is finished
  if (!is_there_more_to_schedule_in_group(aug_graph, state, group)) {
    return schedule_transition_end_of_group(
        aug_graph, comp, comp_index, cto_node, cond, state, remaining, group,
        parent_ph, true /* circular */);
  }

  // Try finding a scc to schedule
  if (remaining > 0) {
    return schedule_transition(aug_graph, comp, comp_index, prev, cond, state,
                               remaining, group, parent_ph,
                               true /* circular */);
  }

  // All done with scheduling
  return NULL;
}

/**
 * Utility function that greedy schedules non-circular
 * @param aug_graph Augmented dependency graph
 * @param comp SCC component
 * @param comp_index component index
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* greedy_schedule_circular(AUG_GRAPH* aug_graph,
                                          SCC_COMPONENT* comp,
                                          const int comp_index,
                                          CTO_NODE* prev,
                                          CONDITION cond,
                                          TOTAL_ORDER_STATE* state,
                                          const int remaining,
                                          CHILD_PHASE* prev_group,
                                          const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting greedy_schedule_circular (%s) with (remaining: %d, "
        "prev_group: "
        "<%d,%d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, prev_group->ph, prev_group->ch,
        parent_ph);
  }

  int i;

  for (i = 0; i < comp->length; i++) {
    INSTANCE* instance = &aug_graph->instances.array[comp->array[i]];
    CHILD_PHASE* group = &state->instance_groups[instance->index];
    CTO_NODE* cto_node;

    // Already scheduled
    if (state->schedule[instance->index])
      continue;

    // check to see if makes sense (No need to schedule something that occurs
    // only in a different condition branch.)
    if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, instance_condition(instance))) {
      state->schedule[instance->index] = true;
      cto_node = greedy_schedule_noncircular(aug_graph, prev, cond, state,
                                             remaining - 1, group, parent_ph);
      state->schedule[instance->index] = false;
      return cto_node;
    }

    // If edgeset condition is not impossible then go ahead with scheduling
    if (group_ready_to_go(aug_graph, state, cond, instance->index)) {
      if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
        printf("-> Scheduled scc via greedy circular scheduler (instance: ");
        print_instance(instance, stdout);
        printf(", group: <%+d,%+d>, cond: (%+d,%+d), inst_cond: (%+d,%+d))\n\n",
               group->ph, group->ch, cond.positive, cond.negative,
               instance_condition(instance).positive,
               instance_condition(instance).negative);
      }

      // If it is local then continue scheduling
      if (instance_is_local(aug_graph, state, instance->index)) {
        cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
        cto_node->cto_prev = prev;
        cto_node->cto_instance = instance;
        cto_node->child_phase.ch = group->ch;
        cto_node->child_phase.ph = group->ph;
        cto_node->visit = parent_ph;
        cto_node->component = comp_index;
        // instance has been scheduled (and will not be
        // considered for scheduling in the recursive call)
        state->schedule[instance->index] = true;

        if (if_rule_p(instance->fibered_attr.attr)) {
          int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
          cond.negative |= cmask;
          cto_node->cto_if_false = greedy_schedule_circular(
              aug_graph, comp, comp_index, cto_node, cond, state, remaining - 1,
              group, parent_ph);
          cond.negative &= ~cmask;
          cond.positive |= cmask;
          cto_node->cto_if_true = greedy_schedule_circular(
              aug_graph, comp, comp_index, cto_node, cond, state, remaining - 1,
              group, parent_ph);
          cond.positive &= ~cmask;
        } else {
          cto_node->cto_next = greedy_schedule_circular(
              aug_graph, comp, comp_index, cto_node, cond, state, remaining - 1,
              group, parent_ph);
        }

        state->schedule[instance->index] = false;  // Release it

        return cto_node;
      } else {
        // Instance is not local then delegate it to circular group scheduler
        return schedule_transition_start_of_group(
            aug_graph, comp, comp_index, prev, cond, state, remaining, group,
            parent_ph, true /* circular */);
      }
    }
  }

  // If there is any instance in the SCC that has not been scheduled
  if (is_there_more_to_schedule_in_scc(aug_graph, comp, state)) {
    fatal_error("Cannot schedule any further");
  }

  // Fallback to scc scheduler
  return schedule_transition(aug_graph, comp, comp_index, prev, cond, state,
                             remaining, prev_group, parent_ph,
                             true /* circular */);
}

struct sccs_info {
  SCC_COMPONENT** components;
  int* components_index;
  int count;
};

typedef struct sccs_info SccsInfo;

/**
 * Utility function that find a SCC component where all instances can be
 * scheduled
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static SccsInfo* find_all_sccs_to_schedule(AUG_GRAPH* aug_graph,
                                           CTO_NODE* prev,
                                           CONDITION cond,
                                           TOTAL_ORDER_STATE* state,
                                           const int remaining,
                                           const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting find_all_sccs_to_schedule (%s) with (remaining: %d, "
        "group: "
        ", parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, parent_ph);
  }

  int i, j, k;
  int n = aug_graph->instances.length;
  PHY_GRAPH* phy_parent = Declaration_info(aug_graph->lhs_decl)->node_phy_graph;
  int count_sccs = 0;
  SCC_COMPONENT** components = (SCC_COMPONENT**)malloc(
      aug_graph->components.length * sizeof(SCC_COMPONENT*));
  int* components_index =
      (int*)malloc(aug_graph->components.length * sizeof(int));

  for (i = 0; i < aug_graph->components.length; i++) {
    SCC_COMPONENT* comp = &aug_graph->components.array[i];

    // If any of instances in this cycle are scheduled then this component
    // is already scheduled
    if (state->schedule[comp->array[0]])
      continue;

    // Not a circular SCC
    if (comp->length == 1)
      continue;

    if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
      printf("\nInvestigating readiness of SCC #%d:\n", i);
      for (j = 0; j < comp->length; j++) {
        INSTANCE* in = &aug_graph->instances.array[comp->array[j]];
        CHILD_PHASE* instance_group = &state->instance_groups[in->index];
        print_instance(in, stdout);
        printf(" <%+d,%+d>\n", instance_group->ph, instance_group->ch);
      }

      printf("\n");
    }

    size_t temp_schedule_size = n * sizeof(bool);
    bool* temp_schedule = (bool*)alloca(temp_schedule_size);
    memcpy(temp_schedule, state->schedule, temp_schedule_size);

    // Temporarily mark all attributes in this scheduled
    for (j = 0; j < comp->length; j++) {
      INSTANCE* in = &aug_graph->instances.array[comp->array[j]];
      temp_schedule[in->index] = 1;
    }

    bool scc_ready = true;

    for (j = 0; j < comp->length; j++) {
      INSTANCE* in = &aug_graph->instances.array[comp->array[j]];
      CHILD_PHASE* instance_group = &state->instance_groups[in->index];

      for (k = 0; k < n; k++) {
        if (temp_schedule[k])
          continue;

        int index = k * n + in->index;

        EDGESET edges;

        // Look at all dependencies from instances in the component to
        // instances that are not scheduled
        for (edges = aug_graph->graph[index]; edges != NULL;
             edges = edges->rest) {
          // If the merge condition is impossible, ignore this edge
          if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, edges->cond))
            continue;

          if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
            // Can't continue with scheduling if a dependency with a
            // "possible" condition has not been scheduled yet
            printf("SCC (%d) was not ready to be scheduled because of:\n", i);

            print_instance(&aug_graph->instances.array[k], stdout);
            printf(" <%+d,+%d> -> ", state->instance_groups[k].ph,
                   state->instance_groups[k].ch);
            print_instance(&aug_graph->instances.array[in->index], stdout);
            printf(" <%+d,%+d> (%d)\n", state->instance_groups[in->index].ph,
                   state->instance_groups[in->index].ch,
                   get_edgeset_combine_dependencies(edges));
          }

          scc_ready = false;
        }
      }
    }

    // This component is ready to be scheduled as a group
    if (scc_ready) {
      for (j = 0; j < comp->length; j++) {
        INSTANCE* in = &aug_graph->instances.array[comp->array[j]];
        CHILD_PHASE* instance_group = &state->instance_groups[in->index];

        if (instance_group->ch != -1 && !group_is_local(instance_group)) {
          PHY_GRAPH* child_phy =
              Declaration_info(state->children.array[instance_group->ch])
                  ->node_phy_graph;

          if (child_phy != NULL && phy_parent->cyclic_flags[parent_ph] &&
              !child_phy->cyclic_flags[abs(instance_group->ph)]) {
            // Scheduling this will result in scheduling children circular
            // attribute in non-circular parent phase

            if ((oag_debug & DEBUG_ORDER) &&
                (oag_debug & DEBUG_ORDER_VERBOSE)) {
              char instance_to_str[BUFFER_SIZE];
              FILE* f = fmemopen(instance_to_str, sizeof(instance_to_str), "w");
              print_instance(in, f);
              fclose(f);

              printf(
                  "[%s] Cannot schedule SCC containing noncircular %s "
                  "<%+d,%+d> in circular parent phase %d\n",
                  aug_graph_name(aug_graph), instance_to_str,
                  instance_group->ph, instance_group->ch, parent_ph);
            }

            scc_ready = false;
          }
        }
      }

      if (!scc_ready)
        continue;

      if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
        printf("\nFound a SCC #%d ready to be scheduled:\n", i);
        for (j = 0; j < comp->length; j++) {
          INSTANCE* in = &aug_graph->instances.array[comp->array[j]];
          CHILD_PHASE* instance_group = &state->instance_groups[in->index];
          print_instance(in, stdout);
          printf(" <%+d,%+d>\n", instance_group->ph, instance_group->ch);
        }
        printf("\n");

        for (j = 0; j < comp->length; j++) {
          for (k = 0; k < comp->length; k++) {
            INSTANCE* in1 = &aug_graph->instances.array[comp->array[j]];
            INSTANCE* in2 = &aug_graph->instances.array[comp->array[k]];

            EDGESET es = aug_graph->graph[in1->index * n + in2->index];
            print_edgeset(es, stdout);
          }
        }

        printf("\n");
      }

      components[count_sccs] = comp;
      components_index[count_sccs] = i;
      count_sccs++;
    }
  }

  SccsInfo* result = (SccsInfo*)malloc(sizeof(SccsInfo));
  result->components = components;
  result->components_index = components_index;
  result->count = count_sccs;

  return result;
}

static void find_scc_to_schedule(AUG_GRAPH* aug_graph,
                                 CTO_NODE* prev,
                                 CONDITION cond,
                                 TOTAL_ORDER_STATE* state,
                                 const int remaining,
                                 CHILD_PHASE* group,
                                 const short parent_ph,
                                 SCC_COMPONENT** comp,
                                 int* comp_index) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting schedule_transition (%s) with (remaining: %d, "
        "group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  // No more SCC to schedule
  if (remaining == 0) {
    return;
  }

  int i;
  SccsInfo* components = find_all_sccs_to_schedule(aug_graph, prev, cond, state,
                                                   remaining, parent_ph);

  if (components->count == 0) {
    fatal_error("Could not find any SCC to schedule");
  }

  *comp = components->components[0];
  *comp_index = components->components_index[0];
  CHILD_PHASE* group_to_schedule = group;

  for (i = 0; i < components->count; i++) {
    if (is_there_more_to_schedule_in_scc_group(
            aug_graph, components->components[i], state, group)) {
      *comp = components->components[i];
      *comp_index = components->components_index[i];
      break;
    }
  }

  free(components->components);
  free(components->components_index);
  free(components);
}

/**
 * Utility function that find a SCC component where all instances can be
 * scheduled
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_transition(AUG_GRAPH* aug_graph,
                                     SCC_COMPONENT* prev_comp,
                                     const int prev_comp_index,
                                     CTO_NODE* prev,
                                     CONDITION cond,
                                     TOTAL_ORDER_STATE* state,
                                     int remaining,
                                     CHILD_PHASE* group,
                                     const short parent_ph,
                                     const bool circular) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting schedule_transition (%s) with (remaining: %d, "
        "group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  // No more SCC to schedule
  if (remaining == 0) {
    return NULL;
  }


  int i;
  SccsInfo* components = find_all_sccs_to_schedule(aug_graph, prev, cond, state,
                                                   remaining, parent_ph);

  if (components->count == 0) {
    if (is_there_more_to_schedule_in_group(aug_graph, state, group)) {
      return group_schedule_noncircular(aug_graph, prev, cond, state, remaining,
                                        group, parent_ph);
    } else {
      return greedy_schedule_noncircular(aug_graph, prev, cond, state,
                                         remaining, group, parent_ph);
    }
  }

  SCC_COMPONENT* comp;
  int comp_index;
  find_scc_to_schedule(aug_graph, prev, cond, state, remaining, group,
                       parent_ph, &comp, &comp_index);

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf("\nScheduling SCC #%d starting with group <%+d,%+d>\n", comp_index,
           group->ph, group->ch);
  }

  CTO_NODE* result =
      group_schedule_circular(aug_graph, comp, comp_index, prev, cond, state,
                              remaining, group, parent_ph);

  return result;
}

/**
 * Utility function to get children of augmented dependency graph as array
 * of declarations
 * @param aug_graph Augmented dependency graph
 * @param state State
 */
static void set_aug_graph_children(AUG_GRAPH* aug_graph,
                                   TOTAL_ORDER_STATE* state) {
  Declaration* children_arr = NULL;
  int children_count = 0;

  Declaration current;
  for (current = aug_graph->first_rhs_decl; current != NULL;
       current = DECL_NEXT(current)) {
    children_count++;
  }

  int i = 0;
  children_arr = (Declaration*)HALLOC(sizeof(Declaration) * children_count);
  for (current = aug_graph->first_rhs_decl; current != NULL;
       current = DECL_NEXT(current)) {
    children_arr[i++] = current;
  }

  // Assign children vector array
  state->children.array = children_arr;
  state->children.length = children_count;
}

/**
 * Utility function to schedule augmented dependency graph
 * @param aug_graph Augmented dependency graph
 */
static void schedule_augmented_dependency_graph(AUG_GRAPH* aug_graph) {
  int n = aug_graph->instances.length;
  CONDITION cond;
  int i, j, ch;

  (void)close_augmented_dependency_graph(aug_graph);

  // Now schedule graph: we need to generate a conditional total order.
  if (oag_debug & PROD_ORDER) {
    printf("Scheduling conditional total order for %s\n",
           aug_graph_name(aug_graph));
  }
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    for (i = 0; i < n; ++i) {
      INSTANCE* in = &(aug_graph->instances.array[i]);
      print_instance(in, stdout);
      printf(": ");
      Declaration ad = in->fibered_attr.attr;
      Declaration chdecl;

      j = 0, ch = -1;
      for (chdecl = aug_graph->first_rhs_decl; chdecl != 0;
           chdecl = DECL_NEXT(chdecl)) {
        if (in->node == chdecl)
          ch = j;
        ++j;
      }
      if (in->node == aug_graph->lhs_decl || ch >= 0) {
        PHY_GRAPH* npg = Declaration_info(in->node)->node_phy_graph;
        int ph = attribute_schedule(npg, &(in->fibered_attr));
        printf("<%d,%d>\n", ph, ch);
      } else {
        printf("local\n");
      }
    }
  }

  TOTAL_ORDER_STATE* state =
      (TOTAL_ORDER_STATE*)alloca(sizeof(TOTAL_ORDER_STATE));

  size_t instance_groups_size = n * sizeof(CHILD_PHASE);
  CHILD_PHASE* instance_groups = (CHILD_PHASE*)alloca(instance_groups_size);
  memset(instance_groups, (int)0, instance_groups_size);

  // Assign <ph,ch> to each attribute instance
  for (i = 0; i < n; i++) {
    INSTANCE* in = &(aug_graph->instances.array[i]);
    Declaration ad = in->fibered_attr.attr;
    Declaration chdecl;

    int j = 0, ch = -1;
    for (chdecl = aug_graph->first_rhs_decl; chdecl != 0;
         chdecl = DECL_NEXT(chdecl)) {
      if (in->node == chdecl)
        ch = j;
      ++j;
    }

    if (in->node == aug_graph->lhs_decl || ch >= 0) {
      PHY_GRAPH* npg = Declaration_info(in->node)->node_phy_graph;
      int ph = attribute_schedule(npg, &(in->fibered_attr));
      instance_groups[i].ph = (short)ph;
      instance_groups[i].ch = (short)ch;
    }
  }

  state->instance_groups = instance_groups;

  // Find children of augmented graph: this will be used as argument to
  // visit calls
  set_aug_graph_children(aug_graph, state);

  size_t schedule_size = n * sizeof(bool);
  bool* schedule = (bool*)alloca(schedule_size);

  // False here means nothing is scheduled yet
  memset(schedule, false, schedule_size);
  state->schedule = schedule;

  // Set default max phase of parent
  state->max_parent_ph =
      Declaration_info(aug_graph->lhs_decl)->node_phy_graph->max_phase;

  size_t max_child_ph_size = state->children.length * sizeof(short);
  short* max_child_ph = (short*)alloca(max_child_ph_size);

  // Set default max phase of children indexed by child index
  memset(max_child_ph, (int)0, max_child_ph_size);
  state->max_child_ph = max_child_ph;

  // Collect max_parent_ph and max_child_ph
  for (i = 0; i < state->children.length; i++) {
    Declaration child = state->children.array[i];
    PHY_GRAPH* phy = Declaration_info(child)->node_phy_graph;
    state->max_child_ph[i] =
        phy == NULL ? 0 : Declaration_info(child)->node_phy_graph->max_phase;
  }

  state->child_visit_markers =
      (bool**)alloca(state->children.length * sizeof(bool*));

  state->child_inh_investigations =
      (bool**)alloca(state->children.length * sizeof(bool*));
  for (i = 0; i < state->children.length; i++) {
    size_t child_visit_markers_size = state->max_child_ph[i] * sizeof(bool);

    state->child_visit_markers[i] = (bool*)alloca(child_visit_markers_size);
    memset(state->child_visit_markers[i], false, child_visit_markers_size);

    state->child_inh_investigations[i] =
        (bool*)alloca(child_visit_markers_size);
    memset(state->child_inh_investigations[i], false, child_visit_markers_size);
  }

  state->parent_synth_investigations =
      (bool*)alloca(state->max_parent_ph * sizeof(bool));
  memset(state->parent_synth_investigations, false,
         state->max_parent_ph * sizeof(bool));

  state->parent_visit_markers =
      (bool*)alloca(state->max_parent_ph * sizeof(bool));
  memset(state->parent_visit_markers, false,
         state->max_parent_ph * sizeof(bool));

  if (oag_debug & DEBUG_ORDER) {
    printf("\nInstances %s:\n", aug_graph_name(aug_graph));
    for (i = 0; i < n; i++) {
      INSTANCE* in = &(aug_graph->instances.array[i]);
      CHILD_PHASE group = state->instance_groups[i];
      print_instance(in, stdout);
      printf(": ");

      if (!group.ph && !group.ch) {
        printf("local\n");
      } else {
        printf("<%+d,%+d>\n", group.ph, group.ch);
      }
    }
  }

  cond.negative = 0;
  cond.positive = 0;

  printf("\ntenative order for %s\n", aug_graph_name(aug_graph));

  for (i = 0; i < aug_graph->scc_order.length; i++) {
    int comp_index = aug_graph->scc_order.array[i];
    SCC_COMPONENT comp = aug_graph->components.array[comp_index];
    for (j = 0; j < comp.length; j++) {
      INSTANCE* in = &aug_graph->instances.array[comp.array[j]];
      print_instance(in, stdout);
      printf(" <%+d,%+d>\n", state->instance_groups[in->index].ph,
             state->instance_groups[in->index].ch);
    }
  }

  printf("\n");

  // It is safe to assume inherited attribute of parents have no
  // dependencies and should be scheduled right away
  aug_graph->total_order = group_schedule_noncircular(
      aug_graph, NULL, cond, state, n, &parent_inherited_group, 1);

  // if (aug_graph->total_order == NULL)
  // { fatal_error("Failed to create total order."); }

  if (oag_debug & DEBUG_ORDER) {
    printf("\nSchedule for %s (%d children):\n", aug_graph_name(aug_graph),
           state->children.length);
    print_total_order(aug_graph->total_order, -1, 0, stdout);
  }

  // Ensure generated total order is valid
  // assert_total_order(aug_graph, state, aug_graph->total_order);
}

/**
 * @brief Computes total-preorder of set of attributes
 * @param module APS module
 * @param s state
 */
void compute_static_schedule(STATE* s) {
  state_scc(s);

  int j;
  for (j = 0; j < s->phyla.length; ++j) {
    schedule_summary_dependency_graph(&s->phy_graphs[j]);

    /* now perform closure */
    int saved_analysis_debug = analysis_debug;
    int j;

    if (oag_debug & TYPE_3_DEBUG) {
      analysis_debug |= TWO_EDGE_CYCLE;
    }

    if (analysis_debug & DNC_ITERATE) {
      printf("\n**** After OAG schedule for phylum %d:\n\n", j);
    }

    if (analysis_debug & ASSERT_CLOSED) {
      for (j = 0; j < s->match_rules.length; ++j) {
        printf("Checking rule %d\n", j);
        assert_closed(&s->aug_graphs[j]);
      }
    }

    dnc_close(s);

    analysis_debug = saved_analysis_debug;

    if (analysis_debug & DNC_ITERATE) {
      printf("\n*** After closure after schedule OAG phylum %d\n\n", j);
      print_analysis_state(s, stdout);
      print_cycles(s, stdout);
    }
  }

  for (j = 0; j < s->match_rules.length; ++j) {
    schedule_augmented_dependency_graph(&s->aug_graphs[j]);
  }
  schedule_augmented_dependency_graph(&s->global_dependencies);

  if (analysis_debug & (DNC_ITERATE | DNC_FINAL)) {
    printf("*** FINAL OAG ANALYSIS STATE ***\n");
    print_analysis_state(s, stdout);
    print_cycles(s, stdout);
  }
}
