#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aps-ag.h"
#include "jbb-alloc.h"
#include "jbb.h"

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
  // One-d array, boolean indicating whether there is any parent synthesized
  // attribute at phase indexed by phase number
  bool* any_parent_synth;
  // One-d array, boolean indicating whether there is any parent inherited
  // attribute at phase indexed by phase number
  bool* any_parent_inh;
};

typedef struct total_order_state TOTAL_ORDER_STATE;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define IS_VISIT_MARKER(node) (node->cto_instance == NULL)

// Greedy scheduler
static CTO_NODE* schedule_visits(AUG_GRAPH* aug_graph,
                                 CTO_NODE* prev,
                                 CONDITION cond,
                                 TOTAL_ORDER_STATE* state,
                                 int remaining,
                                 CHILD_PHASE* group,
                                 int prev_i,
                                 short parent_ph);

// Group scheduler
static CTO_NODE* schedule_visits_group(AUG_GRAPH* aug_graph,
                                       CTO_NODE* prev,
                                       CONDITION cond,
                                       TOTAL_ORDER_STATE* state,
                                       int remaining,
                                       CHILD_PHASE* group,
                                       int prev_i,
                                       short parent_ph);

/**
 * Utility function that checks whether instance belongs to any phylum cycle or
 * not
 * @param phy_graph phylum graph
 * @param in attribute instance
 * @return -1 if instance does not belong to any cycle or index of phylum cycle
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
 * Utility function that checks whether instance belongs to any aug graph cycle
 * or not
 * @param aug_graph augmented graph
 * @param in attribute instance
 * @return -1 if instance does not belong to any cycle or index of aug graph
 * cycle
 */
static int instance_in_aug_cycle(AUG_GRAPH* aug_graph, INSTANCE* in) {
  int i, j;
  for (i = 0; i < aug_graph->cycles.length; i++) {
    CYCLE* cyc = &aug_graph->cycles.array[i];
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
  int done = 0;
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
  int phase = 0;
  int done = 0;
  BOOL cont = true;

  if (oag_debug & TOTAL_ORDER) {
    printf("Scheduling order for %s\n", decl_name(phy_graph->phylum));
  }

  // Nothing is scheduled
  memset(phy_graph->summary_schedule, 0, n * sizeof(int));

  int i, j;
  size_t phase_size = (n + 1) * sizeof(BOOL);

  BOOL* circular_phase = (BOOL*)HALLOC(phase_size);
  memset(circular_phase, false, phase_size);

  BOOL* empty_phase = (BOOL*)HALLOC(phase_size);
  memset(empty_phase, false, phase_size);

  // Hold on to the flag indicating whether phase is circular or not
  phy_graph->cyclic_flags = circular_phase;
  phy_graph->empty_phase = empty_phase;

  int count_non_circular = 0, count_circular = 0;
  int cycle_happened = false;
  do {
    phase++;

    // Schedule non-circular attributes in this phase
    count_non_circular = schedule_phase_non_circular(phy_graph, phase);
    if (count_non_circular) {
      done += count_non_circular;
      circular_phase[phase] = false;

      if (oag_debug & TOTAL_ORDER) {
        printf("^^^ non-circular\n");
      }
      continue;
    } else if (cycle_happened) {
      // Add an empty phase between circular and non-circular
      circular_phase[phase] = false;
      // Mark this phase as empty
      empty_phase[phase] = true;
      phase++;
    }

    // Schedule circular attributes in this phase
    count_circular = schedule_phase_circular(phy_graph, phase);
    if (count_circular) {
      done += count_circular;
      circular_phase[phase] = true;
      cycle_happened = true;

      if (oag_debug & TOTAL_ORDER) {
        printf("^^^ circular\n");
      }
      continue;
    } else {
      cycle_happened = false;
    }
  } while (count_non_circular || count_circular);

  // This is used by code generation
  phy_graph->max_phase = phase - 1;

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
static void print_total_order(CTO_NODE* cto, int indent, FILE* stream) {
  if (cto == NULL)
    return;
  bool extra_newline = false;
  bool print_group = true;

  print_indent(indent, stream);
  if (cto->cto_instance == NULL) {
    fprintf(stream, "visit marker");
    if (cto->child_decl != NULL) {
      fprintf(stream, " (%s) ", decl_name(cto->child_decl));
    } else {
      extra_newline = true;
    }
  } else {
    print_instance(cto->cto_instance, stream);
    CONDITION cond = instance_condition(cto->cto_instance);
    fprintf(stream, " (%d,%d) ", cond.positive, cond.negative);

    if (if_rule_p(cto->cto_instance->fibered_attr.attr)) {
      print_group = false;
    }
  }

  if (print_group) {
    fprintf(stream, " <%d,%d>", cto->child_phase.ph, cto->child_phase.ch);
  }

  fprintf(stream, "\n");
  if (extra_newline) {
    fprintf(stream, "\n");
  }

  if (cto->cto_if_true != NULL) {
    print_indent(indent + 1, stream);
    fprintf(stream, "(true)\n");
    print_total_order(cto->cto_if_true, indent + 2, stdout);
    print_indent(indent + 1, stream);
    fprintf(stream, "(false)\n");
    indent += 2;
  }

  print_total_order(cto->cto_next, indent, stdout);
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
  fprintf(stderr, "Instances (%s):\n", decl_name(aug_graph->syntax_decl));

  int i, j, k;
  int n = aug_graph->instances.length;
  for (i = 0; i < n; i++) {
    print_instance(&aug_graph->instances.array[i], stream);
    fprintf(stream, " <%d, %d> (%s)\n", state->instance_groups[i].ph,
            state->instance_groups[i].ch,
            state->schedule[i] ? "scheduled" : "not-scheduled");
  }

  fprintf(stderr, "\nNot scheduled instances (%s):\n",
          decl_name(aug_graph->syntax_decl));

  size_t printed_size = sizeof(bool) * n;
  bool* printed = (bool*)alloca(printed_size);
  memset(printed, false, printed_size);
  for (i = 0; i < n; i++) {
    if (!state->schedule[i] && !printed[i]) {
      int count = 0;
      for (j = 0; j < n; j++) {
        if (!printed[j] && instances_in_same_group(aug_graph, state, i, j)) {
          INSTANCE instance = aug_graph->instances.array[j];
          CHILD_PHASE group = state->instance_groups[j];
          bool ready = instance_ready_to_go(aug_graph, state, cond, i, j);

          print_instance(&instance, stream);
          fprintf(stream, " <%d, %d> (%s) ", group.ph, group.ch,
                  ready ? "ready" : "not ready");

          if (!ready) {
            bool stop = false;
            for (k = 0; k < n && !stop; k++) {
              // Already scheduled then ignore
              if (state->schedule[k] ||
                  instances_in_same_group(aug_graph, state, i, k)) {
                continue;
              }

              EDGESET edges;

              /* Look at all dependencies from j to i */
              for (edges = aug_graph->graph[k * n + j]; edges != NULL && !stop;
                   edges = edges->rest) {
                /* If the merge condition is impossible, ignore
                 * this edge */
                if (!MERGED_CONDITION_IS_IMPOSSIBLE(cond, edges->cond)) {
                  CHILD_PHASE source =
                      state->instance_groups[edges->source->index];
                  CHILD_PHASE sink = state->instance_groups[edges->sink->index];

                  print_instance(edges->source, stream);
                  fprintf(stream, " (%d,%d) -> ", source.ph, source.ch);
                  print_instance(edges->sink, stream);
                  fprintf(stream, " (%d,%d)\n", sink.ph, sink.ch);

                  stop = true;
                }
              }
            }
          } else {
            printf("\n");
          }

          printed[j] = true;
          count++;
        }
      }

      if (count > 1) {
        fprintf(stream, "\n");
      }
    }
  }

  fprintf(stream, "Schedule so far:\n");
  // For debugging purposes, traverse all the way back
  while (prev != NULL && prev->cto_prev != NULL) {
    prev = prev->cto_prev;
  }

  print_total_order(prev, 0, stream);
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
 * Given a generic instance index it returns boolean indicating if its ready to
 * be scheduled or not
 * @param aug_graph Augmented dependency graph
 * @param cond current condition
 * @param state state
 * @param i group index
 * @param j instance index to test
 * @return boolean indicating whether instance is ready to be scheduled
 */
static bool noncircular_instance_ready_to_go(AUG_GRAPH* aug_graph,
                                             TOTAL_ORDER_STATE* state,
                                             const CONDITION cond,
                                             const int i,
                                             const int j) {
  int k;
  EDGESET edges;
  int n = aug_graph->instances.length;

  for (k = 0; k < n; k++) {
    // Already scheduled then ignore
    if (state->schedule[k])
      continue;

    // If from the same group then ignore
    if (instances_in_same_group(aug_graph, state, i, k))
      continue;

    int index = k * n + j;  // k (source) >--> j (sink) edge

    /* Look at all dependencies from j to i */
    for (edges = aug_graph->graph[index]; edges != NULL; edges = edges->rest) {
      /* If the merge condition is impossible, ignore this edge */
      if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, edges->cond))
        continue;

      if (oag_debug & DEBUG_ORDER) {
        // Can not continue with scheduling if a dependency with a
        // "possible" condition has not been scheduled yet
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
 * Given a generic instance index it returns boolean indicating if its ready to
 * be scheduled or not
 * @param aug_graph Augmented dependency graph
 * @param cond current condition
 * @param state state
 * @param i instance index to test
 * @return boolean indicating whether group is ready to be scheduled
 */
static bool noncircular_group_ready_to_go(AUG_GRAPH* aug_graph,
                                          TOTAL_ORDER_STATE* state,
                                          const CONDITION cond,
                                          const int i) {
  if (oag_debug & DEBUG_ORDER) {
    printf("Checking group readyness of: ");
    print_instance(&aug_graph->instances.array[i], stdout);
    printf("\n");
  }

  INSTANCE in = aug_graph->instances.array[i];
  int n = aug_graph->instances.length;
  CHILD_PHASE group = state->instance_groups[i];
  int j, k;

  for (j = 0; j < n; j++) {
    CHILD_PHASE current_group = state->instance_groups[j];

    // Instance in the same group but cannot be considered
    if (instances_in_same_group(aug_graph, state, i, j)) {
      if (state->schedule[j])
        continue;  // already scheduled

      if (!noncircular_instance_ready_to_go(aug_graph, state, cond, i, j)) {
        return false;
      }
    }
  }

  return true;
}

/**
 * Simple function to check if there is more to schedule in the group of index
 * @param aug_graph Augmented dependency graph
 * @param state state
 * @param parent_group parent group key
 * @return boolean indicating if there is more in this group that needs to be
 * scheduled
 */
static bool is_there_more_to_schedule_in_noncircular_group(
    AUG_GRAPH* aug_graph,
    TOTAL_ORDER_STATE* state,
    CHILD_PHASE* parent_group) {
  int n = aug_graph->instances.length;
  int i;
  for (i = 0; i < n; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];

    // Only consider non-circular instances
    if (instance_in_aug_cycle(aug_graph, in) > -1)
      continue;

    // Instance in the same group but cannot be considered
    CHILD_PHASE* group_key = &state->instance_groups[i];

    // Check if in the same group
    if (child_phases_are_equal(parent_group, group_key)) {
      if (!state->schedule[i]) {
        return true;
      }
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
                        short ph,
                        short ch,
                        bool immediate,
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
  bool node_is_cond = false;

  if (group_is_local(&current->child_phase) &&
      if_rule_p(current->cto_instance->fibered_attr.attr)) {
    node_is_cond = true;
    followed_by(current->cto_if_true, ph, ch, immediate, visit_marker_only,
                &if_true_any);
  }

  followed_by(current->cto_next, ph, ch, immediate, visit_marker_only,
              &if_false_any);

  *any |= (if_true_any || if_false_any);
}

/**
 * Utility function used by assert_total_order to check sanity of total order
 *  - After visit marker <ph,-1> there should be no attribute
 * belonging to <ph,-1> or <-ph,-1>
 *  - Immediately before visit marker <ph,ch> should be attribute
 * belonging to <-ph,ch>
 *  - Or immediately after visit marker <ph,ch> should be attribute belonging
 * to <ph,ch>
 * @param current CTO_NODE node
 * @param prev_group <ph,ch> group
 */
static void total_order_sanity_check(CTO_NODE* current,
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

      if (state->any_parent_synth[current_group->ph] &&
          !preceded_by_parent_synthesized_current_phase) {
        fatal_error(
            "Expected to be preceded by parent synthesized "
            "attribute of current phase <%d,%d>",
            current_group->ph, -1);
      }

      // Boolean indicating whether followed by inherited attribute of
      // parent <-(ph+1),-1>
      bool followed_by_parent_inherited_next_phase = false;
      followed_by(current->cto_next, -(current->child_phase.ph + 1), -1, false,
                  false, &followed_by_parent_inherited_next_phase);

      if (state->any_parent_inh[current_group->ph + 1] &&
          !followed_by_parent_inherited_next_phase) {
        fatal_error(
            "Expected to be followed by parent inherited "
            "attribute of next phase <%d,%d>",
            current_group->ph + 1, -1);
      }
    } else {
      // Boolean indicating whether visit marker was followed by child
      // synthesized attribute(s) <ph,ch>
      bool followed_by_child_synthesized = false;
      followed_by(current->cto_next, current->child_phase.ph,
                  current->child_phase.ch, true, false,
                  &followed_by_child_synthesized);

      // Boolean indicating whether visit marker was followed by child
      // inherited attribute(s) <-ph,ch>
      bool preceded_by_child_inherited = prev_group->ph == -current_group->ph &&
                                         prev_group->ch == current_group->ch;

      if (!(followed_by_child_synthesized || preceded_by_child_inherited)) {
        fatal_error("After visit marker <ph,ch> the phase should be <ph,ch>.");
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
      total_order_sanity_check(current->cto_if_true, &current->child_phase,
                               prev_parent, state);
    }
  }

  total_order_sanity_check(current->cto_next, current_group, prev_parent,
                           state);
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
      followed_by(head, j, i, false, true, &any);

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
  total_order_sanity_check(head, &parent_inherited_group,
                           &parent_inherited_group, state);

  // Condition #3: consecutiveness of child visit calls
  child_visit_consecutive(aug_graph, state, head);
}

/**
 * Utility function to handle start of scheduling of groups
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_noncircular_transition_start_of_group(
    AUG_GRAPH* aug_graph,
    CTO_NODE* prev,
    CONDITION cond,
    TOTAL_ORDER_STATE* state,
    int remaining,
    CHILD_PHASE* group,
    short parent_ph) {
  CTO_NODE* cto_node;

  // If we are starting to schedule child synthesized attribute outside of
  // group scheduler it means child synthesized attribute did not immediately
  // followed child inherited attribute thus add a visit marker <ph,ch>
  if (group->ph > 0 && group->ch > -1) {
    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = group->ph;
    cto_node->child_phase.ch = group->ch;
    cto_node->child_decl = state->children.array[group->ch];
    cto_node->visit = parent_ph;
    cto_node->cto_next = schedule_noncircular_group(
        aug_graph, prev, cond, state, remaining, group, parent_ph);
    return cto_node;
  }

  // If parent phase is greater than current parent attribute phase then we
  // have reached the end of previous phase and so add a end of parent phase
  // visit marker <ph,-1>
  if (abs(group->ph) > parent_ph && group->ch == -1) {
    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = parent_ph;
    cto_node->child_phase.ch = -1;
    cto_node->visit = parent_ph;
    cto_node->cto_next = schedule_noncircular_group(
        aug_graph, prev, cond, state, remaining, group, parent_ph + 1);
    return cto_node;
  }

  return schedule_noncircular_group(aug_graph, prev, cond, state, remaining,
                                    group, parent_ph);
}

/**
 * Utility function to handle start of scheduling of groups
 * @param aug_graph Augmented dependency graph
 * @param cyc Cycle
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_circular_transition_start_of_group(
    AUG_GRAPH* aug_graph,
    CYCLE* cyc,
    CTO_NODE* prev,
    CONDITION cond,
    TOTAL_ORDER_STATE* state,
    int remaining,
    CHILD_PHASE* group,
    short parent_ph) {
  CTO_NODE* cto_node;

  // If we are starting to schedule child synthesized attribute outside of
  // group scheduler it means child synthesized attribute did not immediately
  // followed child inherited attribute thus add a visit marker <ph,ch>
  if (group->ph > 0 && group->ch > -1) {
    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = group->ph;
    cto_node->child_phase.ch = group->ch;
    cto_node->child_decl = state->children.array[group->ch];
    cto_node->visit = parent_ph;
    cto_node->cto_next = schedule_circular_group(
        aug_graph, cyc, prev, cond, state, remaining, group, parent_ph);
    return cto_node;
  }

  // If parent phase is greater than current parent attribute phase then we
  // have reached the end of previous phase and so add a end of parent phase
  // visit marker <ph,-1>
  if (abs(group->ph) > parent_ph && group->ch == -1) {
    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = parent_ph;
    cto_node->child_phase.ch = -1;
    cto_node->visit = parent_ph;
    cto_node->cto_next = schedule_circular_group(
        aug_graph, cyc, prev, cond, state, remaining, group, parent_ph + 1);
    return cto_node;
  }

  return schedule_circular_group(aug_graph, cyc, prev, cond, state, remaining,
                                 group, parent_ph);
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
static CTO_NODE* schedule_noncircular_transition_end_of_group(
    AUG_GRAPH* aug_graph,
    CTO_NODE* prev,
    CONDITION cond,
    TOTAL_ORDER_STATE* state,
    int remaining,
    CHILD_PHASE* group,
    short parent_ph) {
  // If we find ourselves scheduling a <-ph,ch>, we need to (after putting in
  // all the instances in that group), we need to schedule the visit of the
  // child (add a CTO node with a null instance but with <ph,ch) and then ALSO
  // schedule immediately all the syn attributes of that child's phase.
  // (<+ph,ch> group, if any).
  if (group->ph < 0 && group->ch > -1) {
    // Visit marker
    CTO_NODE* cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = -group->ph;
    cto_node->child_phase.ch = group->ch;
    cto_node->child_decl = state->children.array[group->ch];
    cto_node->visit = parent_ph;
    cto_node->cto_next = schedule_noncircular_group(
        aug_graph, cto_node, cond, state, remaining /* no change */,
        &cto_node->child_phase, parent_ph);
    return cto_node;
  }

  // If we find ourselves scheduling a <+ph,ch> then we need to add visit marker
  // for empty phase and then continue with greedy scheduler. This is because
  // there is no instance in empty phase to guide us to add visit marker.
  if (group->ph > 0 && group->ch != 0) {
    PHY_GRAPH* phy =
        Declaration_info(state->children.array[group->ch])->node_phy_graph;

    if (phy->empty_phase[group->ph + 1]) {
      // Visit marker
      CTO_NODE* cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
      cto_node->cto_prev = prev;
      cto_node->cto_instance = NULL;
      cto_node->child_phase.ph = group->ph;
      cto_node->child_phase.ch = group->ch;
      cto_node->child_decl = state->children.array[group->ch];
      cto_node->visit = parent_ph;
      cto_node->cto_next = schedule_noncircular(
          aug_graph, cto_node, cond, state, remaining /* no change */,
          &cto_node->child_phase, parent_ph);
      return cto_node;
    }
  }

  // Fallback to normal scheduler
  return schedule_noncircular(aug_graph, prev, cond, state,
                              remaining /* no change */, group, parent_ph);
}

/**
 * Utility function to handle visit end marker
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param instance_groups array of <ph,ch> indexed by INSTANCE index
 * @param parent_ph current parent phase being worked on
 * @return head of linked list
 */
static CTO_NODE* schedule_end(AUG_GRAPH* aug_graph,
                              CTO_NODE* prev,
                              TOTAL_ORDER_STATE* state,
                              short parent_ph) {
  CTO_NODE* cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
  cto_node->cto_prev = prev;
  cto_node->cto_instance = NULL;
  cto_node->child_phase.ph = parent_ph;
  cto_node->child_phase.ch = -1;
  cto_node->visit = parent_ph;
  return cto_node;
}

static CTO_NODE* schedule_circular_group(AUG_GRAPH* aug_graph,
                                         CYCLE* cyc,
                                         CTO_NODE* prev,
                                         CONDITION cond,
                                         TOTAL_ORDER_STATE* state,
                                         int remaining,
                                         CHILD_PHASE* group,
                                         short parent_ph) {}

/**
 * Circular group scheduler
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_circular(AUG_GRAPH* aug_graph,
                                   CYCLE* cyc,
                                   CTO_NODE* prev,
                                   CONDITION cond,
                                   TOTAL_ORDER_STATE* state,
                                   int remaining,
                                   CHILD_PHASE* group,
                                   short parent_ph) {
  int i, j, k;
  int n = aug_graph->instances.length;

  for (i = 0; i < cyc->instances.length; i++) {
    INSTANCE* in = &cyc->instances.array[i];
    CTO_NODE* cto_node;

    // Already scheduled
    if (state->schedule[in->index])
      continue;

    // check to see if makes sense (No need to schedule something that occurs
    // only in a different condition branch.)
    if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, instance_condition(in))) {
      state->schedule[i] = true;
      cto_node = schedule_circular(aug_graph, cyc, prev, cond, state,
                                   remaining - 1, group, parent_ph);
      state->schedule[i] = false;
      return cto_node;
    }

    // If all instances that directly depend on this are already scheduled
    bool following_natural_order = true;
    for (j = 0; j < cyc->instances.length; j++) {
      INSTANCE* other = &cyc->instances.array[j];

      if (state->schedule[other->index])
        continue;

      DEPENDENCY dep = get_edgeset_combine_dependencies(
          aug_graph->graph[other->index * n + in->index]);

      if (dep & DEPENDENCY_MAYBE_DIRECT) {
        following_natural_order = false;
      }
    }

    // We need to use the natural order when scheduling attributes in the cycle
    if (!following_natural_order)
      continue;

    // If it is local then continue scheduling
    if (instance_is_local(aug_graph, state, i)) {
      cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
      cto_node->cto_prev = prev;
      cto_node->cto_instance = in;
      cto_node->child_phase.ch = group->ch;
      cto_node->child_phase.ph = group->ph;
      cto_node->visit = parent_ph;
      state->schedule[i] =
          true;  // instance has been scheduled (and will not be
                 // considered for scheduling in the recursive call)

      if (if_rule_p(in->fibered_attr.attr)) {
        int cmask = 1 << (if_rule_index(in->fibered_attr.attr));
        cond.negative |= cmask;
        cto_node->cto_if_false =
            schedule_circular(aug_graph, cyc, cto_node, cond, state,
                              remaining - 1, group, parent_ph);
        cond.negative &= ~cmask;
        cond.positive |= cmask;
        cto_node->cto_if_true =
            schedule_circular(aug_graph, cyc, cto_node, cond, state,
                              remaining - 1, group, parent_ph);
        cond.positive &= ~cmask;
      } else {
        cto_node->cto_next =
            schedule_circular(aug_graph, cyc, cto_node, cond, state,
                              remaining - 1, group, parent_ph);
      }

      state->schedule[i] = false;  // Release it

      return cto_node;
    } else {
      // Instance is not local then delegate it to circular group scheduler
      return schedule_circular_transition_start_of_group(
          aug_graph, cyc, prev, cond, state, remaining, group, parent_ph);
    }
  }

  // Fallback to noncircular scheduler
  return schedule_noncircular(aug_graph, prev, cond, state, remaining, group,
                              parent_ph);
}

/**
 * Recursive scheduling function to only schedule
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_circular_find_cycle(AUG_GRAPH* aug_graph,
                                              CTO_NODE* prev,
                                              CONDITION cond,
                                              TOTAL_ORDER_STATE* state,
                                              int remaining,
                                              CHILD_PHASE* group,
                                              short parent_ph) {
  int i, j, k;
  int n = aug_graph->instances.length;

  for (i = 0; i < aug_graph->cycles.length; i++) {
    CYCLE* cyc = &aug_graph->cycles.array[i];

    // If any of instances in this cycle are scheduled then this cycle is
    // already schedule
    if (state->schedule[cyc->instances.array[0].index])
      continue;

    size_t temp_schedule_size = n * sizeof(bool);
    bool* temp_schedule = (bool*)alloca(temp_schedule_size);
    memcpy(temp_schedule, state->schedule, temp_schedule_size);

    // Temporarily mark all attributes in this scheduled
    for (j = 0; j < cyc->instances.length; j++) {
      INSTANCE* in = &cyc->instances.array[j];
      temp_schedule[in->index] = 1;
    }

    bool cycle_ready = true;

    for (j = 0; j < cyc->instances.length; j++) {
      INSTANCE* in = &cyc->instances.array[j];

      for (k = 0; k < n; k++) {
        if (temp_schedule[k])
          continue;

        int index = k * n + in->index;

        EDGESET edges;

        // Look at all dependencies from instances in the cycle to instances
        // that are not scheduled
        for (edges = aug_graph->graph[index]; edges != NULL;
             edges = edges->rest) {
          // If the merge condition is impossible, ignore this edge
          if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, edges->cond))
            continue;

          if (oag_debug & DEBUG_ORDER) {
            // Can't continue with scheduling if a dependency with a
            // "possible" condition has not been scheduled yet
            printf("This edgeset was not ready to be scheduled because of:\n");
            print_edgeset(edges, stdout);
            printf("\n");
          }

          cycle_ready = false;
        }
      }
    }

    // This cycle is ready to be scheduled as a group
    if (cycle_ready) {
      // Delegate it to the circular group scheduler
      return schedule_circular(aug_graph, cyc, prev, cond, state, remaining,
                               group, parent_ph);
    }
  }

  // We get to circular scheduler is non-circular scheduler cannot go any
  // further if circular scheduler cannot go any further then we can not
  // scheduler this attribute grammar
  if (remaining > 0) {
    fflush(stdout);
    print_schedule_error_debug(aug_graph, state, prev, cond, stderr);
    fatal_error("Cannot make conditional total order!");
  }

  return NULL;
}

/**
 * Recursive scheduling function
 * @param aug_graph Augmented dependency graph
 * @param prev previous CTO node
 * @param cond current CONDITION
 * @param state state
 * @param remaining count of remaining instances to schedule
 * @param group parent group key
 * @return head of linked list
 */
static CTO_NODE* schedule_noncircular_group(AUG_GRAPH* aug_graph,
                                            CTO_NODE* prev,
                                            CONDITION cond,
                                            TOTAL_ORDER_STATE* state,
                                            int remaining,
                                            CHILD_PHASE* group,
                                            short parent_ph) {
  int i;
  int n = aug_graph->instances.length;
  CTO_NODE* cto_node = prev;

  /* If nothing more to do, we are done. */
  if (remaining == 0) {
    return schedule_visit_end(aug_graph, prev, state, parent_ph);
  }

  /* Outer condition is impossible, its a dead-end branch */
  if (CONDITION_IS_IMPOSSIBLE(cond))
    return NULL;

  for (i = 0; i < n; i++) {
    INSTANCE* instance = &aug_graph->instances.array[i];
    CHILD_PHASE* instance_group = &state->instance_groups[i];

    // Already scheduled then ignore
    if (state->schedule[i])
      continue;

    // Do not scheduled circular attribute instances in non-circular scheduler
    if (instance_in_aug_cycle(aug_graph, instance) > -1)
      continue;

    // Check if everything is in the same group, don't check for dependencies
    // Locals will never end-up in this function
    if (instance_group->ph == group->ph && instance_group->ch == group->ch) {
      cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
      cto_node->cto_prev = prev;
      cto_node->cto_instance = instance;
      cto_node->child_phase.ph = group->ph;
      cto_node->child_phase.ch = group->ch;

      state->schedule[i] =
          true;  // instance has been scheduled (and will not be
                 // considered for scheduling in the recursive call)

      if (if_rule_p(instance->fibered_attr.attr)) {
        int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
        cond.negative |= cmask;
        cto_node->cto_if_false = schedule_noncircular(
            aug_graph, cto_node, cond, state, remaining - 1, group, parent_ph);
        cond.negative &= ~cmask;
        cond.positive |= cmask;
        cto_node->cto_if_true = schedule_noncircular(
            aug_graph, cto_node, cond, state, remaining - 1, group, parent_ph);
        cond.positive &= ~cmask;
      } else {
        cto_node->cto_next = schedule_noncircular_group(
            aug_graph, cto_node, cond, state, remaining - 1, group, parent_ph);
      }

      state->schedule[i] = false;  // Release it

      return cto_node;
    }
  }

  // Group is finished
  if (!is_there_more_to_schedule_in_noncircular_group(aug_graph, state,
                                                      group)) {
    return schedule_noncircular_transition_end_of_group(
        aug_graph, cto_node, cond, state, remaining, group, parent_ph);
  }

  // Try circular scheduler now
  if (remaining > 0) {
    return schedule_circular_find_cycle(aug_graph, prev, cond, state, remaining,
                                        group, parent_ph);
  }

  // All done with scheduling
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
static CTO_NODE* schedule_noncircular(AUG_GRAPH* aug_graph,
                                      CTO_NODE* prev,
                                      CONDITION cond,
                                      TOTAL_ORDER_STATE* state,
                                      int remaining,
                                      CHILD_PHASE* prev_group,
                                      short parent_ph) {
  int i;
  int n = aug_graph->instances.length;
  CTO_NODE* cto_node = NULL;

  // If nothing more to do, we are done.
  if (remaining == 0) {
    return schedule_end(aug_graph, prev, state, parent_ph);
  }

  // Outer condition is impossible, its a dead-end branch
  if (CONDITION_IS_IMPOSSIBLE(cond))
    return NULL;

  for (i = 0; i < n; i++) {
    INSTANCE* instance = &aug_graph->instances.array[i];
    CHILD_PHASE* group = &state->instance_groups[i];

    // Already scheduled then ignore
    if (state->schedule[i])
      continue;

    // Do not scheduled circular attribute instances in non-circular scheduler
    if (instance_in_aug_cycle(aug_graph, instance) > -1)
      continue;

    // If edgeset condition is not impossible then go ahead with scheduling
    if (noncircular_group_ready_to_go(aug_graph, state, cond, i)) {
      // check to see if makes sense (no need to schedule something that occurs
      // only in a different condition branch.)
      if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, instance_condition(instance))) {
        state->schedule[i] = true;
        cto_node = schedule_noncircular(aug_graph, prev, cond, state,
                                        remaining - 1, prev_group, parent_ph);
        state->schedule[i] = false;
        return cto_node;
      }

      // If it is local then continue scheduling
      if (instance_is_local(aug_graph, state, i)) {
        cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
        cto_node->cto_prev = prev;
        cto_node->cto_instance = instance;
        cto_node->child_phase.ch = group->ch;
        cto_node->child_phase.ph = group->ph;
        cto_node->visit = parent_ph;
        state->schedule[i] =
            true;  // instance has been scheduled (and will not be
                   // considered for scheduling in the recursive call)

        if (if_rule_p(instance->fibered_attr.attr)) {
          int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
          cond.negative |= cmask;
          cto_node->cto_if_false =
              schedule_noncircular(aug_graph, cto_node, cond, state,
                                   remaining - 1, prev_group, parent_ph);
          cond.negative &= ~cmask;
          cond.positive |= cmask;
          cto_node->cto_if_true =
              schedule_noncircular(aug_graph, cto_node, cond, state,
                                   remaining - 1, prev_group, parent_ph);
          cond.positive &= ~cmask;
        } else {
          cto_node->cto_next =
              schedule_noncircular(aug_graph, cto_node, cond, state,
                                   remaining - 1, prev_group, parent_ph);
        }

        state->schedule[i] = false;  // Release it

        return cto_node;
      } else {
        // Instance is not local then delegate it to group scheduler
        return schedule_noncircular_transition_start_of_group(
            aug_graph, prev, cond, state, remaining, group, parent_ph);
      }
    }
  }

  if (remaining > 0) {
    fflush(stdout);
    print_schedule_error_debug(aug_graph, state, prev, cond, stderr);
    fatal_error("Cannot make conditional total order!");

    return NULL;
  }

  return schedule_end(aug_graph, prev, state, parent_ph);
}

/**
 * Utility function to get children of augmented dependency graph as array of
 * declarations
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

static void schedule_augmented_dependency_graph(CYCLES cycles,
                                                AUG_GRAPH* aug_graph) {
  int n = aug_graph->instances.length;
  CONDITION cond;
  int i, j, k;

  (void)close_augmented_dependency_graph(aug_graph);

  // Now schedule graph: we need to generate a conditional total order.

  if (oag_debug & PROD_ORDER) {
    printf("Scheduling conditional total order for %s\n",
           aug_graph_name(aug_graph));
  }
  if (oag_debug & DEBUG_ORDER) {
    for (int i = 0; i < n; ++i) {
      INSTANCE* in = &(aug_graph->instances.array[i]);
      print_instance(in, stdout);
      printf(": ");
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

  // Find children of augmented graph: this will be used as argument to visit
  // calls
  set_aug_graph_children(aug_graph, state);

  size_t schedule_size = n * sizeof(bool);
  bool* schedule = (bool*)alloca(schedule_size);

  // False here means nothing is scheduled yet
  memset(schedule, false, schedule_size);
  state->schedule = schedule;

  // Set default max phase of parent
  state->max_parent_ph = 1;

  size_t max_child_ph_size = state->children.length * sizeof(short);
  short* max_child_ph = (short*)alloca(max_child_ph_size);

  // Set default max phase of children indexed by child index
  memset(max_child_ph, (int)0, max_child_ph_size);
  state->max_child_ph = max_child_ph;

  // Collect max_parent_ph and max_child_ph
  for (i = 0; i < n; i++) {
    CHILD_PHASE group = instance_groups[i];

    if (group.ch == -1) {
      state->max_parent_ph = MAX(abs(group.ph), state->max_parent_ph);
    } else if (!group_is_local(&group)) {
      state->max_child_ph[group.ch] =
          MAX(abs(group.ph), state->max_child_ph[group.ch]);
    }
  }

  // Collect parent_inh and parent_synth
  size_t parent_inh_synth_size = (state->max_parent_ph + 1) * sizeof(bool);
  bool* any_parent_inh = (bool*)alloca(parent_inh_synth_size);
  bool* any_parent_synth = (bool*)alloca(parent_inh_synth_size);

  memset(any_parent_inh, false, parent_inh_synth_size);
  memset(any_parent_synth, false, parent_inh_synth_size);

  state->any_parent_inh = any_parent_inh;
  state->any_parent_synth = any_parent_synth;

  for (i = 0; i < n; i++) {
    CHILD_PHASE group = instance_groups[i];

    if (group.ch == -1) {
      if (group.ph > 0) {
        state->any_parent_synth[group.ph] = true;
      } else {
        state->any_parent_inh[-group.ph] = true;
      }
    }
  }

  if (oag_debug & DEBUG_ORDER) {
    printf("\nInstances %s:\n", decl_name(aug_graph->syntax_decl));
    for (i = 0; i < n; i++) {
      INSTANCE* in = &(aug_graph->instances.array[i]);
      CHILD_PHASE group = state->instance_groups[i];
      print_instance(in, stdout);
      printf(": ");

      if (!group.ph && !group.ch) {
        printf("local\n");
      } else {
        printf("<%d,%d>\n", group.ph, group.ch);
      }
    }
  }

  cond.negative = 0;
  cond.positive = 0;

  // It is safe to assume inherited attribute of parents have no dependencies
  // and should be scheduled right away
  aug_graph->total_order = schedule_noncircular_group(
      aug_graph, NULL, cond, state, n, &parent_inherited_group, 1);

  if (aug_graph->total_order == NULL) {
    fatal_error("Failed to create total order.");
  }

  if (oag_debug & DEBUG_ORDER) {
    printf("\nSchedule for %s (%d children):\n",
           decl_name(aug_graph->syntax_decl), state->children.length);
    print_total_order(aug_graph->total_order, 0, stdout);
  }

  // Ensure generated total order is valid
  assert_total_order(aug_graph, state, aug_graph->total_order);
}

/**
 * @brief Computes total-preorder of set of attributes
 * @param module APS module
 * @param s state
 */
void compute_schedule(Declaration module, STATE* s) {
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
    schedule_augmented_dependency_graph(s->cycles, &s->aug_graphs[j]);
  }
  schedule_augmented_dependency_graph(s->cycles, &s->global_dependencies);

  if (analysis_debug & (DNC_ITERATE | DNC_FINAL)) {
    printf("*** FINAL OAG ANALYSIS STATE ***\n");
    print_analysis_state(s, stdout);
    print_cycles(s, stdout);
  }
}
