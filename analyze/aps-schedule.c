#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aps-ag.h"
#include "aps-debug.h"
#include "aps-tree.h"
#include "jbb-alloc.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define IS_VISIT_MARKER(node) (node->cto_instance == NULL)
#define CHUNK_GUIDING_DEPENDENCY (indirect_control_dependency)

static int BUFFER_SIZE = 1000;

// Enum representing type of chunks
enum ChunkTypeEnum {
  HalfLeft = 1,   // Parent inherited attribute
  HalfRight = 2,  // Parent synthesized attribute
  Local = 4,      // Local
  Visit = 8       // Child visit
};

struct chunk_type {
  enum ChunkTypeEnum type;  // Type of chunk
  int ph;                   // Phase
  int ch;                   // Child number
  int index;                // Index of the chunk in the array of chunks
  bool circular;  // Boolean indicating whether chunk is circular or not
  VECTOR(INSTANCE) instances;  // Array of instances in the chunk
};

typedef struct chunk_type Chunk;

struct chunk_graph_type {
  DEPENDENCY* graph;
  VECTOR(Chunk) instances;
  bool* schedule;
  SCC_COMPONENTS* chunk_components;
};

typedef struct chunk_graph_type ChunkGraph;

// Utility struct to keep of track of information needed to handle group
// scheduling
struct total_order_state {
  // Tuple <ph,ch> indexed by instance number
  CHILD_PHASE* instance_groups;
  VECTOR(Declaration) children;
};

typedef struct total_order_state TOTAL_ORDER_STATE;

static CTO_NODE* group_schedule(AUG_GRAPH* aug_graph,
                                ChunkGraph* chunk_graph,
                                SCC_COMPONENT* chunk_component,
                                const int chunk_component_index,
                                Chunk* chunk,
                                CTO_NODE* prev,
                                CONDITION cond,
                                TOTAL_ORDER_STATE* state,
                                const int remaining,
                                CHILD_PHASE* group,
                                const short parent_ph);

static CTO_NODE* schedule_visit_start(AUG_GRAPH* aug_graph,
                                      ChunkGraph* chunk_graph,
                                      CTO_NODE* prev,
                                      CONDITION cond,
                                      TOTAL_ORDER_STATE* state,
                                      const int remaining,
                                      const short parent_ph);

static CTO_NODE* chunk_schedule(AUG_GRAPH* aug_graph,
                                ChunkGraph* chunk_graph,
                                SCC_COMPONENT* chunk_component,
                                const int chunk_component_index,
                                CTO_NODE* prev,
                                CONDITION cond,
                                TOTAL_ORDER_STATE* state,
                                const int remaining,
                                const short parent_ph);

static CTO_NODE* chunk_component_schedule(AUG_GRAPH* aug_graph,
                                          ChunkGraph* chunk_graph,
                                          CTO_NODE* prev,
                                          CONDITION cond,
                                          TOTAL_ORDER_STATE* state,
                                          const int prev_chunk_component_index,
                                          const int remaining,
                                          const short parent_ph);

static bool chunk_ready_to_go(AUG_GRAPH* aug_graph,
                              ChunkGraph* chunk_graph,
                              SCC_COMPONENT* sink_component,
                              const int sink_component_index,
                              Chunk* sink_chunk,
                              TOTAL_ORDER_STATE* state);

static int chunk_indexof(AUG_GRAPH* aug_graph,
                         ChunkGraph* chunk_graph,
                         INSTANCE* instance);

static CTO_NODE* schedule_visit_end(AUG_GRAPH* aug_graph,
                                    ChunkGraph* chunk_graph,
                                    Chunk* chunk,
                                    CTO_NODE* prev,
                                    CONDITION cond,
                                    TOTAL_ORDER_STATE* state,
                                    const int remaining,
                                    CHILD_PHASE* prev_group,
                                    const short parent_ph);

static int chunk_indexof(AUG_GRAPH* aug_graph,
                         ChunkGraph* chunk_graph,
                         INSTANCE* instance);

static int chunk_component_indexof(AUG_GRAPH* aug_graph,
                                   ChunkGraph* chunk_graph,
                                   Chunk* chunk);

static int chunk_lookup(AUG_GRAPH* aug_graph,
                        ChunkGraph* chunk_graph,
                        enum ChunkTypeEnum type,
                        short ph,
                        short ch);

/**
 * Utility function that checks whether instance belongs to any phylum cycle
 * or not
 * @param phy_graph phylum graph
 * @param in attribute instance
 * @return -1 if instance does not belong to any cycle or index of phylum
 * cycle
 */
static bool instance_in_phylum_cycle(PHY_GRAPH* phy_graph, INSTANCE* in) {
  int n = phy_graph->instances.length;

  return phy_graph->mingraph[in->index * n + in->index];
}

/**
 * Utility function that checks whether instance belongs to any augmented
 * dependency graph cycle or not
 * @param aug_graph augmented dependency graph
 * @param in attribute instance
 * @return -1 if instance does not belong to any cycle or index of augmented
 * dependency graph cycle
 */
static bool instance_in_aug_cycle(AUG_GRAPH* aug_graph, INSTANCE* in) {
  int n = aug_graph->instances.length;

  return edgeset_kind(aug_graph->graph[in->index * n + in->index]);
}

/**
 * Utility function that returns boolean indicating whether attributes in SCC
 * contain parent or not
 * @param aug_graph augmented dependency graph
 * @param comp SCC component
 * @return boolean indicating whether attributes of parent are in SCC
 */
static bool scc_involves_parent(AUG_GRAPH* aug_graph, SCC_COMPONENT* comp) {
  int i;
  for (i = 0; i < comp->length; i++) {
    INSTANCE* in = (INSTANCE*)comp->array[i];

    if (aug_graph->lhs_decl == in->node) {
      return true;
    }
  }

  return false;
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

  if (group_is_local(&current->child_phase) &&
      if_rule_p(current->cto_instance->fibered_attr.attr)) {
    followed_by(current->cto_if_true, ph, ch, immediate, visit_marker_only,
                any);
  }

  followed_by(current->cto_next, ph, ch, immediate, visit_marker_only,
              any);
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
                                     CONDITION cond,
                                     const short ph,
                                     const short ch) {
  int i;
  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];
    CHILD_PHASE* group = &state->instance_groups[in->index];

    if (group->ph == ph && group->ch == ch &&
        !MERGED_CONDITION_IS_IMPOSSIBLE(instance_condition(in), cond)) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Utility function used by assert_total_order to check sanity of total order
 *  1) After end of phase visit marker <ph,-1> there should be no attribute
 *     belonging to <ph,-1> or <-ph,-1> because parent phase has ended.
 *  2) Immediately before child visit marker <ph,ch> there should be attribute
 *     belonging to <-ph,ch> (if any)
 *  3) Immediately after visit marker <ph,ch> should be attribute belonging
 *     to <ph,ch> (if any)
 * @param current CTO_NODE node
 * @param prev_group <ph,ch> group
 */
static void total_order_sanity_check(AUG_GRAPH* aug_graph,
                                     ChunkGraph* chunk_graph,
                                     CTO_NODE* current,
                                     CONDITION cond,
                                     CHILD_PHASE* prev_group,
                                     CHILD_PHASE* prev_parent,
                                     TOTAL_ORDER_STATE* state) {
  if (current == NULL)
    return;

  CHILD_PHASE* current_group = &current->child_phase;

  if (IS_VISIT_MARKER(current)) {
    // If end of parent phase visit marker
    if (current->child_phase.ch == -1) {
      // Boolean indicating whether end of phase visit marker was preceded
      // by parent synthesized attributes <ph,-1> if any
      bool preceded_by_parent_synthesized_current_phase =
          prev_parent->ph > 0 && prev_parent->ch == -1;

      if (aug_graph_contains_phase(aug_graph, state, cond, current_group->ph,
                                   -1) &&
          !preceded_by_parent_synthesized_current_phase) {
        fatal_error(
            "[%s] Expected parent synthesized attribute <%+d,%+d> precede the "
            "end of phase visit marker <%+d,%+d>",
            aug_graph_name(aug_graph), current_group->ph, -1, current_group->ph,
            -1);
      }

      // End of total order so no need to check if after <ph,-1>
      // it is followed by <-(ph+1),-1> because this is the end
      if (current->cto_next == NULL)
        return;

      // Boolean indicating whether followed by inherited attribute of
      // parent for the next phase <-(ph+1),-1> if any
      bool followed_by_parent_inherited_next_phase = false;
      followed_by(current->cto_next, -(current->child_phase.ph + 1), -1,
                  false /* not immediate */, false /* not visit markers only*/,
                  &followed_by_parent_inherited_next_phase);

      if (aug_graph_contains_phase(aug_graph, state, cond,
                                   -(current_group->ph + 1), -1) &&
          !followed_by_parent_inherited_next_phase) {
        aps_warning(current,
                    "[%s] Expected after end of phase visit marker "
                    "<%+d,%+d> to be "
                    "followed by parent inherited attribute of next "
                    "phase <%+d,%+d>",
                    aug_graph_name(aug_graph), current_group->ph, -1,
                    -(current_group->ph + 1), -1);
      }
    } else {
      // Boolean indicating whether child visit marker was followed by child
      // synthesized attribute(s) <ph,ch> if any
      bool followed_by_child_synthesized = false;
      followed_by(current->cto_next, current->child_phase.ph,
                  current->child_phase.ch, true /* immediate */,
                  false /* not visit markers only*/,
                  &followed_by_child_synthesized);

      // Boolean indicating whether visit marker was followed by child
      // inherited attribute(s) <-ph,ch>
      bool preceded_by_child_inherited = prev_group->ph == -current_group->ph &&
                                         prev_group->ch == current_group->ch;

      // If graph contains <ph,ch> then after visit should be group of <ph,ch>
      // Similarly, if graph contains <-ph,ch>, before the visit should be group of <-ph,ch> 
      if (aug_graph_contains_phase(aug_graph, state, cond,
                                   current->child_phase.ph,
                                   current->child_phase.ch) &&
          !followed_by_child_synthesized) {
        fatal_error(
            "[%s] After visit marker <%+d,%+d> the phase should be <ph,ch> "
            "(i.e. "
            "<%+d,%+d>).",
            aug_graph_name(aug_graph), current->child_phase.ph,
            current->child_phase.ch, current->child_phase.ph,
            current->child_phase.ch);
      } else if (aug_graph_contains_phase(aug_graph, state, cond,
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

    // If instance is conditional then adjust the positive and negative
    // conditions
    if (if_rule_p(current->cto_instance->fibered_attr.attr)) {
      int cmask =
          1 << (if_rule_index(current->cto_instance->fibered_attr.attr));

      cond.positive |= cmask;
      total_order_sanity_check(aug_graph, chunk_graph, current->cto_if_true,
                               cond, &current->child_phase, prev_parent, state);
      cond.positive &= ~cmask;

      cond.negative |= cmask;
      total_order_sanity_check(aug_graph, chunk_graph, current->cto_if_false,
                               cond, &current->child_phase, prev_parent, state);
      cond.negative &= ~cmask;
      return;
    }
  }

  // Continue the sanity check
  total_order_sanity_check(aug_graph, chunk_graph, current->cto_next, cond,
                           current_group, prev_parent, state);
}

/**
 * Helper function to assert child visits are consecutive
 * @param current head of total order linked list
 * @param ph head of total order linked list
 * @param ch head of total order linked list
 */
static void child_visit_consecutive_check(CTO_NODE* current,
                                          AUG_GRAPH* aug_graph,
                                          ChunkGraph* chunk_graph,
                                          short ph,
                                          short ch) {
  if (current == NULL)
    return;

  if (IS_VISIT_MARKER(current) && current->child_phase.ch == ch) {
    if (current->child_phase.ph != ph) {
      int current_visit_component_index = chunk_component_indexof(
          aug_graph, chunk_graph,
          &chunk_graph->instances.array[chunk_lookup(
              aug_graph, chunk_graph, Visit, current->child_phase.ph, ch)]);

      int target_visit_component_index = chunk_component_indexof(
          aug_graph, chunk_graph,
          &chunk_graph->instances
               .array[chunk_lookup(aug_graph, chunk_graph, Visit, ph, ch)]);

      if (current_visit_component_index != target_visit_component_index) {
        fatal_error(
            "Out of order child visits, expected visit(%d,%d) but "
            "received visit(%d,%d)",
            ph, ch, current->child_phase.ph, current->child_phase.ch);
      } else {
        aps_warning(current,
                    "Out of order child visits, expected visit(%d,%d) but "
                    "received visit(%d,%d)",
                    ph, ch, current->child_phase.ph, current->child_phase.ch);
      }
    }

    // Done with this phase. Now check the next phase of this child
    child_visit_consecutive_check(current->cto_next, aug_graph, chunk_graph,
                                  ph + 1, ch);
    return;
  }

  if (current->cto_if_true != NULL) {
    child_visit_consecutive_check(current->cto_if_true, aug_graph, chunk_graph,
                                  ph, ch);
  }

  child_visit_consecutive_check(current->cto_next, aug_graph, chunk_graph, ph,
                                ch);
}

/**
 * This function asserts that visits for a particular child are consecutive
 * @param aug_graph Augmented dependency graph
 * @param state state
 * @param head head of total order linked list
 */
static void child_visit_consecutive(AUG_GRAPH* aug_graph,
                                    ChunkGraph* chunk_graph,
                                    TOTAL_ORDER_STATE* state,
                                    CTO_NODE* head) {
  int i;
  for (i = 0; i < state->children.length; i++) {
    child_visit_consecutive_check(head, aug_graph, chunk_graph, 1, i);
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
    Declaration decl = state->children.array[i];
    PHY_GRAPH* phy_graph = DECL_PHY_GRAPH(decl);
    if (phy_graph == NULL)
      continue;

    int max_phase = phy_graph->max_phase;

    for (j = 1; j <= max_phase; j++) {
      bool any = false;
      followed_by(head, j, i, false /* not immediate */,
                  true /* visit markers only */, &any);

      if (!any) {
        fatal_error(
            "Missing child %d (%s) visit call for phase %d in %s aug_graph", i,
            decl_name(decl), j, aug_graph_name(aug_graph));
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

  CHILD_PHASE group = cto_node->child_phase;

  if (IS_VISIT_MARKER(cto_node)) {
    if (group.ch == -1) {
      parent_ph += 1;
    } else {
      if (!group_is_local(&group)) {
        PHY_GRAPH* parent_phy = DECL_PHY_GRAPH(aug_graph->lhs_decl);
        PHY_GRAPH* child_phy = DECL_PHY_GRAPH(state->children.array[group.ch]);

        if (child_phy != NULL && parent_phy->cyclic_flags[parent_ph] &&
            !child_phy->cyclic_flags[group.ph]) {
          fatal_error(
              "Non-circular child visit marker <%+d,%+d> "
              "cannot be scheduled in circular parent visit %d",
              group.ph, group.ch, parent_ph);
        }
      }
    }
  } else {
    if (!group_is_local(&group) && group.ch > -1) {
      PHY_GRAPH* parent_phy = DECL_PHY_GRAPH(aug_graph->lhs_decl);
      PHY_GRAPH* child_phy = DECL_PHY_GRAPH(state->children.array[group.ch]);

      char instance_to_str[BUFFER_SIZE];
      FILE* f = fmemopen(instance_to_str, sizeof(instance_to_str), "w");
      print_instance(cto_node->cto_instance, f);
      fclose(f);

      if (parent_phy->cyclic_flags[parent_ph] &&
          !instance_in_aug_cycle(aug_graph, cto_node->cto_instance)) {
        fatal_error(
            "While investigating %s <%+d,%+d>: non-circular child instance "
            "cannot be scheduled in circular parent visit %d",
            instance_to_str, group.ph, group.ch, parent_ph);
      }
    }
  }

  if (cto_node->cto_if_true != NULL) {
    check_circular_visit(aug_graph, state, cto_node->cto_if_true, parent_ph);
  }

  check_circular_visit(aug_graph, state, cto_node->cto_next, parent_ph);
}

static void ensure_instances_are_in_one_visit(AUG_GRAPH* aug_graph,
                                              TOTAL_ORDER_STATE* state,
                                              CTO_NODE* cto_node,
                                              SCC_COMPONENT* comp,
                                              int parent_ph) {
  if (cto_node == NULL)
    return;

  if (cto_node->cto_instance != NULL) {
    int i;
    for (i = 0; i < comp->length; i++) {
      INSTANCE* in = (INSTANCE*)comp->array[i];
      CHILD_PHASE* group = &state->instance_groups[in->index];

      // Instance is in this SCC
      if (in == cto_node->cto_instance) {
        // We are just initializing the parent_ph to make sure parent_ph is the
        // same for all instances
        if (parent_ph == -1) {
          parent_ph = cto_node->visit;
        } else if (parent_ph != cto_node->visit) {
          char instance_to_str[BUFFER_SIZE];
          FILE* f = fmemopen(instance_to_str, sizeof(instance_to_str), "w");
          print_instance(cto_node->cto_instance, f);
          fclose(f);

          aps_warning(
              NULL,
              "Instance %s <%+d,%+d> of circular SCC involving parent should "
              "be contained in one parent visit. Expected them all to be in "
              "%d parent phase but saw instance in %d parent phase.",
              instance_to_str, group->ph, group->ch, parent_ph,
              cto_node->visit);
        }
      }
    }
  }

  if (cto_node->cto_if_true != NULL) {
    ensure_instances_are_in_one_visit(aug_graph, state, cto_node->cto_if_true,
                                      comp, parent_ph);
  }

  ensure_instances_are_in_one_visit(aug_graph, state, cto_node->cto_next, comp,
                                    parent_ph);
}

static void check_circular_scc_scheduled_in_one_visit(AUG_GRAPH* aug_graph,
                                                      TOTAL_ORDER_STATE* state,
                                                      CTO_NODE* cto_node) {
  int i;
  for (i = 0; i < aug_graph->components->length; i++) {
    SCC_COMPONENT* comp = aug_graph->components->array[i];

    if (scc_involves_parent(aug_graph, comp)) {
      ensure_instances_are_in_one_visit(aug_graph, state, cto_node, comp, -1);
    }
  }
}

static void validate_linked_list(AUG_GRAPH* aug_graph,
                                 CTO_NODE* cto_node,
                                 CTO_NODE* prev) {
  if (cto_node == NULL)
    return;

  if (cto_node->cto_prev != prev) {
    fatal_error("Prev of CTO_NODE is not set correctly.");
    return;
  }

  if (cto_node->cto_next != NULL) {
    if (cto_node->cto_next->cto_prev != cto_node) {
      fatal_error("Next node's Prev is not set correctly.");
      return;
    }
  }

  if (cto_node->cto_if_true != NULL) {
    validate_linked_list(aug_graph, cto_node->cto_if_true, cto_node);
  }

  validate_linked_list(aug_graph, cto_node->cto_next, cto_node);
}

static void check_duplicates(AUG_GRAPH* aug_graph,
                             CTO_NODE* cto_node,
                             bool* schedule) {
  if (cto_node == NULL)
    return;

  if (cto_node->cto_instance != NULL) {
    if (schedule[cto_node->cto_instance->index]) {
      char instance_to_str[BUFFER_SIZE];
      FILE* f = fmemopen(instance_to_str, sizeof(instance_to_str), "w");
      print_instance(cto_node->cto_instance, f);
      fclose(f);

      fatal_error("Instance %s (%d) is duplicated in linkedlist",
                  instance_to_str, cto_node->cto_instance->index);
      return;
    } else {
      schedule[cto_node->cto_instance->index] = true;
    }
  }

  if (cto_node->cto_if_true != NULL) {
    size_t schedule_size = aug_graph->instances.length * sizeof(bool);
    bool* old_schedule = (bool*)alloca(schedule_size);
    memcpy(old_schedule, schedule, schedule_size);

    check_duplicates(aug_graph, cto_node->cto_if_true, schedule);

    schedule = old_schedule;
  }

  check_duplicates(aug_graph, cto_node->cto_next, schedule);
}

/**
 * Function that throws an error if phases are out of order
 * @param aug_graph
 * @param head head of linked list
 * @param state current value of ph being investigated
 */
static void assert_total_order(AUG_GRAPH* aug_graph,
                               ChunkGraph* chunk_graph,
                               TOTAL_ORDER_STATE* state,
                               CTO_NODE* head) {
  // Condition #0: Ensure linkedlist prev, next are set correctly
  validate_linked_list(aug_graph, head, NULL);

  // Condition #1: completeness of child visit calls
  child_visit_completeness(aug_graph, state, head);

  CHILD_PHASE* parent_inherited_group =
      (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
  parent_inherited_group->ph = 1;
  parent_inherited_group->ch = -1;

  CONDITION cond;
  cond.positive = 0;
  cond.negative = 0;

  // Condition #2: general sanity of total order using visit markers
  total_order_sanity_check(aug_graph, chunk_graph, head, cond,
                           parent_inherited_group, parent_inherited_group,
                           state);

  // Condition #3: consecutive of child visit calls
  child_visit_consecutive(aug_graph, chunk_graph, state, head);

  // Condition #4: ensure no non-circular visit of a child in the circular
  // visit of the parent.
  check_circular_visit(aug_graph, state, head, 1);

  // Condition #5: Ensure circular visit involving parent are done as one
  // visit
  check_circular_scc_scheduled_in_one_visit(aug_graph, state, head);

  size_t schedule_size = aug_graph->instances.length * sizeof(bool);
  bool* schedule = (bool*)alloca(schedule_size);
  memset(schedule, false, schedule_size);

  // Condition #6: Ensure no duplicate attribute instance is happening
  check_duplicates(aug_graph, head, schedule);
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

  for (i = 0; i < phy_graph->components->length; i++) {
    SCC_COMPONENT* comp = phy_graph->components->array[i];

    // Not a circular SCC
    if (!phy_graph->component_cycle[i])
      continue;

    // This cycle is already scheduled
    if (phy_graph->summary_schedule[((INSTANCE*)comp->array[0])->index])
      continue;

    size_t temp_schedule_size = n * sizeof(int);
    int* temp_schedule = (int*)alloca(temp_schedule_size);
    memcpy(temp_schedule, phy_graph->summary_schedule, temp_schedule_size);

    // Temporarily mark all attributes in this cycle as scheduled
    for (j = 0; j < comp->length; j++) {
      INSTANCE* in = (INSTANCE*)comp->array[j];
      temp_schedule[in->index] = 1;
    }

    bool cycle_ready = true;

    for (j = 0; j < comp->length; j++) {
      INSTANCE* in = (INSTANCE*)comp->array[j];
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
      for (j = 0; j < comp->length; j++) {
        INSTANCE* in = (INSTANCE*)comp->array[j];
        // -phase for inherited attributes
        // +phase for synthesized attributes
        phy_graph->summary_schedule[in->index] =
            instance_direction(in) == instance_inward ? -phase : phase;

        if (oag_debug & TOTAL_ORDER) {
          printf("%+d ", phy_graph->summary_schedule[in->index]);
          print_instance(in, stdout);
          printf(" [%s]\n", instance_direction(in) == instance_inward
                                ? "inherited"
                                : "synthesized");
        }
      }

      return comp->length;
    }
  }

  return 0;
}

/**
 * Utility function that schedules a single phase of all non-circular
 * attributes
 * @param phy_graph phylum graph
 * @param ph phase its currently scheduling for
 * @param ignore_cycles ignore cycles
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
        !instance_in_phylum_cycle(phy_graph, in) &&
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
          if (sch != 0 && sch != -phase) {
            phy_graph->mingraph[j * n + i] = indirect_control_dependency;
          }
        }
        if (oag_debug & TOTAL_ORDER) {
          printf("%d ", -phase);
          print_instance(in, stdout);
          printf(" [inherited]\n");
        }
      }
    }
  }

  /* now schedule synthesized attributes */
  for (i = 0; i < n; i++) {
    INSTANCE* in = &phy_graph->instances.array[i];
    if (instance_direction(in) == instance_outward &&
        !instance_in_phylum_cycle(phy_graph, in) &&
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
          if (sch != 0 && sch != phase) {
            phy_graph->mingraph[j * n + i] = indirect_control_dependency;
          }
        }
        if (oag_debug & TOTAL_ORDER) {
          printf("+%d ", phase);
          print_instance(in, stdout);
          printf(" [synthesized]\n");
        }
      }
    }
  }

  return done;
}

/**
 * Utility function that calculates ph (phase) for each attribute of a phylum
 * - Note that phases always should start and end with non-circular and there should be
 * empty non-circular between two circular phases.
 * - By this point, fiber cycles have been broken by fiber cycle logic (up/down)
 * @param phy_graph phylum graph to schedule
 */
static void schedule_summary_dependency_graph(PHY_GRAPH* phy_graph) {
  int n = phy_graph->instances.length;
  int phase = 1;
  int done = 0;
  bool cont = true;

  if (oag_debug & TOTAL_ORDER) {
    printf("Scheduling order for %s\n", decl_name(phy_graph->phylum));
  }

  // Find SCC components of instances given a phylum graph
  set_phylum_graph_components(phy_graph);

  // Nothing is scheduled
  memset(phy_graph->summary_schedule, 0, n * sizeof(int));

  int i, j;
  int phase_count = n + 1;
  size_t phase_size_bool = phase_count * sizeof(bool);
  size_t phase_size_int = phase_count * sizeof(int);

  bool* circular_phase = (bool*)HALLOC(phase_size_bool);
  memset(circular_phase, false, phase_size_bool);

  bool* empty_phase = (bool*)HALLOC(phase_size_bool);
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

  // Ensure there is a non-circular at the end if its done scheduling
  if (done == n && cycle_happened)
  {
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

  if (oag_debug & TOTAL_ORDER) {
    printf("\n");
  }

  if (done < n) {
    if (cycle_debug & PRINT_CYCLE) {
      printf("Failed to schedule phylum graph for: %s\n",
             phy_graph_name(phy_graph));
      for (i = 0; i < n; i++) {
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
          for (j = 0; j < n; j++) {
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
static void print_total_order(AUG_GRAPH* aug_graph,
                              CTO_NODE* cto,
                              int chunk_index,
                              bool chunk_circular,
                              TOTAL_ORDER_STATE* state,
                              int indent,
                              FILE* stream) {
  if (cto == NULL) {
    if (oag_debug & DEBUG_ORDER_VERBOSE) {
      fprintf(stream, " Finished scheduling [%s] Chunk #%d\n\n",
              chunk_circular ? "circular" : "non-circular", chunk_index);
    }
    return;
  }
  bool extra_newline = false;
  bool print_group = true;

  if (cto->chunk_index != chunk_index) {
    if (chunk_index != -1) {
      if (oag_debug & DEBUG_ORDER_VERBOSE) {
        fprintf(stream, " Finished scheduling [%s] Chunk #%d\n\n",
                cto->chunk_circular ? "circular" : "non-circular", chunk_index);
      }
    }

    chunk_index = cto->chunk_index;
    chunk_circular = cto->chunk_circular;
    if (oag_debug & DEBUG_ORDER_VERBOSE) {
      fprintf(stream, " Started scheduling [%s] Chunk #%d\n",
              cto->chunk_circular ? "circular" : "non-circular", chunk_index);
    }
  }

  if (cto->cto_instance == NULL) {
    print_indent(indent, stream);
    if (cto->child_phase.ch != -1) {
      PHY_GRAPH* child_phy_graph =
          DECL_PHY_GRAPH(state->children.array[cto->child_phase.ch]);
      fprintf(stream, "-> [%s] ",
              child_phy_graph->empty_phase[cto->child_phase.ph]
                  ? "empty visit"
                  : "none empty visit");
    } else {
      fprintf(stream, "<- ");
    }
    fprintf(stream, "visit marker (%d) ", cto->visit);
    if (cto->child_decl == NULL && cto->child_phase.ch != -1) {
      fatal_error("Missing child_decl for visit marker <%+d,%+d>.",
                  cto->child_phase.ph, cto->child_phase.ch);
    }
    if (cto->child_decl != NULL) {
      fprintf(stream, " (%s) ", decl_name(cto->child_decl));
    } else {
      extra_newline = true;
    }
  } else {
    print_indent(indent, stream);
    print_instance(cto->cto_instance, stream);
    CONDITION cond = instance_condition(cto->cto_instance);
    fprintf(stream, " (visit: %d, component: %d, lineno: %d)", cto->visit,
            cto->chunk_index,
            tnode_line_number(cto->cto_instance->fibered_attr.attr));

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
    print_total_order(aug_graph, cto->cto_if_true, chunk_index, chunk_circular,
                      state, indent + 2, stdout);
    print_indent(indent + 1, stream);
    fprintf(stream, "(false)\n");
    indent += 2;
  }

  print_total_order(aug_graph, cto->cto_next, chunk_index, chunk_circular,
                    state, indent, stdout);
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
            aug_graph->schedule[i] ? "scheduled" : "not-scheduled");
  }

  fprintf(stderr, "\nNon-scheduled SCCs (%s):\n", aug_graph_name(aug_graph));

  for (i = 0; i < aug_graph->components->length; i++) {
    SCC_COMPONENT* comp = aug_graph->components->array[i];

    if (aug_graph->schedule[((INSTANCE*)comp->array[0])->index])
      continue;

    printf("Starting SCC #%d\n", i);
    for (j = 0; j < comp->length; j++) {
      INSTANCE* in = (INSTANCE*)comp->array[j];
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

  print_total_order(aug_graph, prev, -1, false, state, 0, stream);

  printf("Relevant dependencies from non-scheduled instances: \n");
  for (i = 0; i < n; i++) {
    if (aug_graph->schedule[i])
      continue;

    for (j = 0; j < n; j++) {
      if (aug_graph->schedule[j])
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
 * Returns the rank of a chunk (higher the rank equals the higher the priority).
 * The rank will be used to schedule a chunk from list of ready-to-go chunks.
 * @param chunk chunk instance
 * @return int rank
 */
static int get_chunk_rank(Chunk* chunk) {
  switch (chunk->type) {
    case HalfLeft:
      return 8;
    case HalfRight:
      return 2;
    case Visit:
    case Local:
      return 4;
    default: {
      fatal_error("Unknown chunk type %d.", chunk->type);
      return 0;
    }
  }
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
      if (!aug_graph->schedule[in->index]) {
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
    INSTANCE* in = (INSTANCE*)comp->array[i];

    // Instance in the same group but cannot be considered
    CHILD_PHASE* group_key = &state->instance_groups[in->index];

    // Check if in the same group
    if (child_phases_are_equal(parent_group, group_key)) {
      if (!aug_graph->schedule[in->index]) {
        return true;
      }
    }
  }

  return false;
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

static void print_chunk_info(Chunk* chunk) {
  switch (chunk->type) {
    case HalfLeft: {
      printf("Parent inherited for phase: %d", chunk->ph);
      break;
    }
    case HalfRight: {
      printf("Parent synthesized for phase: %d", chunk->ph);
      break;
    }
    case Visit: {
      printf("Child %d %s%s visit for phase: %d", chunk->ch,
             chunk->instances.length ? "" : "empty ",
             chunk->circular ? "circular" : "non-circular", chunk->ph);
      break;
    }
    case Local: {
      printf("Local %sattribute",
             if_rule_p(chunk->instances.array[0].fibered_attr.attr)
                 ? "conditional "
                 : "");
      break;
    }
  }
}

static void print_chunk(AUG_GRAPH* aug_graph,
                        TOTAL_ORDER_STATE* state,
                        Chunk* chunk,
                        int indent) {
  int i;
  print_indent(indent, stdout);
  printf("index: %d [%s] ", chunk->index,
         chunk->circular ? "circular" : "non-circular");

  print_chunk_info(chunk);
  printf("\n");

  for (i = 0; i < chunk->instances.length; i++) {
    INSTANCE* in = &chunk->instances.array[i];
    CHILD_PHASE* group = &state->instance_groups[in->index];

    print_indent(indent + 1, stdout);
    print_instance(in, stdout);
    printf(" <%+d,%+d>\n", group->ph, group->ch);
  }
  printf("\n");
}

/**
 * Utility function to handle transitions between groups while scheduling
 * cycles
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
static CTO_NODE* schedule_transition_end_of_group(
    AUG_GRAPH* aug_graph,
    ChunkGraph* chunk_graph,
    SCC_COMPONENT* chunk_component,
    const int chunk_component_index,
    Chunk* chunk,
    CTO_NODE* prev,
    CONDITION cond,
    TOTAL_ORDER_STATE* state,
    const int remaining,
    CHILD_PHASE* group,
    const short parent_ph) {
  CTO_NODE* cto_node;

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting schedule_transition_end_of_group (%s) with "
        "(remaining: %d, group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  // Child visit marker
  if (group->ph < 0 && group->ch != -1) {
    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = -group->ph;
    cto_node->child_phase.ch = group->ch;
    cto_node->child_decl = state->children.array[group->ch];
    cto_node->visit = parent_ph;
    cto_node->chunk_index = chunk->index;
    cto_node->chunk_circular = chunk->circular;
    cto_node->cto_next = group_schedule(
        aug_graph, chunk_graph, chunk_component, chunk_component_index, chunk,
        cto_node, cond, state, remaining, &cto_node->child_phase, parent_ph);
    return cto_node;
  }

  // End of parent visit marker
  if (group->ph > 0 && group->ch == -1) {
    cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
    cto_node->cto_prev = prev;
    cto_node->cto_instance = NULL;
    cto_node->child_phase.ph = group->ph;
    cto_node->child_phase.ch = -1;
    cto_node->child_decl = NULL;
    cto_node->visit = parent_ph;
    cto_node->chunk_index = chunk->index;
    cto_node->chunk_circular = chunk->circular;
    cto_node->cto_next =
        schedule_visit_start(aug_graph, chunk_graph, cto_node, cond, state,
                             remaining, parent_ph + 1);
    return cto_node;
  }

  // Fallback to normal scheduler to find the next chunk within the chunk
  // component to schedule
  return chunk_schedule(aug_graph, chunk_graph, chunk_component,
                        chunk_component_index, prev, cond, state,
                        remaining /* no change */, parent_ph);
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
static CTO_NODE* schedule_transition_start_of_group(
    AUG_GRAPH* aug_graph,
    ChunkGraph* chunk_graph,
    SCC_COMPONENT* chunk_component,
    const int chunk_component_index,
    Chunk* chunk,
    CTO_NODE* prev,
    CONDITION cond,
    TOTAL_ORDER_STATE* state,
    const int remaining,
    CHILD_PHASE* group,
    const short parent_ph) {
  CTO_NODE* cto_node;

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting schedule_transition_start_of_group (%s) with "
        "(remaining: %d, group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  // Continue with group scheduler
  return group_schedule(aug_graph, chunk_graph, chunk_component,
                        chunk_component_index, chunk, prev, cond, state,
                        remaining, group, parent_ph);
}

/**
 * Greedy group scheduler
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
static CTO_NODE* group_schedule(AUG_GRAPH* aug_graph,
                                ChunkGraph* chunk_graph,
                                SCC_COMPONENT* chunk_component,
                                const int chunk_component_index,
                                Chunk* chunk,
                                CTO_NODE* prev,
                                CONDITION cond,
                                TOTAL_ORDER_STATE* state,
                                const int remaining,
                                CHILD_PHASE* group,
                                const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting group_schedule (%s) with (remaining: %d, group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  int i;
  CTO_NODE* cto_node = prev;

  /* Outer condition is impossible, it's a dead-end branch */
  if (CONDITION_IS_IMPOSSIBLE(cond)) {
    return schedule_visit_end(aug_graph, chunk_graph, chunk, prev, cond, state,
                              remaining, group, parent_ph);
  }

  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* instance = &aug_graph->instances.array[i];
    CHILD_PHASE* instance_group = &state->instance_groups[instance->index];

    // Already scheduled
    if (aug_graph->schedule[instance->index])
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
      cto_node->chunk_index = chunk->index;
      cto_node->chunk_circular = chunk->circular;

      // instance has been scheduled (and will not be
      // considered for scheduling in the recursive call)
      aug_graph->schedule[instance->index] = true;

      chunk_graph->schedule[chunk_indexof(aug_graph, chunk_graph, instance)] =
          true;

      // If it is local then sometime is wrong
      if (instance_is_local(aug_graph, state, instance->index)) {
        fatal_error(
            "Group scheduler cannot handle scheduling local attributes");
        return NULL;
      }

      if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
        printf("-> Scheduled via group scheduler (instance: ");
        print_instance(instance, stdout);
        printf(
            ", group: <%+d,%+d>, cond: (%+d,%+d), inst_cond: (%+d,%+d), "
            "remaining: %d)\n\n",
            group->ph, group->ch, cond.positive, cond.negative,
            instance_condition(instance).positive,
            instance_condition(instance).negative, remaining - 1);
      }

      if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, instance_condition(instance))) {
        cto_node = group_schedule(aug_graph, chunk_graph, chunk_component,
                                  chunk_component_index, chunk, prev, cond,
                                  state, remaining - 1, group, parent_ph);
      } else {
        cto_node->cto_next = group_schedule(
            aug_graph, chunk_graph, chunk_component, chunk_component_index,
            chunk, cto_node, cond, state, remaining - 1, group, parent_ph);
      }

      chunk_graph->schedule[chunk_indexof(aug_graph, chunk_graph, instance)] =
          false;  // Release it

      aug_graph->schedule[instance->index] = false;

      return cto_node;
    }
  }

  chunk_graph->schedule[chunk->index] = true;

  // Group scheduling is finished
  cto_node = schedule_transition_end_of_group(
      aug_graph, chunk_graph, chunk_component, chunk_component_index, chunk,
      cto_node, cond, state, remaining, group, parent_ph);

  chunk_graph->schedule[chunk->index] = false;

  return cto_node;
}

/**
 * @brief local chunk scheduler.
 * This function is similar to group_schedule except it can handle IF
 * conditionals which are locals too.
 */
static CTO_NODE* local_chunk_schedule(AUG_GRAPH* aug_graph,
                                      ChunkGraph* chunk_graph,
                                      SCC_COMPONENT* chunk_component,
                                      const int chunk_component_index,
                                      Chunk* chunk,
                                      CTO_NODE* prev,
                                      CONDITION cond,
                                      TOTAL_ORDER_STATE* state,
                                      const int remaining,
                                      CHILD_PHASE* group,
                                      const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting local_chunk_schedule (%s) with (remaining: %d, group: "
        "<%+d,%+d>, parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, group->ph, group->ch, parent_ph);
  }

  int i;
  CTO_NODE* cto_node = prev;

  /* Outer condition is impossible, it's a dead-end branch */
  if (CONDITION_IS_IMPOSSIBLE(cond)) {
    return schedule_visit_end(aug_graph, chunk_graph, chunk, prev, cond, state,
                              remaining, group, parent_ph);
  }

  for (i = 0; i < chunk->instances.length; i++) {
    INSTANCE* instance = &chunk->instances.array[i];
    CHILD_PHASE* instance_group = &state->instance_groups[instance->index];

    // Already scheduled
    if (aug_graph->schedule[instance->index])
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
      cto_node->chunk_index = chunk->index;
      cto_node->chunk_circular = chunk->circular;

      // instance has been scheduled (and will not be
      // considered for scheduling in the recursive call)
      aug_graph->schedule[instance->index] = true;

      chunk_graph->schedule[chunk_indexof(aug_graph, chunk_graph, instance)] =
          true;

      // If it is local then something is wrong
      if (!instance_is_local(aug_graph, state, instance->index)) {
        fatal_error("Local chunk scheduler can only handle local attributes");
        return NULL;
      }

      if (MERGED_CONDITION_IS_IMPOSSIBLE(cond, instance_condition(instance))) {
        cto_node = local_chunk_schedule(
            aug_graph, chunk_graph, chunk_component, chunk_component_index,
            chunk, prev, cond, state, remaining - 1, group, parent_ph);
      } else {
        if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
          printf("-> Scheduled via local scheduler (instance: ");
          print_instance(instance, stdout);
          printf(
              ", group: <%+d,%+d>, cond: (%+d,%+d), inst_cond: (%+d,%+d), "
              "remaining: %d)\n\n",
              group->ph, group->ch, cond.positive, cond.negative,
              instance_condition(instance).positive,
              instance_condition(instance).negative, remaining - 1);
        }

        if (if_rule_p(instance->fibered_attr.attr)) {
          int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));

          cond.positive |= cmask;
          cto_node->cto_if_true = local_chunk_schedule(
              aug_graph, chunk_graph, chunk_component, chunk_component_index,
              chunk, cto_node, cond, state, remaining - 1, group, parent_ph);
          cond.positive &= ~cmask;

          cond.negative |= cmask;
          cto_node->cto_if_false = local_chunk_schedule(
              aug_graph, chunk_graph, chunk_component, chunk_component_index,
              chunk, cto_node, cond, state, remaining - 1, group, parent_ph);
          cond.negative &= ~cmask;

        } else {
          cto_node->cto_next = local_chunk_schedule(
              aug_graph, chunk_graph, chunk_component, chunk_component_index,
              chunk, cto_node, cond, state, remaining - 1, group, parent_ph);
        }
      }

      chunk_graph->schedule[chunk_indexof(aug_graph, chunk_graph, instance)] =
          false;  // Release it

      aug_graph->schedule[instance->index] = false;

      return cto_node;
    }
  }

  // Group scheduling is finished
  return schedule_transition_end_of_group(
      aug_graph, chunk_graph, chunk_component, chunk_component_index, chunk,
      cto_node, cond, state, remaining, group, parent_ph);
}

static CTO_NODE* group_schedule_chunk(AUG_GRAPH* aug_graph,
                                      ChunkGraph* chunk_graph,
                                      SCC_COMPONENT* chunk_component,
                                      const int chunk_component_index,
                                      Chunk* chunk,
                                      CTO_NODE* prev,
                                      CONDITION cond,
                                      TOTAL_ORDER_STATE* state,
                                      const int remaining,
                                      const short parent_ph) {
  switch (chunk->type) {
    case HalfLeft: {
      CHILD_PHASE* group = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
      group->ch = -1;
      group->ph = -chunk->ph;

      return group_schedule(aug_graph, chunk_graph, chunk_component,
                            chunk_component_index, chunk, prev, cond, state,
                            remaining, group, parent_ph);
    }
    case HalfRight: {
      CHILD_PHASE* group = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
      group->ch = -1;
      group->ph = chunk->ph;

      return group_schedule(aug_graph, chunk_graph, chunk_component,
                            chunk_component_index, chunk, prev, cond, state,
                            remaining, group, parent_ph);
    }
    case Visit: {
      CHILD_PHASE* group = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
      group->ch = chunk->ch;
      group->ph = -chunk->ph;

      return group_schedule(aug_graph, chunk_graph, chunk_component,
                            chunk_component_index, chunk, prev, cond, state,
                            remaining, group, parent_ph);
    }
    case Local: {
      CHILD_PHASE* group = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
      group->ch = chunk->ch;
      group->ph = chunk->ph;

      return local_chunk_schedule(aug_graph, chunk_graph, chunk_component,
                                  chunk_component_index, chunk, prev, cond,
                                  state, remaining, group, parent_ph);
    }
    default: {
      fatal_error("Unknown chunk type.");
      return NULL;
    }
  }

  return NULL;
}

/**
 * @brief Scheduler that finds the next chunk in a given SCC to schedule
 */
static CTO_NODE* chunk_schedule(AUG_GRAPH* aug_graph,
                                ChunkGraph* chunk_graph,
                                SCC_COMPONENT* chunk_component,
                                const int chunk_component_index,
                                CTO_NODE* prev,
                                CONDITION cond,
                                TOTAL_ORDER_STATE* state,
                                const int remaining,
                                const short parent_ph) {
  size_t rank_array_size = chunk_component->length * sizeof(int);
  int* rank_array = (int*)alloca(rank_array_size);
  memset(rank_array, (int)0, rank_array_size);

  int i, j;

  // Just an assertion to make sure all instances in the chunks marked as
  // scheduled have actually been scheduled
  for (i = 0; i < chunk_graph->instances.length; i++) {
    Chunk* chunk = &chunk_graph->instances.array[i];

    if (chunk_graph->schedule[chunk->index])
      continue;

    for (j = 0; j < chunk->instances.length; j++) {
      INSTANCE* in = &chunk->instances.array[j];

      if (!aug_graph->schedule[in->index])
        break;
    }

    if (chunk->instances.length > 0 && j == chunk->instances.length) {
      aps_warning(
          chunk,
          "Chunk #%d was marked as not scheduled but all instances in this "
          "non-empty chunk are already scheduled.",
          chunk->index);
    }
  }

  // Find out which chunk(s) are available to schedule and determine their rank
  PHY_GRAPH* parent_phy = DECL_PHY_GRAPH(aug_graph->lhs_decl);
  for (i = 0; i < chunk_component->length; i++) {
    Chunk* chunk = (Chunk*)chunk_component->array[i];

    if (chunk->type == Visit && !chunk->circular &&
        parent_phy->cyclic_flags[parent_ph]) {
      aps_warning(
          chunk,
          "Manually prevented scheduling non-circular visit <%+d,%+d> in "
          "circular parent phase %d in %s augmented dependency graph",
          chunk->ch, chunk->ph, parent_ph, aug_graph_name(aug_graph));
      continue;
    }

    if (chunk_graph->schedule[chunk->index] ||
        !chunk_ready_to_go(aug_graph, chunk_graph, chunk_component,
                           chunk_component_index, chunk, state)) {
      continue;
    }

    int rank = get_chunk_rank(chunk);
    rank_array[i] = rank;
  }

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Finding next chunk to schedule from list of available chunks in "
        "component #%d of %s augmented dependency graph:\n",
        chunk_component_index, aug_graph_name(aug_graph));
  }

  int current_rank = 0;
  int max_rank_index = 0;
  for (i = 0; i < chunk_component->length; i++) {
    if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
      if (rank_array[i] > 0) {
        Chunk* chunk = (Chunk*)chunk_component->array[i];

        print_indent(2, stdout);
        printf("Rank=%d ", rank_array[i]);
        print_chunk_info(chunk);
        printf("\n");
      }
    }

    if (rank_array[i] > current_rank) {
      max_rank_index = i;
      current_rank = rank_array[i];
    }
  }

  // If no chunk ready to go, then trigger component scheduler
  if (current_rank == 0) {
    if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
      printf(
          "-> Found no chunk to schedule, moving on to component scheduler.\n");
    }

    return chunk_component_schedule(aug_graph, chunk_graph, prev, cond, state,
                                    chunk_component_index, remaining,
                                    parent_ph);
  } else {
    // Continue the group scheduler from the chunk
    Chunk* chunk_to_schedule_next = chunk_component->array[max_rank_index];

    if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
      print_indent(1, stdout);
      printf("Next chunk to schedule rank=%d ", current_rank);
      print_chunk_info(chunk_to_schedule_next);
      printf("\n");
    }

    return group_schedule_chunk(aug_graph, chunk_graph, chunk_component,
                                chunk_component_index, chunk_to_schedule_next,
                                prev, cond, state, remaining, parent_ph);
  }
}

/**
 * @brief Utility function that returns index of given chunk
 */
static Chunk* find_chunk(AUG_GRAPH* aug_graph,
                         ChunkGraph* chunk_graph,
                         enum ChunkTypeEnum type,
                         const int ph,
                         const int ch) {
  int i;
  for (i = 0; i < chunk_graph->instances.length; i++) {
    Chunk* chunk = &chunk_graph->instances.array[i];
    if (chunk->type == type && chunk->ph == ph && chunk->ch == ch) {
      return chunk;
    }
  }
  return NULL;
}

/**
 * @brief Utility function that returns index of belonging SCC of chunk
 */
static int chunk_indexof(AUG_GRAPH* aug_graph,
                         ChunkGraph* chunk_graph,
                         INSTANCE* instance) {
  int i, j;
  for (i = 0; i < chunk_graph->instances.length; i++) {
    Chunk* chunk = &chunk_graph->instances.array[i];

    for (j = 0; j < chunk->instances.length; j++) {
      INSTANCE* other_instance = &chunk->instances.array[j];

      if (instance->index == other_instance->index) {
        return chunk->index;
      }
    }
  }

  fatal_error(
      "Failed to find chunk with instance index %d in augmented "
      "dependency graph %s.",
      instance->index, aug_graph_name(aug_graph));

  return -1;
}

/**
 * @brief Utility function that returns index of belonging SCC of chunk
 */
static int chunk_component_indexof(AUG_GRAPH* aug_graph,
                                   ChunkGraph* chunk_graph,
                                   Chunk* chunk) {
  int i, j;
  for (i = 0; i < chunk_graph->chunk_components->length; i++) {
    SCC_COMPONENT* component = chunk_graph->chunk_components->array[i];

    for (j = 0; j < component->length; j++) {
      Chunk* other_chunk = component->array[j];

      if (chunk->index == other_chunk->index) {
        return i;
      }
    }
  }

  fatal_error(
      "Failed to find the SCC component of chunk with index %d in augmented "
      "dependency graph %s.",
      chunk->index, aug_graph_name(aug_graph));

  return -1;
}

/**
 * @brief Returns the index of a chunk with given properties or -1
 */
static int chunk_lookup(AUG_GRAPH* aug_graph,
                        ChunkGraph* chunk_graph,
                        enum ChunkTypeEnum type,
                        short ph,
                        short ch) {
  int i;
  for (i = 0; i < chunk_graph->instances.length; i++) {
    Chunk* chunk = &chunk_graph->instances.array[i];

    if (chunk->type == type && chunk->ch == ch && chunk->ph == ph) {
      return i;
    }
  }

  fatal_error(
      "Failed to find chunk in augmented "
      "dependency graph %s containing chunk with ph=%d, ch=%d and type=%d.",
      aug_graph_name(aug_graph), ph, ch, type);

  return -1;
}

static void debug_chunk_dependencies(ChunkGraph* chunk_graph,
                                     SCC_COMPONENT* sink_component,
                                     Chunk* sink_chunk) {
  int i, j;

  int* chunk_dependencies =
      (int*)alloca(sink_component->length * sizeof(int) * 2);
  int count = 0;
  chunk_dependencies[count++] = sink_chunk->index;

  Chunk* current_sink_chunk = sink_chunk;

  j = 0;
  while (true) {
    for (i = 0; i < sink_component->length; i++) {
      Chunk* source_chunk = sink_component->array[i];

      if (source_chunk->index == current_sink_chunk->index)
        continue;

      if (chunk_graph->schedule[source_chunk->index])
        continue;

      DEPENDENCY forward_dep =
          chunk_graph
              ->graph[source_chunk->index * chunk_graph->instances.length +
                      current_sink_chunk->index];

      if (!forward_dep || !(forward_dep & DEPENDENCY_MAYBE_DIRECT))
        continue;

      chunk_dependencies[count++] = source_chunk->index;
      current_sink_chunk = source_chunk;
      break;
    }

    // If we are back to the chunk we started, then stop
    if (current_sink_chunk == sink_chunk)
      break;

    // If we have visited all chunks inside the component then we have found all
    // the sink_chunk dependencies, then stop
    if (i == sink_component->length)
      break;

    // If number of dependencies of the sink_chunk is greater than count of
    // components, then we are obviously in a cycle, then stop
    if (j > sink_component->length) {
      aps_warning(sink_chunk,
                  "Number of dependencies of the chunk #%d is more than number "
                  "of chunks in the component, there is a nested cycle.",
                  sink_chunk->index);
      break;
    }

    j++;
  }

  for (i = 0; i < count; i++) {
    print_indent(i, stdout);
    printf("<-");
    printf("Chunk #%d (", chunk_dependencies[i]);
    print_chunk_info(&chunk_graph->instances.array[chunk_dependencies[i]]);
    printf(")\n");
  }

  printf("\n");

  if (current_sink_chunk == sink_chunk) {
    printf("\n");
    aps_warning(sink_chunk,
                "Debugging chunk dependency sequence failed because chunk #%d "
                "directly depends on itself",
                sink_chunk->index);
  }
}

/**
 * @brief Utility function that return boolean indicating whether single chunk
 * is ready to go
 */
static bool chunk_ready_to_go(AUG_GRAPH* aug_graph,
                              ChunkGraph* chunk_graph,
                              SCC_COMPONENT* sink_component,
                              const int sink_component_index,
                              Chunk* sink_chunk,
                              TOTAL_ORDER_STATE* state) {
  int i, j, k;

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf("Checking readiness of chunk #%d (", sink_chunk->index);
    print_chunk_info(sink_chunk);
    printf(") of %s augmented dependency graph\n", aug_graph_name(aug_graph));
    print_indent(1, stdout);
    print_chunk(aug_graph, state, sink_chunk, 1);
  }

  for (i = 0; i < sink_chunk->instances.length; i++) {
    INSTANCE* source_instance = &sink_chunk->instances.array[i];
    if (!aug_graph->schedule[source_instance->index])
      break;
  }

  if (chunk_graph->graph[sink_chunk->index * chunk_graph->instances.length +
                         sink_chunk->index] &
      DEPENDENCY_MAYBE_DIRECT) {
    aps_warning(sink_chunk,
                "Scheduling chunk #%d of component #%d may not be possible as "
                "it directly depends on itself.",
                sink_chunk->index, sink_component_index);
  }

  // All instances in this chunk have already been scheduled
  if (sink_chunk->instances.length > 0 && i == sink_chunk->instances.length) {
    if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
      printf(" All instances in this non-empty chunk have been schedule.");
    }
    return false;
  }

  for (i = 0; i < sink_component->length; i++) {
    Chunk* source_chunk = (Chunk*)sink_component->array[i];

    // Check dependencies against myself
    if (source_chunk->index == sink_chunk->index)
      continue;

    // Already scheduled
    if (chunk_graph->schedule[source_chunk->index])
      continue;

    DEPENDENCY forward_dep =
        chunk_graph->graph[source_chunk->index * chunk_graph->instances.length +
                           sink_chunk->index];

    DEPENDENCY backward_dep =
        chunk_graph->graph[sink_chunk->index * chunk_graph->instances.length +
                           source_chunk->index];

    if (!forward_dep || !(forward_dep & DEPENDENCY_MAYBE_DIRECT))
      continue;

    // Cannot check dependency maybe direct against a chunk that we are in cycle
    // with kind = direct, it would be a cycle
    if ((forward_dep & DEPENDENCY_MAYBE_DIRECT) &&
        (backward_dep & DEPENDENCY_MAYBE_DIRECT)) {
      aps_warning(sink_chunk,
                  "Bi-directional direct dependency between chunk %d -> %d "
                  "(forward_dep=%d, backward_dep=%d)",
                  source_chunk->index, sink_chunk->index, forward_dep,
                  backward_dep);
      continue;
    }

    if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
      printf(
          "Chunk #%d within component %d not ready because of direct "
          "dependency (kind=%d) from chunk #%d (",
          sink_chunk->index, sink_component_index, forward_dep,
          source_chunk->index);
      print_chunk_info(source_chunk);
      printf(")\n");

      debug_chunk_dependencies(chunk_graph, sink_component, sink_chunk);
      printf("\n");

      for (j = 0; j < source_chunk->instances.length; j++) {
        INSTANCE* source_instance = &source_chunk->instances.array[j];
        CHILD_PHASE* source_group =
            &state->instance_groups[source_instance->index];

        if (aug_graph->schedule[source_instance->index])
          continue;

        for (k = 0; k < sink_chunk->instances.length; k++) {
          INSTANCE* sink_instance = &sink_chunk->instances.array[k];
          CHILD_PHASE* sink_group =
              &state->instance_groups[sink_instance->index];

          if (MERGED_CONDITION_IS_IMPOSSIBLE(
                  instance_condition(source_instance),
                  instance_condition(sink_instance)))
            continue;

          // See if there is a direct dependency between instance in this
          // component and everyone else within the SCC of this component
          DEPENDENCY dep = edgeset_kind(
              aug_graph
                  ->graph[source_instance->index * aug_graph->instances.length +
                          sink_instance->index]);
          if (dep & DEPENDENCY_MAYBE_DIRECT) {
            print_indent(1, stdout);
            printf("// ");
            print_instance(source_instance, stdout);
            printf(" <%+d,%+d> -> ", source_group->ph, source_group->ch);
            print_instance(sink_instance, stdout);
            printf(" <%+d,%+d> (kind=%d)\n", sink_group->ph, sink_group->ch,
                   dep);
          }
        }
      }
    }
    return false;
  }

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(" Chunk #%d is ready to go.\n", sink_chunk->index);
  }

  return true;
}

/**
 * @brief Utility function that return boolean indicating whether component of
 * chunks is ready to go
 */
static bool chunk_component_ready_to_go(AUG_GRAPH* aug_graph,
                                        ChunkGraph* chunk_graph,
                                        SCC_COMPONENT* sink_component,
                                        const int sink_component_index) {
  int i, j, k;

  // See if there is any instance in this chunk that has not been scheduled
  for (i = 0; i < sink_component->length; i++) {
    Chunk* chunk = (Chunk*)sink_component->array[i];

    if (!chunk_graph->schedule[chunk->index])
      break;
  }

  // All chunks in this component have already been scheduled, nothing else to
  // be done with this component
  if (i == sink_component->length)
    return false;

  for (i = 0; i < chunk_graph->chunk_components->length; i++) {
    SCC_COMPONENT* source_component = chunk_graph->chunk_components->array[i];

    // Check readiness against other chunk components, not itself
    if (i == sink_component_index)
      continue;

    for (j = 0; j < source_component->length; j++) {
      Chunk* source_chunk = (Chunk*)source_component->array[j];

      // This chunk is already scheduled
      if (chunk_graph->schedule[source_chunk->index])
        continue;

      for (k = 0; k < sink_component->length; k++) {
        Chunk* sink_chunk = (Chunk*)sink_component->array[k];

        if (chunk_graph
                ->graph[source_chunk->index * chunk_graph->instances.length +
                        sink_chunk->index]) {
          return false;
        }
      }
    }
  }

  return true;
}

/**
 * @brief Utility function that greedy schedules SCC of chunks
 */
static CTO_NODE* chunk_component_schedule(AUG_GRAPH* aug_graph,
                                          ChunkGraph* chunk_graph,
                                          CTO_NODE* prev,
                                          CONDITION cond,
                                          TOTAL_ORDER_STATE* state,
                                          const int prev_chunk_component_index,
                                          const int remaining,
                                          const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting chunk_component_schedule (%s) with (remaining: %d, "
        "parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, parent_ph);
  }

  int i, j;

  size_t rank_array_size = chunk_graph->chunk_components->length * sizeof(int);
  int* rank_array = (int*)alloca(rank_array_size);
  memset(rank_array, 0, rank_array_size);

  // Find SCC of chunk ready to be scheduled
  for (i = 0; i < chunk_graph->chunk_components->length; i++) {
    SCC_COMPONENT* chunk_component = chunk_graph->chunk_components->array[i];

    // This is needed to avoid requesting to schedule the same SCC, not doing
    // this can cause infinite loop because it will keep retrying to schedule
    // the same component
    if (prev_chunk_component_index == i)
      continue;

    // Found SCC of chunk that is ready
    if (chunk_component_ready_to_go(aug_graph, chunk_graph, chunk_component,
                                    i)) {
      // Schedule a chunk within this SCC of chunk
      for (j = 0; j < chunk_component->length; j++) {
        Chunk* chunk = (Chunk*)chunk_component->array[j];

        rank_array[i] |= get_chunk_rank(chunk);
      }
    }
  }

  int current_rank = 0;
  int max_rank_index = 0;
  for (i = 0; i < chunk_graph->chunk_components->length; i++) {
    if (rank_array[i] > current_rank) {
      max_rank_index = i;
      current_rank = rank_array[i];
    }
  }

  if (current_rank == 0) {
    fatal_error(
        "Not sure what to do at this point of scheduling for %s augmented "
        "dependency graph because there is no component to schedule and "
        "already tried scheduling component #%d and none of its chunks were "
        "scheduled.",
        aug_graph_name(aug_graph), prev_chunk_component_index);
    return NULL;
  }

  return chunk_schedule(aug_graph, chunk_graph,
                        chunk_graph->chunk_components->array[max_rank_index],
                        max_rank_index, prev, cond, state, remaining,
                        parent_ph);
}

/**
 * @brief Utility function that handles the necessary routine to end the
 * parent phase
 */
static CTO_NODE* schedule_visit_end(AUG_GRAPH* aug_graph,
                                    ChunkGraph* chunk_graph,
                                    Chunk* chunk,
                                    CTO_NODE* prev,
                                    CONDITION cond,
                                    TOTAL_ORDER_STATE* state,
                                    const int remaining,
                                    CHILD_PHASE* prev_group,
                                    const short parent_ph) {
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf(
        "Starting schedule_visit_end (%s) with "
        "(remaining: %d parent_ph: %d)\n",
        aug_graph_name(aug_graph), remaining, parent_ph);
  }

  CTO_NODE* cto_node = (CTO_NODE*)HALLOC(sizeof(CTO_NODE));
  cto_node->cto_prev = prev;
  cto_node->cto_instance = NULL;
  cto_node->child_phase.ph = parent_ph;
  cto_node->child_phase.ch = -1;
  cto_node->visit = parent_ph;
  cto_node->chunk_index = chunk->index;
  cto_node->chunk_circular = chunk->circular;
  cto_node->cto_next = schedule_visit_start(
      aug_graph, chunk_graph, cto_node, cond, state, remaining, parent_ph + 1);
  return cto_node;
}

/**
 * @brief Utility function that handles the necessary routine to start the
 * parent phase
 */
static CTO_NODE* schedule_visit_start(AUG_GRAPH* aug_graph,
                                      ChunkGraph* chunk_graph,
                                      CTO_NODE* prev,
                                      CONDITION cond,
                                      TOTAL_ORDER_STATE* state,
                                      const int remaining,
                                      const short parent_ph) {
  // Need to schedule parent inherited attributes of this phase
  CHILD_PHASE* parent_inh = (CHILD_PHASE*)alloca(sizeof(CHILD_PHASE));
  parent_inh->ph = -parent_ph;
  parent_inh->ch = -1;

  PHY_GRAPH* parent_phy = DECL_PHY_GRAPH(aug_graph->lhs_decl);

  // No more visit to schedule.
  if (parent_ph > parent_phy->max_phase)
    return NULL;

  // It is safe to assume inherited attribute of parents have no
  // dependencies and should be scheduled right away
  Chunk* chunk = find_chunk(aug_graph, chunk_graph, HalfLeft, parent_ph, -1);
  if (chunk == NULL) {
    fatal_error(
        "Chunks are not properly created because chunk of parent inherited "
        "attribute of phase %d is missing",
        parent_inh);
    return NULL;
  }

  int chunk_component_index =
      chunk_component_indexof(aug_graph, chunk_graph, chunk);

  if (chunk_component_index == -1) {
    fatal_error(
        "Chunk components are not properly created because component "
        "containing chunk of parent inherited "
        "attribute of phase %d is missing",
        parent_inh);
  }

  SCC_COMPONENT* chunk_component =
      chunk_graph->chunk_components->array[chunk_component_index];

  return group_schedule(aug_graph, chunk_graph, chunk_component,
                        chunk_component_index, chunk, prev, cond, state,
                        remaining, parent_inh, parent_ph);
}

/**
 * 1) collects chunks using <ph,ch> already assigned to the instances
 * 2) brings over dependencies from instances as chunk dependency
 * 3) adds guiding dependencies between chunks
 * 4) finds the SCC components of chunks
 * @param aug_graph augmented dependency graph
 * @param state scheduling state
 * @return chunk graph
 */
static ChunkGraph* collect_aug_graph_chunks(AUG_GRAPH* aug_graph,
                                            TOTAL_ORDER_STATE* state) {
  PHY_GRAPH* parent_phy = DECL_PHY_GRAPH(aug_graph->lhs_decl);

  Chunk** chunks_array =
      (Chunk**)alloca(aug_graph->instances.length * 10 * sizeof(Chunk*));
  int chunks_count = 0;

  int parent_ph, ph, ch;
  int i, j, k, l;
  INSTANCE* array =
      (INSTANCE*)alloca(aug_graph->instances.length * sizeof(INSTANCE));
  int count;

  // Collect half chunks (halfleft and halfright)
  for (parent_ph = 1; parent_ph <= parent_phy->max_phase; parent_ph++) {
    // Collect parent inherited of this phase
    array = (INSTANCE*)alloca(aug_graph->instances.length * sizeof(INSTANCE));
    count = 0;

    for (i = 0; i < aug_graph->instances.length; i++) {
      INSTANCE* in = &aug_graph->instances.array[i];
      CHILD_PHASE* group = &state->instance_groups[in->index];

      if (group->ph == -parent_ph && group->ch == -1) {
        array[count++] = *in;
      }
    }

    Chunk* left_half_chunk = (Chunk*)malloc(sizeof(Chunk));
    left_half_chunk->type = HalfLeft;
    left_half_chunk->ph = parent_ph;
    left_half_chunk->ch = -1;  // Irrelevant
    left_half_chunk->circular = parent_phy->cyclic_flags[parent_ph];
    left_half_chunk->index = chunks_count;

    VECTORALLOC(left_half_chunk->instances, INSTANCE, count);

    for (i = 0; i < count; i++) {
      left_half_chunk->instances.array[i] = array[i];
    }

    chunks_array[chunks_count++] = left_half_chunk;

    // Collect parent synthesized of this phase
    count = 0;

    for (i = 0; i < aug_graph->instances.length; i++) {
      INSTANCE* in = &aug_graph->instances.array[i];
      CHILD_PHASE* group = &state->instance_groups[in->index];

      if (group->ph == parent_ph && group->ch == -1) {
        array[count++] = *in;
      }
    }

    Chunk* right_half_chunk = (Chunk*)malloc(sizeof(Chunk));
    right_half_chunk->type = HalfRight;
    right_half_chunk->ph = parent_ph;
    right_half_chunk->ch = -1;  // Irrelevant
    right_half_chunk->circular = parent_phy->cyclic_flags[parent_ph];
    right_half_chunk->index = chunks_count;

    VECTORALLOC(right_half_chunk->instances, INSTANCE, count);

    for (i = 0; i < count; i++) {
      right_half_chunk->instances.array[i] = array[i];
    }

    chunks_array[chunks_count++] = right_half_chunk;
  }

  // Collect visits chunks
  for (ch = 0; ch < state->children.length; ch++) {
    Declaration child_decl = state->children.array[ch];
    PHY_GRAPH* child_phy = DECL_PHY_GRAPH(child_decl);

    if (child_phy == NULL)
      continue;

    for (ph = 1; ph <= child_phy->max_phase; ph++) {
      // Collect visit for this child phase
      count = 0;

      for (i = 0; i < aug_graph->instances.length; i++) {
        INSTANCE* in = &aug_graph->instances.array[i];
        CHILD_PHASE* group = &state->instance_groups[in->index];

        if (abs(group->ph) == ph && group->ch == ch) {
          array[count++] = *in;
        }
      }

      size_t size_of_chunk = sizeof(Chunk);
      Chunk* visit_chunk = (Chunk*)malloc(size_of_chunk);
      visit_chunk->type = Visit;
      visit_chunk->ph = ph;
      visit_chunk->ch = ch;
      visit_chunk->circular = child_phy->cyclic_flags[ph];
      visit_chunk->index = chunks_count;

      VECTORALLOC(visit_chunk->instances, INSTANCE, count);

      for (i = 0; i < count; i++) {
        visit_chunk->instances.array[i] = array[i];
      }

      chunks_array[chunks_count++] = visit_chunk;
    }
  }

  // Collect locals chunks
  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];
    CHILD_PHASE* group = &state->instance_groups[in->index];

    if (group->ph == 0 && group->ch == 0) {
      Chunk* local_chunk = (Chunk*)malloc(sizeof(Chunk));
      local_chunk->type = Local;
      local_chunk->ph = 0;  // Irrelevant
      local_chunk->ch = 0;  // Irrelevant
      local_chunk->index = chunks_count;
      local_chunk->circular = instance_in_aug_cycle(aug_graph, in);

      VECTORALLOC(local_chunk->instances, INSTANCE, 1);
      local_chunk->instances.array[0] = *in;

      chunks_array[chunks_count++] = local_chunk;
    }
  }

  ChunkGraph* chunk_graph = (ChunkGraph*)malloc(sizeof(ChunkGraph));
  chunk_graph->schedule = (bool*)calloc(sizeof(bool), chunks_count);
  chunk_graph->graph =
      (DEPENDENCY*)calloc(sizeof(DEPENDENCY), chunks_count * chunks_count);

  VECTORALLOC(chunk_graph->instances, Chunk, chunks_count);

  // Copy over from stack allocated array to heap array
  for (i = 0; i < chunks_count; i++) {
    chunk_graph->instances.array[i] = *chunks_array[i];
  }

  if ((oag_debug & DEBUG_ORDER)) {
    printf("\nlist of chunks for %s\n", aug_graph_name(aug_graph));
    for (i = 0; i < chunk_graph->instances.length; i++) {
      Chunk* chunk = &chunk_graph->instances.array[i];

      print_indent(1, stdout);
      if (oag_debug & DEBUG_ORDER_VERBOSE) {
        print_chunk(aug_graph, state, chunk, 1);
      } else {
        print_chunk_info(chunk);
        printf("\n");
      }
    }
    printf("\n");
  }

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf("Chunk dependency graph for %s augmented dependency graph:\n",
           aug_graph_name(aug_graph));
  }

  // Add edges between chunks
  for (i = 0; i < chunk_graph->instances.length; i++) {
    Chunk* chunk_source = &chunk_graph->instances.array[i];

    for (j = 0; j < chunk_graph->instances.length; j++) {
      Chunk* chunk_sink = &chunk_graph->instances.array[j];

      // Assign dependencies between chunks via their instances by going
      // through the dependency of instances inside the chunk and carry them
      // over to chunk dependency graph
      for (k = 0; k < chunk_source->instances.length; k++) {
        INSTANCE* instance_source = &chunk_source->instances.array[k];

        for (l = 0; l < chunk_sink->instances.length; l++) {
          INSTANCE* instance_sink = &chunk_sink->instances.array[l];

          if (MERGED_CONDITION_IS_IMPOSSIBLE(
                  instance_condition(instance_source),
                  instance_condition(instance_sink)))
            continue;

          DEPENDENCY old_dep =
              chunk_graph->graph[i * chunk_graph->instances.length + j];

          DEPENDENCY new_dep = edgeset_kind(
              aug_graph
                  ->graph[instance_source->index * aug_graph->instances.length +
                          instance_sink->index]);

          if (new_dep) {
            if ((oag_debug & DEBUG_ORDER) &&
                (oag_debug & DEBUG_ORDER_VERBOSE)) {
              print_indent(2, stdout);
              printf("Regular dependency from chunk %d (", i);
              print_chunk_info(chunk_source);
              printf(") to chunk %d (", j);
              print_chunk_info(chunk_sink);
              printf(") because of:\n");
              print_indent(4, stdout);

              print_instance(instance_source, stdout);
              printf(" <%+d,%+d> -> ",
                     state->instance_groups[instance_source->index].ph,
                     state->instance_groups[instance_source->index].ch);
              print_instance(instance_sink, stdout);
              printf(" <%+d,%+d>  (kind=%d)\n",
                     state->instance_groups[instance_sink->index].ph,
                     state->instance_groups[instance_sink->index].ch, new_dep);
            }

            chunk_graph->graph[i * chunk_graph->instances.length + j] =
                dependency_join(new_dep, old_dep);
          }
        }
      }

      // Assign dependency between visits of the same children by adding edge
      // from chunk visit (ph,ch) and (ph+1,ch)
      if (chunk_source->type == Visit && chunk_sink->type == Visit &&
          chunk_source->ch == chunk_sink->ch &&
          chunk_source->ph < chunk_sink->ph) {
        DEPENDENCY old_dep =
            chunk_graph->graph[i * chunk_graph->instances.length + j];

        chunk_graph->graph[i * chunk_graph->instances.length + j] =
            dependency_join(old_dep, CHUNK_GUIDING_DEPENDENCY);

        if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
          // Write debugging if its a new edge
          print_indent(2, stdout);
          printf("Guiding dependency from chunk %d (", i);
          print_chunk_info(chunk_source);
          printf(") to chunk %d (", j);
          print_chunk_info(chunk_sink);
          printf(")\n");
        }
      }

      // Assign dependency between parent inherited and parent synthesized
      // attributes of the parent for the same phase
      if (chunk_source->type == HalfLeft && chunk_sink->type == HalfRight &&
          chunk_source->ph == chunk_sink->ph) {
        DEPENDENCY old_dep =
            chunk_graph->graph[i * chunk_graph->instances.length + j];

        chunk_graph->graph[i * chunk_graph->instances.length + j] =
            dependency_join(old_dep, CHUNK_GUIDING_DEPENDENCY);

        if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
          // Write debugging if its a new edge
          print_indent(2, stdout);
          printf("Guiding dependency from chunk %d (", i);
          print_chunk_info(chunk_source);
          printf(") to chunk %d (", j);
          print_chunk_info(chunk_sink);
          printf(")\n");
        }
      }

      // Assign dependency between synthesized/inherited attributes of the
      // parent across different phases to make sure parent visits are
      // sequential
      if (chunk_source->type == HalfRight && chunk_sink->type == HalfLeft &&
          chunk_source->ph + 1 == chunk_sink->ph) {
        DEPENDENCY old_dep =
            chunk_graph->graph[i * chunk_graph->instances.length + j];

        chunk_graph->graph[i * chunk_graph->instances.length + j] =
            dependency_join(old_dep, CHUNK_GUIDING_DEPENDENCY);

        if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
          print_indent(2, stdout);
          printf("Guiding dependency from chunk %d (", i);
          print_chunk_info(chunk_source);
          printf(") to chunk %d (", j);
          print_chunk_info(chunk_sink);
          printf(")\n");
        }
      }
    }
  }

  // Transitive closure
  bool changed = false;
  do {
    changed = false;
    for (i = 0; i < chunk_graph->instances.length; i++) {
      for (j = 0; j < chunk_graph->instances.length; j++) {
        for (k = 0; k < chunk_graph->instances.length; k++) {
          // i->j && j->k then i->k
          DEPENDENCY dep_ij =
              chunk_graph->graph[i * chunk_graph->instances.length + j];
          DEPENDENCY dep_jk =
              chunk_graph->graph[j * chunk_graph->instances.length + k];
          DEPENDENCY old_dep_ik =
              chunk_graph->graph[i * chunk_graph->instances.length + k];
          DEPENDENCY new_dep_ik = dependency_trans(dep_ij, dep_jk);

          if (dep_ij && dep_jk && (old_dep_ik | new_dep_ik) != old_dep_ik) {
            changed = true;
            chunk_graph->graph[i * chunk_graph->instances.length + k] =
                dependency_join(new_dep_ik, old_dep_ik);
          }
        }
      }
    }
  } while (changed);

  SccGraph scc_graph;
  scc_graph_initialize(&scc_graph, chunk_graph->instances.length);

  // Add vertices to SCC graph
  for (i = 0; i < chunk_graph->instances.length; i++) {
    Chunk* chunk = &chunk_graph->instances.array[i];
    scc_graph_add_vertex(&scc_graph, (void*)chunk);
  }

  // Add edges to SCC graph
  for (i = 0; i < chunk_graph->instances.length; i++) {
    Chunk* chunk_source = &chunk_graph->instances.array[i];
    for (j = 0; j < chunk_graph->instances.length; j++) {
      Chunk* chunk_sink = &chunk_graph->instances.array[j];
      if (chunk_graph->graph[i * chunk_graph->instances.length + j]) {
        scc_graph_add_edge(&scc_graph, (void*)chunk_source, (void*)chunk_sink);
      }
    }
  }

  SCC_COMPONENTS* components = scc_graph_components(&scc_graph);
  if (oag_debug & DEBUG_ORDER) {
    printf("Components of chunks of aug_graph: %s\n",
           aug_graph_name(aug_graph));

    for (i = 0; i < components->length; i++) {
      SCC_COMPONENT* comp = components->array[i];

      printf("=> component #%d\n", i);

      for (j = 0; j < comp->length; j++) {
        Chunk* chunk = (Chunk*)comp->array[j];

        print_indent(4, stdout);
        printf("(%d) ", chunk->index);

        if (oag_debug & DEBUG_ORDER_VERBOSE) {
          print_chunk(aug_graph, state, chunk, 0);
        } else {
          print_chunk_info(chunk);
          printf("\n");
        }
      }
    }
  }

  chunk_graph->chunk_components = components;

  return chunk_graph;
}

/**
 * @brief utility function to free the memory allocated for chunk graph
 * @param chunk_graph chunk graph struct
 */
static void free_chunk_graph(ChunkGraph* chunk_graph) {
  free(chunk_graph->graph);
  free(chunk_graph->schedule);
  free(chunk_graph);
}

/**
 * Utility function to schedule augmented dependency graph
 * @param aug_graph Augmented dependency graph
 * @param original_state_dependency Original state dependency
 */
static void schedule_augmented_dependency_graph(
    AUG_GRAPH* aug_graph,
    DEPENDENCY original_state_dependency) {
  int n = aug_graph->instances.length;
  CONDITION cond;
  int i, j, ch;

  (void)close_augmented_dependency_graph(aug_graph);

  // Find SCC components of instances given a augmented dependency graph
  set_aug_graph_components(aug_graph);

  for (i = 0; i < aug_graph->components->length; i++) {
    if (original_state_dependency == no_dependency &&
        aug_graph->component_cycle[i]) {
      fatal_error(
          "The scheduler cannot handle the AG (%s) since it has a "
          "conditional cycle.",
          aug_graph_name(aug_graph));
    }
  }

  // Now schedule graph: we need to generate a conditional total order.
  if (oag_debug & PROD_ORDER) {
    printf("Scheduling conditional total order for %s\n",
           aug_graph_name(aug_graph));
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
      if (in->node == chdecl) {
        ch = j;
      }
      j++;
    }

    if (in->node == aug_graph->lhs_decl || ch >= 0) {
      PHY_GRAPH* npg = DECL_PHY_GRAPH(in->node);
      int ph = attribute_schedule(npg, &(in->fibered_attr));
      instance_groups[i].ph = (short)ph;
      instance_groups[i].ch = (short)ch;
    }
  }

  state->instance_groups = instance_groups;

  // Find children of augmented graph: this will be used as argument to
  // visit calls
  set_aug_graph_children(aug_graph, state);

  size_t schedule_size = n * sizeof(int);
  aug_graph->schedule = (int*)alloca(schedule_size);

  // False here means nothing is scheduled yet
  memset(aug_graph->schedule, 0, schedule_size);

  if (oag_debug & DEBUG_ORDER) {
    printf("\nInstances %s:\n", aug_graph_name(aug_graph));
    for (i = 0; i < n; i++) {
      INSTANCE* in = &(aug_graph->instances.array[i]);
      CHILD_PHASE group = state->instance_groups[i];
      print_instance(in, stdout);
      printf(":index: %d lineno: %d ", in->index,
             tnode_line_number(in->fibered_attr.attr));

      int cycle = instance_in_aug_cycle(aug_graph, in);
      if (cycle > -1) {
        printf(" [in cycle:%d] ", cycle);
      } else {
        printf(" [non-circular] ");
      }

      if (!group.ph && !group.ch) {
        printf("local\n");
      } else {
        printf("<%+d,%+d>\n", group.ph, group.ch);
      }
    }
  }

  ChunkGraph* chunk_graph = collect_aug_graph_chunks(aug_graph, state);

  cond.negative = 0;
  cond.positive = 0;

  aug_graph->total_order =
      schedule_visit_start(aug_graph, chunk_graph, NULL, cond, state, n,
                           1 /* parent visit number */);

  if (aug_graph->total_order == NULL) {
    fatal_error("Failed to create total order.");
  }

  if (oag_debug & DEBUG_ORDER) {
    printf("\nSchedule for %s (%d children):\n", aug_graph_name(aug_graph),
           state->children.length);
    print_total_order(aug_graph, aug_graph->total_order, -1, false, state, 0,
                      stdout);
  }

  // Ensure generated total order is valid
  assert_total_order(aug_graph, chunk_graph, state, aug_graph->total_order);

  free_chunk_graph(chunk_graph);
}

/**
 * @brief Computes total-preorder of set of attributes
 * @param s state
 */
void compute_static_schedule(STATE* s) {
  int i, j;
  for (i = 0; i < s->phyla.length; i++) {
    schedule_summary_dependency_graph(&s->phy_graphs[i]);

    dnc_close(s);

    /* now perform closure */
    int saved_analysis_debug = analysis_debug;

    if (oag_debug & TYPE_3_DEBUG) {
      analysis_debug |= TWO_EDGE_CYCLE;
    }

    if (analysis_debug & DNC_ITERATE) {
      printf("\n**** After OAG schedule for phylum %d:\n\n", i);
    }

    if (analysis_debug & ASSERT_CLOSED) {
      for (j = 0; j < s->match_rules.length; j++) {
        printf("Checking rule %d\n", j);
        assert_closed(&s->aug_graphs[j]);
      }
    }

    analysis_debug = saved_analysis_debug;

    if (analysis_debug & DNC_ITERATE) {
      printf("\n*** After closure after schedule OAG phylum %d\n\n", i);
      print_analysis_state(s, stdout);
      print_cycles(s, stdout);
    }
  }

  DEPENDENCY dep = analysis_state_cycle(s);

  for (i = 0; i < s->match_rules.length; i++) {
    schedule_augmented_dependency_graph(&s->aug_graphs[i],
                                        s->original_state_dependency);
  }
  schedule_augmented_dependency_graph(&s->global_dependencies,
                                      s->original_state_dependency);

  if (analysis_debug & (DNC_ITERATE | DNC_FINAL)) {
    printf("*** FINAL OAG ANALYSIS STATE ***\n");
    print_analysis_state(s, stdout);
    print_cycles(s, stdout);
  }
}
