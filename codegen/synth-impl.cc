#include <string.h>
#include <cstddef>

#include <algorithm>
#include <iostream>
#include <numeric>
extern "C" {
#include <stdio.h>

#include "aps-ag.h"
}
#include <functional>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

#include "dump.h"
#include "implement.h"

#define LOCAL_VALUE_FLAG (1 << 28)

#ifdef APS2SCALA
#define DEREF "."
#else
#define DEREF "->"
#endif

// visit procedures are called:
// visit_n_m
// where n is the number of the phy_graph and m is the phase.
// This does a dispatch to visit_n_m_p
// where p is the production number (0-based constructor index)
#define PHY_GRAPH_NUM(pg) (pg - pg->global_state->phy_graphs)

#define KEY_BLOCK_ITEM_CONDITION 1
#define KEY_BLOCK_ITEM_INSTANCE 2

struct block_item_base {
  int key;
  INSTANCE* instance;
  struct block_item_base* prev;
};

typedef struct block_item_base BlockItem;

struct block_item_condition {
  int key; /* KEY_BLOCK_ITEM_CONDITION */
  INSTANCE* instance;
  BlockItem* prev;
  Declaration condition;
  BlockItem* next_positive;
  BlockItem* next_negative;
};

struct block_item_instance {
  int key; /* KEY_BLOCK_ITEM_INSTANCE */
  INSTANCE* instance;
  BlockItem* prev;
  BlockItem* next;
};

vector<Block> current_blocks;
BlockItem* current_scope_block;

// Given a block, it prints it linearized schedule.
static void print_linearized_block(BlockItem* block) {
  if (block != NULL) {
    printf("%s", indent().c_str());
    print_instance(block->instance, stdout);
    printf(" %d", block->instance->index);
    printf("\n");
    if (block->key == KEY_BLOCK_ITEM_CONDITION) {
      struct block_item_condition* cond = (struct block_item_condition*)block;
      printf("%spositive\n", indent().c_str());
      nesting_level++;
      print_linearized_block(cond->next_positive);
      nesting_level--;
      printf("%snegative\n", indent().c_str());
      nesting_level++;
      print_linearized_block(cond->next_negative);
      nesting_level--;
    } else {
      print_linearized_block(((struct block_item_instance*)block)->next);
    }
  }
}

static vector<INSTANCE*> sort_instances(AUG_GRAPH* aug_graph) {
  vector<INSTANCE*> result;

  int n = aug_graph->instances.length;
  int i;
  for (i = 0; i < n; i++) {
    INSTANCE* instance = &aug_graph->instances.array[i];
    if (!if_rule_p(instance->fibered_attr.attr)) {
      result.push_back(instance);
    }
  }

  for (i = 0; i < n; i++) {
    INSTANCE* instance = &aug_graph->instances.array[i];
    if (if_rule_p(instance->fibered_attr.attr)) {
      result.push_back(instance);
    }
  }

  return result;
}

// Given an augmented dependency graph, it linearize it recursively
static BlockItem* linearize_block_helper(AUG_GRAPH* aug_graph,
                                         vector<INSTANCE*> sorted_instances,
                                         bool* scheduled,
                                         CONDITION* cond,
                                         BlockItem* prev,
                                         int remaining) {
  // impossible merge condition
  if (CONDITION_IS_IMPOSSIBLE(*cond)) {
    return NULL;
  }

  int i, j;
  int n = aug_graph->instances.length;

  for (auto it = sorted_instances.begin(); it != sorted_instances.end(); it++) {
    INSTANCE* instance = *it;
    int i = instance->index;

    if (scheduled[i])
      continue;

    // impossible merge condition, cannot schedule this instance
    if (MERGED_CONDITION_IS_IMPOSSIBLE(*cond, instance_condition(instance))) {
      scheduled[i] = true;
      BlockItem* result =
          linearize_block_helper(aug_graph, sorted_instances, scheduled, cond, prev, remaining - 1);
      scheduled[i] = false;
      return result;
    }

    // printf("trying to schedule: ");
    // print_instance(instance, stdout);
    // printf("\n");

    bool ready_to_schedule = true;
    for (j = 0; j < n && ready_to_schedule; j++) {
      INSTANCE* other_instance = &aug_graph->instances.array[j];

      // printf("checking dependency: ");
      // print_instance(other_instance, stdout);
      // printf("\n");

      // already scheduled dependency
      if (scheduled[j]) {
        // printf("already scheduled\n");
        continue;
      }

      // impossible merge condition, ignore this dependency
      if (MERGED_CONDITION_IS_IMPOSSIBLE(instance_condition(instance), instance_condition(other_instance))) {
        // printf("impossible merge condition\n");
        continue;
      }

      // not a direct dependency
      if (!(edgeset_kind(aug_graph->graph[j * n + i]) & DEPENDENCY_MAYBE_DIRECT)) {
        // printf("not a direct dependency\n");
        continue;
      }

      ready_to_schedule = false;
      break;
    }

    // if all dependencies are ready to schedule
    if (!ready_to_schedule) {
      continue;
    }

    // printf("scheduling: ");
    // print_instance(instance, stdout);
    // printf("\n\n");

    BlockItem* item_base;
    scheduled[i] = true;

    if (if_rule_p(instance->fibered_attr.attr)) {
      struct block_item_condition* item =
          (struct block_item_condition*)malloc(sizeof(struct block_item_condition));
      item_base = (BlockItem*)item;

      item->key = KEY_BLOCK_ITEM_CONDITION;
      item->instance = instance;
      item->condition = instance->fibered_attr.attr;
      item->prev = prev;

      int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
      cond->positive |= cmask;
      item->next_positive =
          linearize_block_helper(aug_graph, sorted_instances, scheduled, cond, item_base, remaining - 1);
      cond->positive &= ~cmask;
      cond->negative |= cmask;
      item->next_negative =
          linearize_block_helper(aug_graph, sorted_instances, scheduled, cond, item_base, remaining - 1);
      cond->negative &= ~cmask;
    } else {
      struct block_item_instance* item =
          (struct block_item_instance*)malloc(sizeof(struct block_item_instance));
      item_base = (BlockItem*)item;
      item->key = KEY_BLOCK_ITEM_INSTANCE;
      item->instance = instance;
      item->prev = prev;
      item->next =
          linearize_block_helper(aug_graph, sorted_instances, scheduled, cond, item_base, remaining - 1);
    }

    scheduled[i] = false;

    return item_base;
  }

  if (remaining != 0) {
    fatal_error("failed to schedule some instances, remaining: %d", remaining);
  }

  return NULL;
}

// Given an augmented dependency graph, it linearizes
// the direct dependency schedule.
static BlockItem* linearize_block(AUG_GRAPH* aug_graph) {
  int n = aug_graph->instances.length;
  bool* scheduled = (bool*)alloca(sizeof(bool) * n);
  memset(scheduled, 0, sizeof(bool) * n);

  CONDITION cond = {0, 0};
  vector<INSTANCE*> sorted_instances = sort_instances(aug_graph);

  return linearize_block_helper(aug_graph, sorted_instances, scheduled, &cond, NULL, n);
}

// Given an instance it traverses the direct dependency schedule
// trying to find the instance and if it sees the condition
// along the way, it returns that condition.
static BlockItem* find_surrounding_block(BlockItem* block, INSTANCE* instance) {
  while (block != NULL) {
    if (block->key == KEY_BLOCK_ITEM_CONDITION) {
      return block;
    } else if (block->key == KEY_BLOCK_ITEM_INSTANCE) {
      if (block->instance == instance) {
        return block;
      } else {
        block = ((struct block_item_instance*)block)->next;
      }
    }
  }

  return NULL;
}

enum instance_direction custom_instance_direction(INSTANCE* i) {
  enum instance_direction dir = fibered_attr_direction(&i->fibered_attr);
  if (i->node == NULL) {
    return dir;
  } else if (DECL_IS_LHS(i->node)) {
    return dir;
  } else if (DECL_IS_RHS(i->node)) {
    return dir;
  } else if (DECL_IS_LOCAL(i->node)) {
    return instance_local;
  } else {
    fatal_error("%d: unknown attributed node", tnode_line_number(i->node));
    return dir; /* keep CC happy */
  }
}

static bool instance_is_local(INSTANCE* instance) {
  if (instance->fibered_attr.fiber == NULL) {
    return custom_instance_direction(instance) == instance_local;
  } else {
    return fibered_attr_direction(&instance->fibered_attr) == instance_local;
  }
}

static bool instance_is_synthesized(INSTANCE* instance) {
  if (instance->fibered_attr.fiber == NULL) {
    return custom_instance_direction(instance) == instance_outward;
  } else {
    return fibered_attr_direction(&instance->fibered_attr) == instance_outward;
  }
}

static bool instance_is_inherited(INSTANCE* instance) {
  if (instance->fibered_attr.fiber == NULL) {
    return custom_instance_direction(instance) == instance_inward;
  } else {
    return fibered_attr_direction(&instance->fibered_attr) == instance_inward;
  }
}

static bool instance_is_pure_shared_info(INSTANCE* instance) {
  return instance->fibered_attr.fiber == NULL && ATTR_DECL_IS_SHARED_INFO(instance->fibered_attr.attr);
}

static std::vector<INSTANCE*> collect_phylum_graph_attr_dependencies(PHY_GRAPH* phylum_graph,
                                                                     INSTANCE* sink_instance) {
  std::vector<INSTANCE*> result;

  int i;
  int n = phylum_graph->instances.length;

  for (i = 0; i < n; i++) {
    INSTANCE* source_instance = &phylum_graph->instances.array[i];
    if (!instance_is_pure_shared_info(source_instance) && source_instance->index != sink_instance->index &&
        phylum_graph->mingraph[source_instance->index * n + sink_instance->index]) {
      result.push_back(source_instance);
    }
  }

  return result;
}

static bool is_function_decl_attribute(INSTANCE* instance) {
  if (instance->node != NULL && ABSTRACT_APS_tnode_phylum(instance->node) == KEYDeclaration) {
    switch (Declaration_KEY(instance->node)) {
      case KEYpragma_call: {
        Declaration fdecl = Declaration_info(instance->node)->proxy_fdecl;
        switch (Declaration_KEY(fdecl)) {
          case KEYfunction_decl:
            return fdecl;
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
  }

  return false;
}

static std::vector<INSTANCE*> collect_aug_graph_attr_dependencies(AUG_GRAPH* aug_graph,
                                                                  INSTANCE* sink_instance) {
  std::vector<INSTANCE*> result;

  int i;
  int n = aug_graph->instances.length;

  for (i = 0; i < n; i++) {
    INSTANCE* source_instance = &aug_graph->instances.array[i];
    if (!instance_is_pure_shared_info(source_instance) && source_instance->index != sink_instance->index &&
        !is_function_decl_attribute(source_instance) &&
        edgeset_kind(aug_graph->graph[source_instance->index * n + sink_instance->index])) {
      result.push_back(source_instance);
    }
  }

  return result;
}

// only works for strict
// APS fiber uses untracked, do this untracked
static std::vector<INSTANCE*> collect_fiber_dependencies_to_finish(AUG_GRAPH* aug_graph,
                                                                   INSTANCE* source_instance) {
  std::vector<INSTANCE*> result;
  int i;
  int n = aug_graph->instances.length;
  for (i = 0; i < n; i++) {
    INSTANCE* sink_instance = &aug_graph->instances.array[i];
    if (sink_instance->node == source_instance->node &&
        sink_instance->fibered_attr.attr == source_instance->fibered_attr.attr &&
        sink_instance->fibered_attr.fiber != NULL &&
        // sink_instance->fibered_attr.fiber->shorter == base_fiber &&
        fiber_is_reverse(sink_instance->fibered_attr.fiber)) {
      auto dependencies = collect_aug_graph_attr_dependencies(aug_graph, sink_instance);
      result.insert(result.end(), dependencies.begin(), dependencies.end());
    }
  }

  return result;
}

static vector<AUG_GRAPH*> collect_lhs_aug_graphs(STATE* state, PHY_GRAPH* pgraph) {
  vector<AUG_GRAPH*> result;

  int i;
  int n = state->match_rules.length;
  for (i = 0; i < n; i++) {
    AUG_GRAPH* aug_graph = &state->aug_graphs[i];
    PHY_GRAPH* aug_graph_pgraph = Declaration_info(aug_graph->lhs_decl)->node_phy_graph;

    if (aug_graph_pgraph == pgraph) {
      switch (Declaration_KEY(aug_graph->lhs_decl)) {
        case KEYsome_function_decl:
          continue;
          break;
        default:
          break;
      }

      result.push_back(aug_graph);
    }
  }

  return result;
}

static bool find_instance(AUG_GRAPH* aug_graph,
                          Declaration node,
                          FIBERED_ATTRIBUTE& fiber_attr,
                          INSTANCE** instance_out) {
  int i;
  for (i = 0; i < aug_graph->instances.length; i++) {
    INSTANCE* instance = &aug_graph->instances.array[i];
    if (instance->node == node && instance->fibered_attr.attr == fiber_attr.attr) {
      if (fibered_attr_equal(&instance->fibered_attr, &fiber_attr)) {
        *instance_out = instance;
        return true;
      }
    }
  }

  return false;
}

string attr_to_string(Declaration attr) {
  if (ATTR_DECL_IS_SHARED_INFO(attr)) {
    return "sharedinfo";
  } else {
    return decl_name(attr);
  }
}

string fiber_to_string(FIBER fiber) {
  std::stringstream ss;

  while (fiber != NULL && fiber->field != NULL) {
    std::string field = decl_name(fiber->field);
    size_t pos = field.find("!");
    if (pos != std::string::npos) {
      field.replace(pos, 1, "X");  // Replace "!" with X
    }

    ss << field;
    fiber = fiber->shorter;
    if (fiber->field != NULL) {
      ss << "_";
    }
  }

  return ss.str();
}

string instance_to_string(INSTANCE* in, bool force_include_node_type = false, bool trim_node = false) {
  vector<string> result;

  Declaration node = in->node;
  if (force_include_node_type && node == NULL) {
    node = current_aug_graph->lhs_decl;
  }

  if (!trim_node && node != NULL) {
    if (Declaration_KEY(node) == KEYpragma_call) {
      result.push_back(symbol_name(pragma_call_name(node)));
    } else {
      result.push_back(decl_name(node));
    }
  }

  if (in->fibered_attr.attr != NULL) {
    result.push_back(attr_to_string(in->fibered_attr.attr));
  }

  if (in->fibered_attr.fiber != NULL) {
    result.push_back(fiber_to_string(in->fibered_attr.fiber));
  }

  return std::accumulate(std::next(result.begin()), result.end(), result[0],
                         [](std::string a, std::string b) { return a + "_" + b; });
}

static string instance_to_string_with_nodetype(Declaration polymorphic, INSTANCE* in) {
  Declaration attr = in->fibered_attr.attr;
  std::stringstream ss;

  if (Declaration_KEY(attr) == KEYvalue_decl && LOCAL_UNIQUE_PREFIX(attr) != 0) {
    ss << "a" << LOCAL_UNIQUE_PREFIX(attr) << "_";
  }

  ss << decl_name(polymorphic) << "_" << instance_to_string(in, false, false);

  return ss.str();
}

static string instance_to_attr(INSTANCE* in) {
  Declaration attr = in->fibered_attr.attr;
  std::stringstream ss;

  if (Declaration_KEY(attr) == KEYvalue_decl && LOCAL_UNIQUE_PREFIX(attr) != 0) {
    ss << "a" << LOCAL_UNIQUE_PREFIX(attr) << "_";
  } else {
    ss << "a_";
  }

  ss << in;

  return ss.str();
}

static std::vector<SYNTH_FUNCTION_STATE*> build_synth_functions_state(STATE* s) {
  std::vector<SYNTH_FUNCTION_STATE*> synth_function_states;
  int i, j, k;
  int aug_graph_count = s->match_rules.length;

  for (i = 0; i < s->phyla.length; i++) {
    PHY_GRAPH* pg = &s->phy_graphs[i];
    if (Declaration_KEY(pg->phylum) != KEYphylum_decl) {
      continue;
    }

    for (j = 0; j < pg->instances.length; j++) {
      INSTANCE* in = &pg->instances.array[j];
      bool is_fiber = in->fibered_attr.fiber != NULL;
      bool is_shared_info = ATTR_DECL_IS_SHARED_INFO(in->fibered_attr.attr);
      bool is_pure_shareinfo = !is_fiber && is_shared_info;

      if (!instance_is_synthesized(in)) {
        printf("Skipping instance ");
        print_instance(in, stdout);
        printf("\n");
        continue;
      }

      SYNTH_FUNCTION_STATE* state = new SYNTH_FUNCTION_STATE();
      state->fdecl_name = instance_to_string_with_nodetype(pg->phylum, in);
      state->source = in;
      state->is_phylum_instance = true;
      state->source_phy_graph = pg;
      state->is_fiber_evaluation = is_fiber || is_shared_info;
      state->regular_dependencies = collect_phylum_graph_attr_dependencies(pg, in);
      state->aug_graphs = collect_lhs_aug_graphs(s, pg);

      synth_function_states.push_back(state);
    }
  }

  for (i = 0; i < aug_graph_count; i++) {
    AUG_GRAPH* aug_graph = &s->aug_graphs[i];

    switch (Declaration_KEY(aug_graph->lhs_decl)) {
      case KEYsome_function_decl:
        continue;
        break;
      default:
        break;
    }

    for (j = 0; j < aug_graph->instances.length; j++) {
      INSTANCE* instance = &aug_graph->instances.array[j];
      bool is_local = instance_direction(instance) == instance_local;
      bool is_fiber = instance->fibered_attr.fiber != NULL;
      bool is_shared_info = ATTR_DECL_IS_SHARED_INFO(instance->fibered_attr.attr);

      if (is_local && !is_fiber && !if_rule_p(instance->fibered_attr.attr)) {
        switch (Declaration_KEY(instance->fibered_attr.attr)) {
          case KEYformal:
            continue;
            break;
          default:
            break;
        }

        Declaration attr = instance->fibered_attr.attr;
        PHY_GRAPH* pg = Declaration_info(aug_graph->lhs_decl)->node_phy_graph;
        Declaration tdecl = canonical_type_decl(canonical_type(value_decl_type(attr)));

        SYNTH_FUNCTION_STATE* state = new SYNTH_FUNCTION_STATE();
        state->fdecl_name = instance_to_string_with_nodetype(tdecl, instance);
        state->source = instance;
        state->is_phylum_instance = false;
        state->source_phy_graph = pg;
        state->is_fiber_evaluation = is_fiber || is_shared_info;
        state->regular_dependencies = collect_aug_graph_attr_dependencies(aug_graph, instance);

        vector<AUG_GRAPH*> aug_graphs;
        aug_graphs.push_back(aug_graph);
        state->aug_graphs = aug_graphs;

        synth_function_states.push_back(state);
      }
    }
  }

  for (auto it = synth_function_states.begin(); it != synth_function_states.end(); it++) {
    SYNTH_FUNCTION_STATE* synth_functions_state = *it;
    printf("Synth function: %s\n", synth_functions_state->fdecl_name.c_str());
    printf("  Source: ");
    print_instance(synth_functions_state->source, stdout);
    printf("\n");
    printf("  Is phylum instance: %s\n", synth_functions_state->is_phylum_instance ? "true" : "false");
    printf("  Is fiber evaluation: %s\n", synth_functions_state->is_fiber_evaluation ? "true" : "false");
    printf("  Formals:\n");
    printf("  Aug graphs:\n");
    for (auto it = synth_functions_state->aug_graphs.begin(); it != synth_functions_state->aug_graphs.end(); it++) {
      printf("    %s (%d)\n", aug_graph_name(*it), Declaration_KEY((*it)->lhs_decl));
    }

    printf("  Regular dependencies:\n");
    for (auto it = synth_functions_state->regular_dependencies.begin();
         it != synth_functions_state->regular_dependencies.end(); it++) {
      printf("    ");
      print_instance(*it, stdout);
      printf("\n");
    }
    printf("\n");
  }

  return synth_function_states;
}

static void destroy_synth_function_states(vector<SYNTH_FUNCTION_STATE*> states) {
  for (auto it = states.begin(); it != states.end(); it++) {
    delete (*it);
  }
}

static void dump_attribute_type(INSTANCE* in, ostream& os) {
  CanonicalType* ctype = canonical_type(some_value_decl_type(in->fibered_attr.attr));
  switch (ctype->key) {
    case KEY_CANONICAL_USE: {
      os << "T_" << decl_name(canonical_type_decl(ctype));
      break;
    }
    case KEY_CANONICAL_FUNC: {
      struct Canonical_function_type* fdecl_ctype = (struct Canonical_function_type*)ctype;
      os << "T_" << decl_name(canonical_type_decl(fdecl_ctype->return_type));
      break;
    }
    default:
      break;
  }
}

template <typename Callable>
auto dump_fiber_attribute_assignment(INSTANCE* instance, string node, Callable func, ostream& os)
    -> decltype(func()) {
  char* field = decl_name(instance->fibered_attr.fiber->field);

  std::string s = field;
  s.erase(std::remove(s.begin(), s.end(), '!'), s.end());

  os << "a_" << s << DEREF;

  if (debug) {
    os << "assign";
  } else {
    os << "set";
  }

  os << "(";

  if (node != "") {
    os << node << ", ";
  }

  bool result = func();

  os << ")";

  return result;
}

static void dump_fiber_dependencies_recursively_helper(
  AUG_GRAPH* aug_graph,
  vector<INSTANCE*> relevant_instances,
  int count_relevant_instances,
  bool* scheduled,
  int& count_scheduled,
  ostream& os) {

  int i;
  int n = aug_graph->instances.length;

  if (count_scheduled == count_relevant_instances) {
    return;
  }

  for (auto it1 = relevant_instances.begin(); it1 != relevant_instances.end(); it1++) {
    INSTANCE* in = *it1;
    if (scheduled[in->index]) {
      continue;
    }

    bool dependency_ready = true;
    for (auto it2 = relevant_instances.begin(); it2 != relevant_instances.end(); it2++) {
      INSTANCE* dependency_instance = *it2;

      if (scheduled[dependency_instance->index] || !edgeset_kind(aug_graph->graph[dependency_instance->index * n + in->index])) {
        continue;
      }

      dependency_ready = false;
    }

    if (!dependency_ready) {
      continue;
    }

    scheduled[in->index] = true;
    os << indent();
    impl->dump_synth_instance(in, os);
    os << "\n";
    count_scheduled++;
    dump_fiber_dependencies_recursively_helper(aug_graph, relevant_instances, count_relevant_instances, scheduled, count_scheduled, os);
  }

  if (count_scheduled != count_relevant_instances) {
    fatal_error("failed to find next dependency to schedule count_scheduled: %d, count_relevant_instances: %d", count_scheduled, count_relevant_instances);
    return;
  }
}

static void dump_fiber_dependencies_recursively(AUG_GRAPH* aug_graph, INSTANCE* sink, ostream& os) {
  int i;
  int n = aug_graph->instances.length;

  int count = 0;
  vector<INSTANCE*> relevant_instances;

  for (i = 0; i < n; i++) {
    INSTANCE* in = &aug_graph->instances.array[i];

    if (in->node != NULL && Declaration_KEY(in->node) == KEYpragma_call) {
      continue;
    }

    if (edgeset_kind(aug_graph->graph[in->index * n + sink->index])) {
      if (in->fibered_attr.fiber != NULL) {
        if (instance_is_synthesized(in) || instance_is_local(in)) {
          relevant_instances.push_back(in);
          count++;
        }
      }
    }
  }

  bool* scheduled = (bool*)alloca(sizeof(bool) * n);
  memset(scheduled, 0, sizeof(bool) * n);

  int count_scheduled = 0;
  dump_fiber_dependencies_recursively_helper(aug_graph, relevant_instances, count, scheduled, count_scheduled, os);
}

#ifdef APS2SCALA
static void dump_synth_functions(STATE* s, ostream& os)
#else  /* APS2SCALA */
static void dump_synth_functions(STATE* s, output_streams& oss)
#endif /* APS2SCALA */
{
#ifdef APS2SCALA
  ostream& oss = os;
#else /* !APS2SCALA */
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  ostream& os = inline_definitions ? hs : cpps;

#endif /* APS2SCALA */
  // first dump all visit functions for each phylum:

  os << "\n";

  int i, j, k;
  int aug_graph_count = s->match_rules.length;
  current_state = s;
  synth_functions_states = build_synth_functions_state(s);

  for (auto state_it = synth_functions_states.begin(); state_it != synth_functions_states.end(); state_it++) {
    SYNTH_FUNCTION_STATE* synth_functions_state = *state_it;

    os << indent() << "// " << synth_functions_state->source << " ("
       << (synth_functions_state->is_phylum_instance ? "phylum" : "auggraph") << " "
       << synth_functions_state->source->index << ")\n";
    if (synth_functions_state->is_fiber_evaluation) {
      os << indent() << "val evaluated_map_" << synth_functions_state->fdecl_name
         << " = scala.collection.mutable.Map[T_" << decl_name(synth_functions_state->source_phy_graph->phylum)
         << ", Boolean]()"
         << "\n\n";
    }

    os << indent() << "def eval_" << synth_functions_state->fdecl_name << "(";
    os << "node: T_" << decl_name(synth_functions_state->source_phy_graph->phylum);

    for (auto it = synth_functions_state->regular_dependencies.begin();
         it != synth_functions_state->regular_dependencies.end(); it++) {
      INSTANCE* source_instance = *it;
      if (source_instance->fibered_attr.fiber != NULL) {
        continue;
      }

      if (if_rule_p(source_instance->fibered_attr.attr)) {
        continue;
      }

      os << ", " << "/*" << source_instance << " (#" << source_instance->index << ") " << "*/ " << "v_"
         << instance_to_string(source_instance, false, true) << ": ";
      dump_attribute_type(source_instance, os);
    }

    os << "): ";
    if (synth_functions_state->is_fiber_evaluation) {
      os << "Unit";
    } else {
      dump_attribute_type(synth_functions_state->source, os);
    }
    os << " = {\n";
    nesting_level++;

    if (synth_functions_state->is_fiber_evaluation) {
      os << indent() << "evaluated_map_" << synth_functions_state->fdecl_name
         << ".getOrElse(node, false) match {\n";
      os << indent(nesting_level + 1) << "case true => ";
      os << "return ()\n";
    } else {
      os << indent() << instance_to_attr(synth_functions_state->source)
         << ".checkNode(node).status match {\n";
      os << indent(nesting_level + 1) << "case Evaluation.ASSIGNED => ";
      os << "return " << instance_to_attr(synth_functions_state->source) << ".get(node)\n";
    }

    os << indent(nesting_level + 1) << "case _ => ()\n";
    os << indent() << "};\n";

    if (synth_functions_state->is_fiber_evaluation) {
      os << indent() << "node match {\n";
    } else {
      os << indent() << "val result = node match {\n";
    }
    nesting_level++;

    for (auto it = synth_functions_state->aug_graphs.begin(); it != synth_functions_state->aug_graphs.end(); it++) {
      AUG_GRAPH* aug_graph = *it;
      int n = aug_graph->instances.length;

      current_aug_graph = aug_graph;
      current_blocks.push_back(matcher_body(top_level_match_m(aug_graph->match_rule)));
      current_scope_block = linearize_block(aug_graph);

      os << indent() << "case " << matcher_pat(top_level_match_m(aug_graph->match_rule)) << " => {\n";
      nesting_level++;

      INSTANCE* aug_graph_instance = NULL;
      if (synth_functions_state->is_phylum_instance) {
        if (!find_instance(aug_graph, aug_graph->lhs_decl, synth_functions_state->source->fibered_attr, &aug_graph_instance)) {
          fatal_error("something is wrong with instances in aug graph %s", aug_graph_name(aug_graph));
        }
      } else {
        aug_graph_instance = synth_functions_state->source;
      }

      if (synth_functions_state->is_fiber_evaluation) {
        dump_fiber_dependencies_recursively(aug_graph, aug_graph_instance, os);
      }

      os << indent();
      impl->dump_synth_instance(aug_graph_instance, os);
      os << "\n";

      current_blocks.clear();

      nesting_level--;
      os << indent() << "}\n";
    }

    os << indent() << "case _ => throw new RuntimeException(\"failed pattern matching: \" + node)\n";

    nesting_level--;
    os << indent() << "};\n";

    if (synth_functions_state->is_fiber_evaluation) {
      os << indent() << "evaluated_map_" << synth_functions_state->fdecl_name << ".update(node, true);\n";
    } else {
      os << indent() << instance_to_attr(synth_functions_state->source) << ".assign(node, result);\n";
      os << indent() << instance_to_attr(synth_functions_state->source) << ".get(node);\n";
      os << indent() << "result\n";
    }
    nesting_level--;
    os << indent() << "}\n\n";
  }

  destroy_synth_function_states(synth_functions_states);
}

class SynthScc : public Implementation {
 public:
  typedef Implementation::ModuleInfo Super;
  class ModuleInfo : public Super {
   public:
    ModuleInfo(Declaration mdecl) : Implementation::ModuleInfo(mdecl) {}

    void note_top_level_match(Declaration tlm, GEN_OUTPUT& oss) { Super::note_top_level_match(tlm, oss); }

    void note_local_attribute(Declaration ld, GEN_OUTPUT& oss) {
      Super::note_local_attribute(ld, oss);
      Declaration_info(ld)->decl_flags |= LOCAL_ATTRIBUTE_FLAG;
    }

    void note_attribute_decl(Declaration ad, GEN_OUTPUT& oss) {
      Declaration_info(ad)->decl_flags |= ATTRIBUTE_DECL_FLAG;
      Super::note_attribute_decl(ad, oss);
    }

    void note_var_value_decl(Declaration vd, GEN_OUTPUT& oss) { Super::note_var_value_decl(vd, oss); }

#ifdef APS2SCALA
    void implement(ostream& os){
#else  /* APS2SCALA */
    void implement(output_streams& oss) {
#endif /* APS2SCALA */
        STATE* s = (STATE*)Declaration_info(module_decl) -> analysis_state;

#ifdef APS2SCALA
    ostream& oss = os;
#else
      ostream& hs = oss.hs;
      ostream& cpps = oss.cpps;
      ostream& os = inline_definitions ? hs : cpps;
      // char *name = decl_name(module_decl);
#endif /* APS2SCALA */

    dump_synth_functions(s, oss);

    // Implement finish routine:
#ifdef APS2SCALA
    os << indent() << "override def finish() : Unit = {\n";
#else  /* APS2SCALA */
      hs << indent() << "void finish()";
      if (!inline_definitions) {
        hs << ";\n";
        cpps << "void " << oss.prefix << "finish()";
      }
      INDEFINITION;
      os << " {\n";
#endif /* APS2SCALA */
    ++nesting_level;

    PHY_GRAPH* start_phy_graph = summary_graph_for(s, s->start_phylum);
    os << indent() << "for (root <- t_" << decl_name(s->start_phylum) << ".nodes) {\n";
    ++nesting_level;
    int i;
    for (i = 0; i < start_phy_graph->instances.length; i++) {
      INSTANCE* in = &start_phy_graph->instances.array[i];

      if (!instance_is_synthesized(in))
        continue;

      os << indent() << "eval_"
         << instance_to_string_with_nodetype(s->start_phylum, &start_phy_graph->instances.array[i])
         << "(root);\n";
    }
    --nesting_level;
    os << indent() << "}\n";

#ifdef APS2SCALA
    os << indent() << "super.finish();\n";
#endif /* ! APS2SCALA */
    --nesting_level;
    os << indent() << "};\n";

    clear_implementation_marks(module_decl);
  }
};

Super* get_module_info(Declaration m) {
  return new ModuleInfo(m);
}

void implement_function_body(Declaration f, ostream& os) {
  dynamic_impl->implement_function_body(f, os);
}

void implement_value_use(Declaration vd, ostream& os) {
  int flags = Declaration_info(vd)->decl_flags;
  if (flags & LOCAL_ATTRIBUTE_FLAG) {
    int instance_index = Declaration_info(vd)->instance_index;
    INSTANCE* instance = &current_aug_graph->instances.array[instance_index];

    Type ty = value_decl_type(vd);
    Declaration ctype_decl = canonical_type_decl(canonical_type(ty));
    os << "eval_" << instance_to_string_with_nodetype(ctype_decl, instance) << "(node";

    int i;
    int n = current_aug_graph->instances.length;
    for (i = 0; i < n; i++) {
      DEPENDENCY dep = edgeset_kind(current_aug_graph->graph[i * n + instance_index]);
      if (dep) {
        INSTANCE* source_instance = &current_aug_graph->instances.array[i];
        Declaration fibered_attr = source_instance->fibered_attr.attr;
        bool is_shared_info = ATTR_DECL_IS_SHARED_INFO(fibered_attr);
        bool is_fiber_attr = source_instance->fibered_attr.fiber != NULL;
        bool is_result = !strcmp("result", decl_name(source_instance->fibered_attr.attr));
        bool is_bad = Declaration_KEY(source_instance->node) == KEYpragma_call;

        if (source_instance->fibered_attr.fiber == NULL && !is_bad && !is_shared_info) {
          os << ", ";
          impl->dump_synth_instance(source_instance, os);
        }
      }
    }

    os << ")";
  } else if (flags & ATTRIBUTE_DECL_FLAG) {
    if (ATTR_DECL_IS_INH(vd)) {
      os << "v_" << decl_name(vd);
    } else {
      // os << "eval_" << decl_name(vd);
      os << "a" << "_" << decl_name(vd) << DEREF << "get";
    }

    // os << "a" << "_" << decl_name(vd) << DEREF << "get";
  } else if (flags & LOCAL_VALUE_FLAG) {
    os << "v" << LOCAL_UNIQUE_PREFIX(vd) << "_" << decl_name(vd);
  } else {
    aps_error(vd, "internal_error: What is special about this?");
  }
}

static Expression default_init(Default def) {
  switch (Default_KEY(def)) {
    case KEYsimple:
      return simple_value(def);
    case KEYcomposite:
      return composite_initial(def);
    default:
      return 0;
  }
}

/* Return new array with instance assignments for block.
 * If "from" is not NULL, then initialize the new array
 * with it.
 */
static vector<std::set<Expression> > make_instance_assignment() {
  int n = current_aug_graph->instances.length;

  vector<std::set<Expression> > from(n);

  // start from the outer-most and override it with the most inner scope
  for (auto it = current_blocks.begin(); it != current_blocks.end(); it++) {
    Block block = *it;
    vector<std::set<Expression> > array(from);

    for (int i = 0; i < n; ++i) {
      INSTANCE* in = &current_aug_graph->instances.array[i];
      Declaration ad = in->fibered_attr.attr;
      if (ad != 0 && in->fibered_attr.fiber == 0 && ABSTRACT_APS_tnode_phylum(ad) == KEYDeclaration) {
        // get default!
        switch (Declaration_KEY(ad)) {
          case KEYattribute_decl:
            array[in->index].insert(default_init(attribute_decl_default(ad)));
            break;
          case KEYvalue_decl:
            array[in->index].insert(default_init(value_decl_default(ad)));
            break;
          default:
            break;
        }
      }
    }

    // Step #1 clear any existing assignments and insert normal assignments
    // Step #2 insert collection assignments
    int step = 1;
    while (step <= 2) {
      Declarations ds = block_body(block);
      for (Declaration d = first_Declaration(ds); d; d = DECL_NEXT(d)) {
        switch (Declaration_KEY(d)) {
          case KEYnormal_assign: {
            if (INSTANCE* in = Expression_info(assign_rhs(d))->value_for) {
              if (in->index >= n)
                fatal_error("bad index [normal_assign] for instance");
              array[in->index].clear();
              array[in->index].insert(assign_rhs(d));
            }
            break;
          }
          case KEYcollect_assign: {
            if (INSTANCE* in = Expression_info(assign_rhs(d))->value_for) {
              if (in->index >= n)
                fatal_error("bad index [collection_assign] for instance");

              if (step == 1)
                array[in->index].clear();
              else
                array[in->index].insert(assign_rhs(d));
            }
            break;
          }
          default:
            break;
        }
      }

      step++;
    }

    // and repeat if any
    from = array;
  }

  return from;
}

void dump_assignment(INSTANCE* in, Expression rhs, ostream& o) {
  Declaration ad = in != NULL ? in->fibered_attr.attr : NULL;
  Symbol asym = ad ? def_name(declaration_def(ad)) : 0;
  bool node_is_syntax = in->node == current_aug_graph->lhs_decl;

  if (in->fibered_attr.fiber != NULL) {
    if (rhs == NULL) {
      if (include_comments) {
        o << indent() << "// " << in << "\n";
      }
      return;
    }

    Declaration assign = (Declaration)tnode_parent(rhs);
    Expression lhs = assign_lhs(assign);
    Declaration field = 0;
    o << indent();
    // dump the object containing the field
    switch (Expression_KEY(lhs)) {
      case KEYvalue_use:
        // shared global collection
        field = USE_DECL(value_use_use(lhs));
#ifdef APS2SCALA
        o << "a_" << decl_name(field) << ".";
        if (debug)
          o << "assign";
        else
          o << "set";
        o << "(" << rhs << ")";
#else  /* APS2SCALA */
          o << "v_" << decl_name(field) << "=";
          switch (Default_KEY(value_decl_default(field))) {
            case KEYcomposite:
              o << composite_combiner(value_decl_default(field));
              break;
            default:
              o << as_val(value_decl_type(field)) << "->v_combine";
              break;
          }
          o << "(v_" << decl_name(field) << "," << rhs << ");\n";
#endif /* APS2SCALA */
        break;
      case KEYfuncall:
        field = field_ref_p(lhs);
        if (field == 0)
          fatal_error("what sort of assignment lhs: %d", tnode_line_number(assign));
        o << "a_" << decl_name(field) << DEREF;
        if (debug)
          o << "assign";
        else
          o << "set";
        o << "(" << field_ref_object(lhs) << "," << rhs << ");\n";
        break;
      default:
        fatal_error("what sort of assignment lhs: %d", tnode_line_number(assign));
    }
    return;
  }

  if (in->node == 0 && ad != NULL) {
    if (rhs) {
      if (Declaration_info(ad)->decl_flags & LOCAL_ATTRIBUTE_FLAG) {
        o << indent() << "a" << LOCAL_UNIQUE_PREFIX(ad) << "_" << asym << DEREF;
        if (debug)
          o << "assign";
        else
          o << "set";
        o << "(anchor," << rhs << ");\n";
      } else {
        int i = LOCAL_UNIQUE_PREFIX(ad);
        if (i == 0) {
#ifdef APS2SCALA
          if (!def_is_constant(value_decl_def(ad))) {
            if (include_comments) {
              o << indent() << "// v_" << asym << " is assigned/initialized by default.\n";
            }
          } else {
            if (include_comments) {
              o << indent() << "// v_" << asym << " is initialized in module.\n";
            }
          }
#else
            o << indent() << "v_" << asym << " = " << rhs << ";\n";
#endif
        } else {
          o << indent() << "v" << i << "_" << asym << " = " << rhs << "; // local\n";
        }
      }
    } else {
      if (Declaration_KEY(ad) == KEYvalue_decl && !direction_is_collection(value_decl_direction(ad))) {
        aps_warning(ad, "Local attribute %s is apparently undefined", decl_name(ad));
      }
      if (include_comments) {
        o << indent() << "// " << in << " is ready now\n";
      }
    }
    return;
  } else if (node_is_syntax) {
    if (ATTR_DECL_IS_SHARED_INFO(ad)) {
      if (include_comments) {
        o << indent() << "// shared info for " << decl_name(in->node) << " is ready.\n";
      }
    } else if (ATTR_DECL_IS_UP_DOWN(ad)) {
      if (include_comments) {
        o << indent() << "// " << decl_name(in->node) << "." << decl_name(ad) << " implicit.\n";
      }
    } else if (rhs) {
      if (Declaration_KEY(in->node) == KEYfunction_decl) {
        if (direction_is_collection(value_decl_direction(ad))) {
          std::cout << "Not expecting collection here!\n";
          o << indent() << "v_" << asym << " = somehow_combine(v_" << asym << "," << rhs << ");\n";
        } else {
          int i = LOCAL_UNIQUE_PREFIX(ad);
          if (i == 0)
            o << indent() << "v_" << asym << " = " << rhs << "; // function\n";
          else
            o << indent() << "v" << i << "_" << asym << " = " << rhs << ";\n";
        }
      } else {
        o << indent() << "a_" << asym << DEREF;
        if (debug)
          o << "assign";
        else
          o << "set";
        o << "(v_" << decl_name(in->node) << "," << rhs << ");\n";
      }
    } else {
      aps_warning(in->node, "Attribute %s.%s is apparently undefined", decl_name(in->node),
                  symbol_name(asym));

      if (include_comments) {
        o << indent() << "// " << in << " is ready.\n";
      }
    }
    return;
  } else if (Declaration_KEY(in->node) == KEYvalue_decl) {
    if (rhs) {
      // assigning field of object
      o << indent() << "a_" << asym << DEREF;
      if (debug)
        o << "assign";
      else
        o << "set";
      o << "(v_" << decl_name(in->node) << "," << rhs << ");\n";
    } else {
      if (include_comments) {
        o << indent() << "// " << in << " is ready now.\n";
      }
    }
    return;
  }
}

void dump_rhs_instance_helper(AUG_GRAPH* aug_graph, BlockItem* item, INSTANCE* instance, ostream& o) {
  if (item->key == KEY_BLOCK_ITEM_INSTANCE) {
    vector<std::set<Expression> > all_assignments = make_instance_assignment();
    std::set<Expression> relevant_assignments = all_assignments[instance->index];

    if (!relevant_assignments.empty()) {
      for (auto it = relevant_assignments.begin(); it != relevant_assignments.end(); it++) {
        Expression rhs = *it;
        if (instance->fibered_attr.fiber != NULL) {
          dump_assignment(instance, rhs, o);
        } else {
          // just dump RHS since synth functions are only interested in RHS, not side-effect
          dump_Expression(rhs, o);
        }
      }

      return;
    }

    if (instance->fibered_attr.fiber != NULL) {
      // Shared info attribute wasn't assigned in this block, dump its default
      if (include_comments) {
        o << "/* did not find any assignment for this fiber attribute " << instance << " */";
      }
      return;
    } else {
      fatal_error("crashed since non-fiber instance is missing an assignment");
      return;
    }
  } else if (item->key == KEY_BLOCK_ITEM_CONDITION) {
    struct block_item_condition* cond = (struct block_item_condition*)item;
    Declaration if_stmt = cond->condition;
    bool result = true;

    o << "if (";
    dump_Expression(if_stmt_cond(if_stmt), o);
    o << ") {\n";
    nesting_level++;
    current_blocks.push_back(if_stmt_if_true(if_stmt));
    o << indent();
    dump_rhs_instance_helper(aug_graph, cond->next_positive, instance, o);
    current_blocks.pop_back();
    o << "\n";
    nesting_level--;
    o << indent() << "} else {\n";
    nesting_level++;
    current_blocks.push_back(if_stmt_if_false(if_stmt));
    o << indent();
    dump_rhs_instance_helper(aug_graph, cond->next_negative, instance, o);
    current_blocks.pop_back();
    nesting_level--;
    o << "\n";
    o << indent() << "}";
  }
}

virtual void dump_synth_instance(INSTANCE* instance, ostream& o) override {
  AUG_GRAPH* aug_graph = current_aug_graph;
  BlockItem* block = find_surrounding_block(current_scope_block, instance);

  Declaration node = instance->node;
  Declaration attr = instance->fibered_attr.attr;
  bool is_parent_instance = current_aug_graph->lhs_decl == instance->node;

  bool is_synthesized = instance_is_synthesized(instance);
  bool is_inherited = instance_is_inherited(instance);

  if (is_inherited) {
    if (is_parent_instance) {
      o << "v_" << instance_to_string(instance, false, true);
      return;
    } else {
      // we need to find the assignment and dump the RHS recursive call
      dump_rhs_instance_helper(aug_graph, block, instance, o);
    }
  } else if (is_synthesized) {
    if (is_parent_instance) {
      dump_rhs_instance_helper(aug_graph, block, instance, o);
    } else {
      Type ty = infer_formal_type(node);
      Declaration ctype_decl = canonical_type_decl(canonical_type(ty));
      PHY_GRAPH* pgraph = summary_graph_for(current_state, ctype_decl);

      for (auto it = synth_functions_states.begin(); it != synth_functions_states.end(); it++) {
        SYNTH_FUNCTION_STATE* synth_function_state = *it;
        if (fibered_attr_equal(&synth_function_state->source->fibered_attr, &instance->fibered_attr)) {
          o << "eval_" << synth_function_state->fdecl_name << "(" << "v_" << decl_name(node);

          std::vector<INSTANCE*> dependencies = synth_function_state->regular_dependencies;
          for (auto it = dependencies.begin(); it != dependencies.end(); it++) {
            INSTANCE* source_instance = *it;

            if (source_instance->fibered_attr.fiber != NULL) {
              continue;
            }

            for (int i = 0; i < current_aug_graph->instances.length; i++) {
              INSTANCE* in = &current_aug_graph->instances.array[i];
              if (in->node == node && fibered_attr_equal(&in->fibered_attr, &source_instance->fibered_attr)) {
                o << ", ";
                o << "/*" << source_instance << "*/ ";
                dump_synth_instance(in, o);
              }
            }
          }

          o << ")";
          return;
        }
      }

      printf("failed to find synth function for instance ");
      print_instance(instance, stdout);
      printf("\n");
      fatal_error("internal error: failed to find synth function for instance");
    }
  } else {
    dump_rhs_instance_helper(aug_graph, block, instance, o);
  }
}
}
;

Implementation* synth_impl = new SynthScc();
