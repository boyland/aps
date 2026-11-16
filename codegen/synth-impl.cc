#include <string.h>

#include <algorithm>
#include <iostream>
extern "C" {
#include <stdio.h>

#include "aps-ag.h"
}
#include <set>
#include <stack>
#include <vector>

#include "dump.h"
#include "implement.h"
#include "synth-util.h"

#ifdef APS2SCALA

void dump_sequence_element_patterns(const std::vector<Pattern>&, ostream&);
void dump_sequence_candidates(Pattern, Expression, const SequenceForPattern&, unsigned, ostream&);

static AUG_GRAPH* current_aug_graph = NULL;
static std::vector<synth_util::SynthFunctionState*> synth_functions_states;
static std::vector<synth_util::SynthCompletionState*> synth_completion_states;
static synth_util::SynthFunctionState* current_synth_functions_state = NULL;
static bool emitting_completion = false;

#define DEREF "."

static SynthImplementation* synth_impl_ptr;

static vector<Block> current_blocks;
static synth_util::BlockItem* current_scope_block;
static vector<synth_util::BlockItem*> dumped_conditional_block_items;
static vector<INSTANCE*> dumped_instances;
static bool emitting_fiber_fixed_point_loop = false;

static const string PREV_LOOP_VAR = "prevIsInsideFixedPoint";

struct FiberScheduleChoice {
  int index;
  bool requires_fixed_point_loop;
};

static DEPENDENCY component_dependency(
    const AUG_GRAPH* aug_graph, const SCC_COMPONENT* component, int source, int sink) {
  const INSTANCE* source_instance = static_cast<INSTANCE*>(component->array[source]);
  const INSTANCE* sink_instance = static_cast<INSTANCE*>(component->array[sink]);
  return edgeset_kind(aug_graph->graph[
      source_instance->index * aug_graph->instances.length + sink_instance->index]);
}

static const SCC_COMPONENT* find_fiber_component(
    const SCC_COMPONENTS* components, int instance_index) {
  for (int i = 0; i < components->length; i++) {
    const SCC_COMPONENT* component = components->array[i];
    for (int j = 0; j < component->length; j++) {
      if (*static_cast<const int*>(component->array[j]) == instance_index) {
        return component;
      }
    }
  }
  fatal_error("cannot find SCC component for fiber instance %d", instance_index);
  return NULL;
}

static FiberScheduleChoice next_fiber_instance(
    const AUG_GRAPH* aug_graph, const SCC_COMPONENT* component, const vector<bool>& scheduled) {
  int count = component->length;
  int remaining = 0;
  for (int i = 0; i < count; i++) {
    if (scheduled[i]) {
      continue;
    }
    remaining++;
    bool ready = true;
    for (int j = 0; j < count; j++) {
      if (!scheduled[j] &&
          (component_dependency(aug_graph, component, j, i) & DEPENDENCY_MAYBE_DIRECT)) {
        ready = false;
        break;
      }
    }
    if (ready) {
      return {i, false};
    }
  }
  if (remaining == 0) {
    return {-1, false};
  }

  vector<int> vertices(count);
  SccGraph graph;
  scc_graph_initialize(&graph, remaining);
  for (int i = 0; i < count; i++) {
    if (!scheduled[i]) {
      vertices[i] = i;
      scc_graph_add_vertex(&graph, &vertices[i]);
    }
  }
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < count; j++) {
      if (!scheduled[i] && !scheduled[j] &&
          (component_dependency(aug_graph, component, i, j) & DEPENDENCY_MAYBE_DIRECT)) {
        scc_graph_add_edge(&graph, &vertices[i], &vertices[j]);
      }
    }
  }

  SCC_COMPONENTS* components = scc_graph_components(&graph);

  FiberScheduleChoice result = {-1, false};
  for (int i = 0; i < count; i++) {
    if (scheduled[i]) {
      continue;
    }
    const SCC_COMPONENT* instance_component = find_fiber_component(components, i);
    if (instance_component->length < 2 &&
        !(component_dependency(aug_graph, component, i, i) & DEPENDENCY_MAYBE_DIRECT)) {
      continue;
    }
    bool ready = true;
    bool requires_fixed_point_loop = false;
    for (int j = 0; j < count; j++) {
      if (scheduled[j]) {
        continue;
      }
      for (int k = 0; k < count; k++) {
        if (scheduled[k] || find_fiber_component(components, k) != instance_component ||
            !(component_dependency(aug_graph, component, j, k) & DEPENDENCY_MAYBE_DIRECT)) {
          continue;
        }
        if (find_fiber_component(components, j) != instance_component) {
          ready = false;
        } else if (component_dependency(aug_graph, component, j, k) & DEPENDENCY_NOT_JUST_FIBER) {
          requires_fixed_point_loop = true;
        }
      }
    }
    if (ready) {
      result = {i, requires_fixed_point_loop};
      break;
    }
  }

  for (int i = 0; i < components->length; i++) {
    free(components->array[i]->array);
    free(components->array[i]);
  }
  free(components->array);
  free(components);
  scc_graph_destroy(&graph);
  return result;
}

static void emit_fiber_convergence_guard(ostream& os) {
  os << indent() << "if (!" << synth_util::LOOP_VAR << ") {\n";
  ++nesting_level;
  os << indent() << "throw Evaluation.CyclicAttributeException(\"value-carrying fiber cycle requires a fixed-point loop\");\n";
  --nesting_level;
  os << indent() << "}\n";
}

class FiberDependencyDumper {
 public:
  static void dump(AUG_GRAPH* aug_graph, INSTANCE* sink, ostream& os) {
    int n = aug_graph->instances.length;
    vector<INSTANCE*> relevant_instances;

    for (int i = 0; i < n; i++) {
      INSTANCE* in = &aug_graph->instances.array[i];
      if (in->node != NULL && Declaration_KEY(in->node) == KEYpragma_call) {
        continue;
      }
      if (in->index == sink->index) {
        continue;
      }
      if (edgeset_kind(aug_graph->graph[in->index * n + sink->index]) &&
        in->fibered_attr.fiber != NULL &&
        (synth_util::instance_is_synthesized(in) ||
          fibered_attr_direction(&in->fibered_attr) == instance_local ||
          fiber_is_reverse(in->fibered_attr.fiber))) {
        relevant_instances.push_back(in);
      }
    }

    if (relevant_instances.empty()) {
      return;
    }

    bool* scheduled = (bool*)alloca(sizeof(bool) * n);
    memset(scheduled, 0, sizeof(bool) * n);

    SccGraph scc_graph;
    scc_graph_initialize(&scc_graph, static_cast<int>(relevant_instances.size()));

    for (auto it = relevant_instances.begin(); it != relevant_instances.end(); it++) {
      scc_graph_add_vertex(&scc_graph, *it);
    }

    for (auto it1 = relevant_instances.begin(); it1 != relevant_instances.end(); it1++) {
      INSTANCE* in1 = *it1;
      for (auto it2 = relevant_instances.begin(); it2 != relevant_instances.end(); it2++) {
        INSTANCE* in2 = *it2;
        if (in1->index == in2->index) {
          continue;
        }
        if (edgeset_kind(aug_graph->graph[in1->index * n + in2->index])) {
          scc_graph_add_edge(&scc_graph, in1, in2);
        }
      }
    }

    SCC_COMPONENTS* components = scc_graph_components(&scc_graph);

    dump_scc_helper(aug_graph, components, scheduled, os);

    scc_graph_destroy(&scc_graph);
  }

 private:
  static bool already_scheduled(SCC_COMPONENT* component, bool* scheduled) {
    for (int i = 0; i < component->length; i++) {
      INSTANCE* in = (INSTANCE*)component->array[i];
      if (!scheduled[in->index]) {
        return false;
      }
    }
    return true;
  }

  static SCC_COMPONENT* find_next_ready_component(AUG_GRAPH* aug_graph, SCC_COMPONENTS* components, bool* scheduled) {
    int n = aug_graph->instances.length;
    SCC_COMPONENT* next_component = NULL;
    int lowest_instance_index = n;

    for (int i = 0; i < components->length; i++) {
      SCC_COMPONENT* component = components->array[i];
      if (already_scheduled(component, scheduled)) {
        continue;
      }

      bool component_ready = true;
      for (int j = 0; j < component->length && component_ready; j++) {
        INSTANCE* in = (INSTANCE*)component->array[j];

        for (int k = 0; k < components->length && component_ready; k++) {
          SCC_COMPONENT* other_component = components->array[k];
          if (other_component == component || already_scheduled(other_component, scheduled)) {
            continue;
          }

          for (int l = 0; l < other_component->length; l++) {
            INSTANCE* other_in = (INSTANCE*)other_component->array[l];
            if (edgeset_kind(aug_graph->graph[other_in->index * n + in->index])) {
              component_ready = false;
              break;
            }
          }
        }
      }

      if (component_ready) {
        // Preserve contribution order when independent fibers build sequences.
        for (int j = 0; j < component->length; j++) {
          INSTANCE* in = (INSTANCE*)component->array[j];
          if (in->index < lowest_instance_index) {
            next_component = component;
            lowest_instance_index = in->index;
          }
        }
      }
    }

    if (next_component != NULL) {
      return next_component;
    }
    fatal_error("no more components to schedule");
    return NULL;
  }

  static void dump_component(AUG_GRAPH* aug_graph, SCC_COMPONENT* component, bool* scheduled, ostream& os) {
    int component_instance_count = component->length;
    vector<bool> component_scheduled(component_instance_count, false);
    for (int i = 0; i < component_instance_count; i++) {
      INSTANCE* source = (INSTANCE*)component->array[i];
      component_scheduled[i] = scheduled[source->index];
    }

    bool needs_caller_fixed_point_check = true;
    while (!already_scheduled(component, scheduled)) {
      FiberScheduleChoice choice =
          next_fiber_instance(aug_graph, component, component_scheduled);
      if (choice.index < 0) {
        fatal_error("cannot find a ready fiber dependency cycle in %s", aug_graph_name(aug_graph));
      }
      INSTANCE* next = (INSTANCE*)component->array[choice.index];
      // Fiber-only cycles are propagated reference dependencies.
      if (choice.requires_fixed_point_loop && !emitting_fiber_fixed_point_loop) {
        if (!aug_graph->global_state->requires_fixed_point_loop) {
          fatal_error("cannot schedule value-carrying fiber cycle in %s at instance %s (index %d) without a fixed-point loop",
                      aug_graph_name(aug_graph),
                      synth_util::instance_to_string(next).c_str(), next->index);
        }
        // This function does not emit its own loop. It can evaluate the cycle
        // only if its caller repeats evaluation until values stop changing.
        // The caller signals that guarantee with isInsideFixedPoint=true.
        if (needs_caller_fixed_point_check) {
          emit_fiber_convergence_guard(os);
          needs_caller_fixed_point_check = false;
        }
      }
      component_scheduled[choice.index] = true;
      scheduled[next->index] = true;
      os << indent();
      synth_impl_ptr->dump_synth_instance(next, os);
      dumped_conditional_block_items.clear();
      dumped_instances.clear();
      os << ";\n";
    }
  }

  static void dump_scc_helper(AUG_GRAPH* aug_graph, SCC_COMPONENTS* components, bool* scheduled, ostream& os) {
    int component_count = components->length;

    for (int i = 0; i < component_count; i++) {
      SCC_COMPONENT* component = find_next_ready_component(aug_graph, components, scheduled);
      dump_component(aug_graph, component, scheduled, os);
    }

    for (int i = 0; i < component_count; i++) {
      if (!already_scheduled(components->array[i], scheduled)) {
        fatal_error("some instances were not scheduled");
      }
    }
  }
};

static void emit_fixed_point_loop_start(ostream& os, const string& flag, bool is_independent = false, INSTANCE* cycle_instance = NULL) {
  // An independent cycle can finish on its own. Otherwise, let an enclosing
  // fixed-point loop drive it instead of starting a nested loop.
  if (is_independent) {
    if (cycle_instance != NULL && cycle_instance->node != NULL) {
      os << indent() << "if (" << synth_util::instance_to_attr(cycle_instance) << ".checkNode(v_" << decl_name(cycle_instance->node) << ").status != Evaluation.ASSIGNED) {\n";
      ++nesting_level;
    }
  } else {
    os << indent() << "if (!" << synth_util::LOOP_VAR << ") {\n";
    ++nesting_level;
  }

  synth_util::emit_fixed_point_loop_start(os, flag);
}

static void emit_fixed_point_loop_end(ostream& os, bool is_independent = false, INSTANCE* cycle_instance = NULL) {
  synth_util::emit_fixed_point_loop_end(os);

  if (is_independent) {
    if (cycle_instance != NULL && cycle_instance->node != NULL) {
      --nesting_level;
      os << indent() << "}\n";
    }
  } else {
    --nesting_level;
    os << indent() << "}\n";
  }
}

static void dump_synth_completion_functions(ostream& os) {
  synth_util::dump_synth_completion_functions(
      synth_completion_states, synth_functions_states, os,
      [](AUG_GRAPH* graph) {
        current_aug_graph = graph;
        current_synth_functions_state = NULL;
        emitting_completion = true;
        current_blocks.push_back(
            matcher_body(top_level_match_m(graph->match_rule)));
      },
      []() {
        current_blocks.clear();
        dumped_conditional_block_items.clear();
        dumped_instances.clear();
        emitting_completion = false;
      },
      [](AUG_GRAPH* graph, INSTANCE* instance, ostream& output) {
        current_scope_block = synth_util::linearize_block(graph, instance);
        dumped_conditional_block_items.clear();
        dumped_instances.clear();
        synth_impl_ptr->dump_synth_instance(instance, output);
      });
}

struct ObjectFieldAssign {
  Declaration field;
  Expression rhs;
  INSTANCE* instance;
};

static std::vector<ObjectFieldAssign> collect_object_field_assignments(AUG_GRAPH* aug_graph, Declaration obj_decl) {
  std::vector<ObjectFieldAssign> result;
  Block body = matcher_body(top_level_match_m(aug_graph->match_rule));
  for (Declaration d = first_Declaration(block_body(body)); d; d = DECL_NEXT(d)) {
    if (Declaration_KEY(d) != KEYnormal_assign) {
      continue;
    }
    Expression lhs = assign_lhs(d);
    if (Expression_KEY(lhs) != KEYfuncall) {
      continue;
    }
    Declaration field = field_ref_p(lhs);
    if (field == NULL) {
      continue;
    }
    Expression obj = field_ref_object(lhs);
    if (Expression_KEY(obj) != KEYvalue_use) {
      continue;
    }
    if (USE_DECL(value_use_use(obj)) != obj_decl) {
      continue;
    }
    ObjectFieldAssign fa;
    fa.field = field;
    fa.rhs = assign_rhs(d);
    fa.instance = Expression_info(assign_rhs(d))->value_for;
    result.push_back(fa);
  }
  return result;
}

static void dump_synth_functions(STATE* s, ostream& os) {
  os << "\n";

  synth_functions_states = synth_util::build_synth_function_states(
      s, /* include_field_assign_dependencies */ true);
  for (auto function_state : synth_functions_states) {
    if (!function_state->is_phylum_instance) {
      continue;
    }
    PHY_GRAPH* graph = function_state->source_phy_graph;
    auto& dependencies = function_state->regular_dependencies;
    // Fiber evaluation may construct local objects using the inherited anchor.
    for (size_t index = 0; index <= dependencies.size(); ++index) {
      INSTANCE* in = index == 0 ? function_state->source : dependencies[index - 1];
      if (in->fibered_attr.fiber == NULL) {
        continue;
      }
      for (int base_index = 0; base_index < graph->instances.length; ++base_index) {
        INSTANCE* base = &graph->instances.array[base_index];
        if (base->fibered_attr.fiber == NULL &&
            base->fibered_attr.attr == in->fibered_attr.attr &&
            !synth_util::instance_is_pure_shared_info(base) &&
            synth_util::instance_is_inherited(base) &&
            std::find(dependencies.begin(), dependencies.end(), base) == dependencies.end()) {
          dependencies.push_back(base);
        }
      }
    }
  }
  bool needs_fixed_point = s->requires_fixed_point_loop || s->requires_circular_update_support;

  for (auto state_it = synth_functions_states.begin(); state_it != synth_functions_states.end(); state_it++) {
    synth_util::SynthFunctionState* function_state = *state_it;
    current_synth_functions_state = function_state;
    string result_var = synth_util::RESULT_VAR_PREFIX + std::to_string(function_state->source->index);

    if (include_comments) {
      os << indent() << "// " << function_state->source << " (" << (function_state->is_phylum_instance ? "phylum" : "aug-graph") << ")\n";
    }
    if (function_state->is_side_effect_evaluation) {
      os << indent() << "val evaluated_map_" << function_state->fdecl_name << " = scala.collection.mutable.Map[Int, Boolean]()\n\n";
    }

    os << indent() << "def eval_" << function_state->fdecl_name << "(";
    os << "node: T_" << decl_name(function_state->source_phy_graph->phylum);

    for (auto it = function_state->regular_dependencies.begin(); it != function_state->regular_dependencies.end(); it++) {
      INSTANCE* source_instance = *it;
      if (synth_util::should_skip_synth_dependency(source_instance)) {
        continue;
      }
      os << ",\n" << indent(nesting_level + 1) << "v_";
      if (!function_state->is_phylum_instance) {
        os << synth_util::instance_to_string(source_instance) << ": ";
      } else {
        os << synth_util::instance_to_string(source_instance, true) << ": ";
      }
      synth_util::dump_attribute_type(source_instance, os);
    }

    os << ")";
    if (needs_fixed_point) {
      os << "(implicit " << synth_util::LOOP_VAR << ": Boolean, changed: AtomicBoolean)";
    }

    os << ": ";
    if (function_state->is_side_effect_evaluation) {
      os << "Unit";
    } else {
      synth_util::dump_attribute_type(function_state->source, os);
    }
    os << " = {\n";
    nesting_level++;

    bool bypass_cache_in_fixed_point = needs_fixed_point &&
        (!function_state->is_side_effect_evaluation ||
         instance_circular(function_state->source));
    if (bypass_cache_in_fixed_point) {
      os << indent() << "if (!" << synth_util::LOOP_VAR << ") {\n";
      nesting_level++;
    }

    if (function_state->is_side_effect_evaluation) {
      os << indent() << "evaluated_map_" << function_state->fdecl_name << ".getOrElse(node.nodeNumber, false) match {\n";
      os << indent(nesting_level + 1) << "case true => return ()\n";
    } else {
      os << indent() << synth_util::instance_to_attr(function_state->source) << ".checkNode(node).status match {\n";
      os << indent(nesting_level + 1) << "case Evaluation.ASSIGNED => ";
      if (include_comments) {
        os << "{\n";
        nesting_level++;
        os << indent(nesting_level + 1) << "Debug.out(\"cache hit for \" + node + \" with value \" + " << synth_util::instance_to_attr(function_state->source) << ".get(node));\n";
        os << indent(nesting_level + 1);
      }
      os << "return " << synth_util::instance_to_attr(function_state->source) << ".get(node)\n";
      if (include_comments) {
        nesting_level--;
        os << indent(nesting_level + 1) << "}\n";
      }
    }

    os << indent(nesting_level + 1) << "case _ => ()\n";
    os << indent() << "};\n";

    if (bypass_cache_in_fixed_point) {
      nesting_level--;
      os << indent() << "}\n";
    }

    if (function_state->is_side_effect_evaluation) {
      os << indent() << "node match {\n";
    } else {
      os << indent() << "val " << result_var << " = ";
      if (needs_fixed_point &&
          DECL_IS_OBJECT(function_state->source->fibered_attr.attr)) {
        string object_attr = synth_util::instance_to_attr(function_state->source);
        os << "if (" << object_attr << ".checkNode(node).status == Evaluation.ASSIGNED) "
           << object_attr << ".get(node) else ";
      }
      os << "node match {\n";
    }
    nesting_level++;

    bool source_circular = needs_fixed_point && synth_util::synth_function_is_circular(function_state);

    for (auto it = function_state->aug_graphs.begin(); it != function_state->aug_graphs.end(); it++) {
      AUG_GRAPH* aug_graph = *it;
      int n = aug_graph->instances.length;

      current_aug_graph = aug_graph;
      current_blocks.push_back(matcher_body(top_level_match_m(aug_graph->match_rule)));

      os << indent() << "case " << matcher_pat(top_level_match_m(aug_graph->match_rule)) << " => {\n";
      nesting_level++;

      INSTANCE* aug_graph_instance = NULL;
      if (function_state->is_phylum_instance) {
        if (!synth_util::find_instance(aug_graph, aug_graph->lhs_decl, function_state->source->fibered_attr, &aug_graph_instance)) {
          fatal_error("something is wrong with instances in aug graph %s", aug_graph_name(aug_graph));
        }
      } else {
        aug_graph_instance = function_state->source;
      }

      current_scope_block = synth_util::linearize_block(aug_graph, aug_graph_instance);

      if (include_comments) {
        os << indent() << "/* Linearized schedule:\n";
        nesting_level++;
        synth_util::print_linearized_block(current_scope_block, os);
        nesting_level--;
        os << indent() << "*/\n";
      }

      int source_instance_index = function_state->source->index;
      string src_attr = synth_util::instance_to_attr(function_state->source);

      bool declared_is_circular = instance_circular(aug_graph_instance);
      bool depends_on_itself = edgeset_kind(aug_graph->graph[source_instance_index * n + source_instance_index]) != 0;
      if (!declared_is_circular && depends_on_itself) {
        aps_warning(aug_graph_instance->node, "Instance %s depends on itself but is not declared circular", synth_util::instance_to_string(aug_graph_instance).c_str());
      }

      bool needs_local_fixed_point_loop = function_state->is_side_effect_evaluation && declared_is_circular && !synth_util::instance_is_pure_shared_info(function_state->source);
      string node_get = ATTR_DECL_IS_SHARED_INFO(function_state->source->fibered_attr.attr) ? "" : "node";
      string node_assign = ATTR_DECL_IS_SHARED_INFO(function_state->source->fibered_attr.attr) ? "" : "node, ";

      if (!function_state->is_side_effect_evaluation) {
        std::vector<INSTANCE*> child_cycle_instances = synth_util::collect_child_cycle_instances(aug_graph);
        for (auto it2 = child_cycle_instances.begin(); it2 != child_cycle_instances.end(); it2++) {
          INSTANCE* cycle_instance = *it2;
          bool independent = synth_util::child_cycle_is_independent(aug_graph, cycle_instance);
          INSTANCE* guarded_cycle = independent ? cycle_instance : NULL;
          emit_fixed_point_loop_start(os, "localChanged", independent, guarded_cycle);
          dumped_conditional_block_items.clear();
          dumped_instances.clear();
          os << indent();
          synth_impl_ptr->dump_synth_instance(cycle_instance, os);
          os << ";\n";
          emit_fixed_point_loop_end(os, independent, guarded_cycle);
        }
        dumped_conditional_block_items.clear();
        dumped_instances.clear();
      }

      if (needs_local_fixed_point_loop) {
        os << indent() << "{\n";
        nesting_level++;
        os << indent() << "val " << PREV_LOOP_VAR << source_instance_index << " = " << synth_util::LOOP_VAR << ";\n";
        os << indent() << "val prevChanged" << source_instance_index << " = changed;\n";
        os << indent() << "val newChanged" << source_instance_index << " = new AtomicBoolean(false);\n";
        if (include_comments) {
          os << indent() << "var iterCount" << source_instance_index << " = 0;\n";
        }
        os << indent() << "do {\n";
        nesting_level++;
        os << indent() << "newChanged" << source_instance_index << ".set(false);\n";
        emitting_fiber_fixed_point_loop = true;
        synth_util::emit_loop_implicits(os, "newChanged" + std::to_string(source_instance_index));
      }

      if (include_comments && function_state->is_side_effect_evaluation && !needs_local_fixed_point_loop) {
        os << "\n";
      }
      FiberDependencyDumper::dump(aug_graph, aug_graph_instance, os);

      if (function_state->is_side_effect_evaluation) {
        os << indent();
        synth_impl_ptr->dump_synth_instance(aug_graph_instance, os);
        os << "\n";
        dumped_conditional_block_items.clear();
        dumped_instances.clear();
      }

      if (!function_state->is_side_effect_evaluation) {
        if (needs_local_fixed_point_loop) {
          os << indent() << src_attr << ".assign(" << node_assign;
          synth_impl_ptr->dump_synth_instance(aug_graph_instance, os);
          os << ", changed);\n";
        } else {
          os << indent();
          synth_impl_ptr->dump_synth_instance(aug_graph_instance, os);
          os << "\n";
        }
      }

      if (needs_local_fixed_point_loop) {
        emitting_fiber_fixed_point_loop = false;
        if (include_comments) {
          os << indent() << "iterCount" << source_instance_index << " += 1;\n";
          os << indent() << "Debug.out(\"fixed-point " << function_state->fdecl_name << " node=\" + node + \" iteration=\" + iterCount" << source_instance_index << ");\n";
        }
        nesting_level--;
        os << indent() << "} while (newChanged" << source_instance_index << ".get && !" << PREV_LOOP_VAR << source_instance_index << ")\n";
        os << indent() << "prevChanged" << source_instance_index << ".compareAndSet(false, newChanged" << source_instance_index << ".get);\n";
        if (!function_state->is_side_effect_evaluation) {
          os << indent() << src_attr << ".get(" << node_get << ")\n";
        }
        nesting_level--;
        os << indent() << "}\n";
      }

      current_blocks.clear();
      dumped_conditional_block_items.clear();
      dumped_instances.clear();

      nesting_level--;
      os << indent() << "}\n";
    }

    os << indent() << "case _ => throw new RuntimeException(\"failed pattern matching: \" + node)\n";

    nesting_level--;
    os << indent() << "};\n";

    if (function_state->is_side_effect_evaluation) {
      os << indent() << "evaluated_map_" << function_state->fdecl_name << ".update(node.nodeNumber, true);\n";
    } else {
      if (source_circular) {
        os << indent() << synth_util::instance_to_attr(function_state->source) << ".assign(node, " << result_var << ", changed);\n";
      } else {
        os << indent() << synth_util::instance_to_attr(function_state->source) << ".assign(node, " << result_var << ");\n";
      }
      os << indent() << synth_util::instance_to_attr(function_state->source) << ".get(node);\n";
    }

    if (!function_state->is_side_effect_evaluation && !function_state->is_phylum_instance) {
      Declaration obj_decl = function_state->source->fibered_attr.attr;
      for (auto ag_it = function_state->aug_graphs.begin(); ag_it != function_state->aug_graphs.end(); ag_it++) {
        AUG_GRAPH* aug_graph = *ag_it;
        std::vector<ObjectFieldAssign> field_assigns = collect_object_field_assignments(aug_graph, obj_decl);
        if (field_assigns.empty()) {
          continue;
        }

        current_aug_graph = aug_graph;
        current_blocks.clear();
        current_blocks.push_back(matcher_body(top_level_match_m(aug_graph->match_rule)));

        os << indent() << "node match {\n";
        nesting_level++;
        os << indent() << "case " << matcher_pat(top_level_match_m(aug_graph->match_rule)) << " => {\n";
        nesting_level++;

        for (auto fa = field_assigns.begin(); fa != field_assigns.end(); fa++) {
          if (fa->instance == NULL) {
            continue;
          }
          current_scope_block = synth_util::linearize_block(aug_graph, fa->instance);
          dumped_conditional_block_items.clear();
          dumped_instances.clear();
          os << indent() << "a_" << decl_name(fa->field) << DEREF;
          if (debug) {
            os << "assign";
          } else {
            os << "set";
          }
          os << "(" << result_var << ", ";
          dump_Expression(fa->rhs, os);
          os << ");\n";
        }

        nesting_level--;
        os << indent() << "}\n";
        os << indent() << "case _ => ()\n";
        nesting_level--;
        os << indent() << "};\n";

        current_blocks.clear();
        dumped_conditional_block_items.clear();
        dumped_instances.clear();
      }
    }

    if (!function_state->is_side_effect_evaluation) {
      os << indent() << result_var << "\n";
    }

    nesting_level--;
    os << indent() << "}\n\n";
  }

}

class SynthImpl : public SynthImplementation {
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

    void implement(ostream& os) {
      STATE* s = (STATE*)Declaration_info(module_decl)->analysis_state;
      dump_synth_functions(s, os);
      synth_completion_states = synth_util::build_synth_completion_states(s);
      dump_synth_completion_functions(os);
      synth_util::dump_synth_finish(s, synth_completion_states, os);

      synth_util::destroy_synth_function_states(synth_functions_states);
      synth_functions_states.clear();
      synth_util::destroy_synth_completion_states(synth_completion_states);
      synth_completion_states.clear();

      clear_implementation_marks(module_decl);
    }
  };

  Super* get_module_info(Declaration m) { return new ModuleInfo(m); }

  void implement_function_body(Declaration f, ostream& os) { dynamic_impl->implement_function_body(f, os); }

  void implement_value_use(Declaration vd, ostream& os) { synth_util::implement_value_use(vd, current_aug_graph, synth_functions_states, this, os); }

  void dump_assignment(INSTANCE* in, Expression rhs, ostream& o) {
    Declaration ad = in != NULL ? in->fibered_attr.attr : NULL;
    Symbol asym = ad ? def_name(declaration_def(ad)) : 0;
    bool node_is_syntax = in->node == current_aug_graph->lhs_decl;

    if (in->fibered_attr.fiber != NULL) {
      if (rhs == NULL) {
        if (include_comments) {
          o << "// " << in << "\n";
        }
        return;
      }

      Declaration assign = (Declaration)tnode_parent(rhs);
      Expression lhs = assign_lhs(assign);
      Declaration field = 0;
      switch (Expression_KEY(lhs)) {
        case KEYvalue_use:
          field = USE_DECL(value_use_use(lhs));
          o << "a_" << decl_name(field) << ".";
          if (debug) {
            o << "assign";
          } else {
            o << "set";
          }
          o << "(";
          dump_Expression(rhs, o);
          if (decl_is_circular(field) &&
              (emitting_fiber_fixed_point_loop ||
               current_aug_graph->global_state->requires_fixed_point_loop ||
               current_aug_graph->global_state->requires_circular_update_support)) {
            o << ", changed";
          }
          o << ")";
          break;
        case KEYfuncall:
          field = field_ref_p(lhs);
          if (field == 0) {
            fatal_error("what sort of assignment lhs: %d", tnode_line_number(assign));
          }
          o << "a_" << decl_name(field) << DEREF;
          if (debug) {
            o << "assign";
          } else {
            o << "set";
          }
          o << "(";
          dump_Expression(field_ref_object(lhs), o);
          o << ", ";
          dump_Expression(rhs, o);
          if (decl_is_circular(field) &&
              (emitting_fiber_fixed_point_loop ||
               current_aug_graph->global_state->requires_fixed_point_loop ||
               current_aug_graph->global_state->requires_circular_update_support)) {
            o << ", changed";
          }
          o << ");\n";
          break;
        default:
          fatal_error("what sort of assignment lhs: %d", tnode_line_number(assign));
      }
      return;
    }

    if (in->node == 0 && ad != NULL) {
      if (rhs) {
        if (Declaration_info(ad)->decl_flags & LOCAL_ATTRIBUTE_FLAG) {
          o << "a" << LOCAL_UNIQUE_PREFIX(ad) << "_" << asym << DEREF;
          if (debug) {
            o << "assign";
          } else {
            o << "set";
          }
          o << "(anchor,";
          dump_Expression(rhs, o);
          o << ");\n";
        } else {
          int i = LOCAL_UNIQUE_PREFIX(ad);
          if (i == 0) {
            if (!def_is_constant(value_decl_def(ad))) {
              if (include_comments) {
                o << "// v_" << asym << " is assigned/initialized by default.\n";
              }
            } else {
              if (include_comments) {
                o << "// v_" << asym << " is initialized in module.\n";
              }
            }
          } else {
            o << "v" << i << "_" << asym << " = ";
            dump_Expression(rhs, o);
            o << "; // local\n";
          }
        }
      } else {
        if (!direction_is_collection(some_value_decl_direction(ad))) {
          aps_warning(ad, "Local attribute %s is apparently undefined", decl_name(ad));
        }
        if (include_comments) {
          o << "// " << in << " is ready now\n";
        }
      }
      return;
    } else if (node_is_syntax) {
      if (ATTR_DECL_IS_SHARED_INFO(ad)) {
        if (include_comments) {
          o << "// shared info for " << decl_name(in->node) << " is ready.\n";
        }
      } else if (ATTR_DECL_IS_UP_DOWN(ad)) {
        if (include_comments) {
          o << "// " << decl_name(in->node) << "." << decl_name(ad) << " implicit.\n";
        }
      } else if (rhs) {
        if (Declaration_KEY(in->node) == KEYfunction_decl) {
          Direction ad_dir = some_value_decl_direction(ad);
          if (direction_is_collection(ad_dir)) {
            std::cout << "Not expecting collection here!\n";
            o << "v_" << asym << " = somehow_combine(v_" << asym << ",";
            dump_Expression(rhs, o);
            o << ");\n";
          } else {
            int i = LOCAL_UNIQUE_PREFIX(ad);
            if (i == 0) {
              o << "v_" << asym << " = ";
              dump_Expression(rhs, o);
              o << "; // function\n";
            } else {
              o << "v" << i << "_" << asym << " = ";
              dump_Expression(rhs, o);
              o << ";\n";
            }
          }
        } else {
          o << "a_" << asym << DEREF;
          if (debug) {
            o << "assign";
          } else {
            o << "set";
          }
          o << "(v_" << decl_name(in->node) << ",";
          dump_Expression(rhs, o);
          o << ");\n";
        }
      } else {
        aps_warning(in->node, "Attribute %s.%s is apparently undefined", decl_name(in->node), symbol_name(asym));
        if (include_comments) {
          o << "// " << in << " is ready.\n";
        }
      }
      return;
    } else if (Declaration_KEY(in->node) == KEYvalue_decl) {
      if (rhs) {
        o << "a_" << asym << DEREF;
        if (debug) {
          o << "assign";
        } else {
          o << "set";
        }
        o << "(v_" << decl_name(in->node) << ",";
        dump_Expression(rhs, o);
        o << ");\n";
      } else {
        if (include_comments) {
          o << "// " << in << " is ready now.\n";
        }
      }
      return;
    }
  }

  void dump_rhs_instance_helper(AUG_GRAPH* aug_graph, synth_util::BlockItem* item, INSTANCE* instance, ostream& o) {
    if (item == NULL) {
      if (include_comments) {
        o << "// " << instance << " is ready now.\n";
      }
      return;
    }

    if (item->key == synth_util::KEY_BLOCK_ITEM_INSTANCE) {
      synth_util::BlockItemInstance* bi = reinterpret_cast<synth_util::BlockItemInstance*>(item);

      if (bi->instance != instance && bi->next != NULL) {
        dump_rhs_instance_helper(aug_graph, bi->next, instance, o);
        return;
      }

      vector<std::set<Expression>> all_assignments = synth_util::make_instance_assignments(current_aug_graph, current_blocks);
      std::set<Expression> relevant_assignments = all_assignments[instance->index];

      if (!relevant_assignments.empty()) {
        vector<Expression> valid_rhs;
        for (auto it = relevant_assignments.begin(); it != relevant_assignments.end(); it++) {
          if (*it != NULL) {
            valid_rhs.push_back(*it);
          }
        }

        if (!valid_rhs.empty()) {
          if (instance->fibered_attr.fiber != NULL) {
            for (auto it = valid_rhs.begin(); it != valid_rhs.end(); it++) {
              if (it != valid_rhs.begin()) {
                o << ";\n" << indent();
              }
              dump_assignment(instance, *it, o);
            }
          } else if (valid_rhs.size() == 1) {
            dump_Expression(valid_rhs[0], o);
          } else {
            Declaration attr = instance->fibered_attr.attr;
            Direction attr_dir = some_value_decl_direction(attr);
            if (!direction_is_collection(attr_dir)) {
              fatal_error("Multiple RHS for non-collection attribute %s", decl_name(attr));
            }
            Type vt = Declaration_KEY(attr) == KEYattribute_decl ? function_type_return_type(attribute_decl_type(attr)) : value_decl_type(attr);
            for (size_t i = 0; i < valid_rhs.size() - 1; i++) {
              o << as_val(vt) << ".v_combine(";
            }
            dump_Expression(valid_rhs[0], o);
            for (size_t i = 1; i < valid_rhs.size(); i++) {
              o << ", ";
              dump_Expression(valid_rhs[i], o);
              o << ")";
            }
          }
          return;
        }
      }

      if (instance->fibered_attr.fiber != NULL) {
        return;
      }

      Declaration attr = instance->fibered_attr.attr;
      bool is_local_collection = direction_is_collection(some_value_decl_direction(attr));
      if (is_local_collection) {
        Type vt = infer_some_value_decl_type(attr);
        CanonicalType* ctype = canonical_type(vt);
        CanonicalSignatureSet csig_set = infer_canonical_signatures(ctype);
        bool is_combinable = false;
        for (int ci = 0; ci < csig_set->num_elements && !is_combinable; ci++) {
          CanonicalSignature* csig = (CanonicalSignature*)csig_set->elements[ci];
          Block body = some_class_decl_contents(csig->source_class);
          for (Declaration bd = first_Declaration(block_body(body)); bd; bd = DECL_NEXT(bd)) {
            if (strcmp(decl_name(bd), "combine") == 0) {
              is_combinable = true;
              break;
            }
          }
        }
        if (is_combinable) {
          o << as_val(vt) << ".v_initial";
          if (include_comments) {
            o << " /* local collection " << decl_name(attr) << ": no direct assignment, using initial */";
          }
          return;
        }
      }

      o << "throw Evaluation.UndefinedAttributeException(\""
        << synth_util::instance_to_string(instance) << "\")";
    } else if (item->key == synth_util::KEY_BLOCK_ITEM_CONDITION) {
      synth_util::BlockItemCondition* cond = reinterpret_cast<synth_util::BlockItemCondition*>(item);
      bool visited_if_stmt = std::find(dumped_conditional_block_items.begin(), dumped_conditional_block_items.end(), item) != dumped_conditional_block_items.end();
      dumped_conditional_block_items.push_back(item);

      switch (ABSTRACT_APS_tnode_phylum(cond->condition)) {
        case KEYDeclaration: {
          Declaration if_stmt = (Declaration)cond->condition;
          if (Declaration_KEY(if_stmt) != KEYif_stmt) {
            fatal_error("expected if statement, got %s %d", decl_name(if_stmt), Declaration_info(if_stmt));
          }

          if (!edgeset_kind(current_aug_graph->graph[cond->instance->index * current_aug_graph->instances.length + instance->index])) {
            printf("\n");
            print_instance(cond->instance, stdout);
            printf(" does not affect ");
            print_instance(instance, stdout);
            printf("\n");
            fatal_error("crashed since instance not affected by condition");
          }

          if (!visited_if_stmt) {
            o << "if (";
            dump_Expression(if_stmt_cond(if_stmt), o);
            o << ") {\n";
            nesting_level++;
          }
          current_blocks.push_back(if_stmt_if_true(if_stmt));
          if (!visited_if_stmt) {
            o << indent();
          }

          vector<INSTANCE*> saved_instances_before_true_branch(dumped_instances);
          dump_rhs_instance_helper(aug_graph, cond->next_positive, instance, o);
          dumped_instances = saved_instances_before_true_branch;
          current_blocks.pop_back();

          if (!visited_if_stmt) {
            o << "\n";
            nesting_level--;
            o << indent() << "} else {\n";
            nesting_level++;
          }
          current_blocks.push_back(if_stmt_if_false(if_stmt));
          if (!visited_if_stmt) {
            o << indent();
          }

          vector<INSTANCE*> saved_instances_before_false_branch(dumped_instances);
          dump_rhs_instance_helper(aug_graph, cond->next_negative, instance, o);
          dumped_instances = saved_instances_before_false_branch;

          current_blocks.pop_back();
          if (!visited_if_stmt) {
            nesting_level--;
            o << "\n";
            o << indent() << "}";
          }
          break;
        }
        case KEYMatch: {
          Match m = (Match)cond->condition;
          Pattern p = matcher_pat(m);
          Declaration header = Match_info(m)->header;
          bool is_for = Declaration_KEY(header) == KEYfor_stmt;
          SequenceForPattern sequence_patterns = {};
          if (sequence_for_pattern(p, &sequence_patterns)) {
            unsigned number = get_match_index(m);
            Expression value = is_for
                ? for_stmt_expr(header) : case_stmt_expr(header);
            Declaration attr = instance->fibered_attr.attr;
            Type value_type = Declaration_KEY(attr) == KEYattribute_decl
                ? function_type_return_type(attribute_decl_type(attr))
                : value_decl_type(attr);
            dump_sequence_candidates(p, value, sequence_patterns, number, o);
            if (is_for) {
              o << ".foldLeft(" << as_val(value_type)
                << ".v_initial) { (v_for_result" << number
                << ", v_sequence_element" << number << ") =>\n";
              nesting_level++;
              o << indent() << "v_sequence_element" << number << " match {\n";
            } else {
              o << ".collectFirst {\n";
            }
            nesting_level++;
            o << indent() << "case ";
            dump_sequence_element_patterns(sequence_patterns.elements, o);
            o << " => ";
            if (is_for) {
              o << as_val(value_type) << ".v_combine(v_for_result"
                << number << ", ";
            }
            vector<INSTANCE*> saved_instances(dumped_instances);
            current_blocks.push_back(matcher_body(m));
            dump_rhs_instance_helper(aug_graph, cond->next_positive, instance, o);
            current_blocks.pop_back();
            dumped_instances = saved_instances;
            if (is_for) {
              o << ")\n";
              o << indent() << "case _ => v_for_result" << number << "\n";
              nesting_level--;
              o << indent() << "}\n";
            } else {
              o << "\n";
              nesting_level--;
              o << indent() << "}.getOrElse {\n";
              nesting_level++;
              current_blocks.push_back(MATCH_NEXT(m) ? 0 : case_stmt_default(header));
              o << indent();
              dump_rhs_instance_helper(aug_graph, cond->next_negative, instance, o);
              current_blocks.pop_back();
              dumped_instances = saved_instances;
              o << "\n";
            }
            nesting_level--;
            o << indent() << "}";
            break;
          }
          if (is_for) {
            fatal_error("unsupported pattern in synthesized for statement");
          }
          if (m == first_Match(case_stmt_matchers(header))) {
            Expression e = case_stmt_expr(header);
            o << "{\n";
            nesting_level++;
            o << indent() << "val node" << instance->index << " = ";
            dump_Expression(e, o);
            o << ";\n";
          }
          o << indent() << "node" << instance->index << " match {\n";
          nesting_level++;
          o << indent() << "case " << p << " => {\n";
          nesting_level += 1;

          Block if_true = matcher_body(m);
          Block if_false = MATCH_NEXT(m) ? 0 : case_stmt_default(header);

          current_blocks.push_back(if_true);
          o << indent();
          dump_rhs_instance_helper(aug_graph, cond->next_positive, instance, o);
          o << "\n";
          current_blocks.pop_back();

          nesting_level--;
          o << indent() << "}\n";
          o << indent() << "case _ => {\n";
          nesting_level++;

          current_blocks.push_back(if_false);
          o << indent();
          dump_rhs_instance_helper(aug_graph, cond->next_negative, instance, o);
          o << "\n";
          current_blocks.pop_back();

          nesting_level--;
          o << indent() << "}\n";
          nesting_level--;
          o << indent() << "}\n";
          if (m == first_Match(case_stmt_matchers(header))) {
            nesting_level--;
            o << indent() << "}";
          }
          break;
        }
        default:
          fatal_error("unhandled if statement type");
          break;
      }
    }
  }

  bool try_dump_funcall(Expression e, ostream& o) override { return synth_util::try_dump_funcall(e, current_aug_graph, this, o); }

  void dump_synth_instance(INSTANCE* instance, ostream& o) override {
    bool already_dumped = false;
    if (std::find(dumped_instances.begin(), dumped_instances.end(), instance) != dumped_instances.end()) {
      already_dumped = true;
    } else {
      dumped_instances.push_back(instance);
    }

    AUG_GRAPH* aug_graph = current_aug_graph;
    synth_util::BlockItem* block = synth_util::find_surrounding_block(current_scope_block, instance);

    Declaration node = instance->node;
    bool is_parent_instance = synth_util::instance_is_parent(instance, current_aug_graph);

    bool is_synthesized = synth_util::instance_is_synthesized(instance);
    bool is_inherited = synth_util::instance_is_inherited(instance);
    bool is_function_call_result =
      synth_util::instance_is_function_call_result(instance);
    bool has_dependency_cycle = edgeset_kind(current_aug_graph->graph[instance->index * current_aug_graph->instances.length + instance->index]);
    bool is_match_formal = synth_util::is_match_formal(instance->fibered_attr.attr);
    bool is_available = is_match_formal || is_inherited;

    if (has_dependency_cycle && already_dumped && !is_available) {
      o << "/* circular dependency detected for " << instance << ", dumping as attribute access */ ";
      o << synth_util::instance_to_attr(instance) << ".get(";
      if (instance->node == NULL) {
        o << "node";
      } else {
        o << "v_" << decl_name(instance->node);
      }
      o << ")";
      return;
    } else if (is_match_formal) {
      o << "v_" << synth_util::instance_to_string(
          instance, emitting_completion || current_synth_functions_state->is_phylum_instance);
    } else if (is_inherited) {
      if (instance->fibered_attr.fiber != NULL &&
          fiber_is_reverse(instance->fibered_attr.fiber)) {
        dump_rhs_instance_helper(aug_graph, block, instance, o);
      } else if (is_parent_instance) {
        o << "v_" << synth_util::instance_to_string(
            instance, emitting_completion || current_synth_functions_state->is_phylum_instance);
        if (!emitting_completion && current_synth_functions_state->is_phylum_instance &&
            !synth_util::synth_function_has_regular_dependency(
                current_synth_functions_state, instance)) {
          o << "(node)";
        }
      } else {
        block = synth_util::linearize_block(aug_graph, instance);
        dump_rhs_instance_helper(aug_graph, block, instance, o);
      }
    } else if (is_function_call_result) {
      dump_rhs_instance_helper(aug_graph, block, instance, o);
    } else if (is_synthesized) {
      if (is_parent_instance) {
        dump_rhs_instance_helper(aug_graph, block, instance, o);
      } else {
        for (auto it = synth_functions_states.begin(); it != synth_functions_states.end(); it++) {
          synth_util::SynthFunctionState* synth_function_state = *it;
          if (fibered_attr_equal(&synth_function_state->source->fibered_attr, &instance->fibered_attr)) {
            o << "eval_" << synth_function_state->fdecl_name << "(\n";
            int saved_nesting = nesting_level;
            nesting_level = std::max(nesting_level + 2, 2);
            o << indent() << "v_" << decl_name(node);

            const std::vector<INSTANCE*>& dependencies = synth_function_state->regular_dependencies;
            for (auto dep_it = dependencies.begin(); dep_it != dependencies.end(); dep_it++) {
              INSTANCE* source_instance = *dep_it;
              if (synth_util::should_skip_synth_dependency(source_instance)) {
                continue;
              }

              for (int i = 0; i < current_aug_graph->instances.length; i++) {
                INSTANCE* in = &current_aug_graph->instances.array[i];
                if (in->node == node && fibered_attr_equal(&in->fibered_attr, &source_instance->fibered_attr)) {
                  o << ",\n" << indent();
                  dump_synth_instance(in, o);
                }
              }
            }
            nesting_level = saved_nesting;

            o << "\n" << indent() << ")";
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
};

#ifndef APS_FIBER_SCHEDULE_TEST
Implementation* synth_impl = synth_impl_ptr = new SynthImpl();
#endif

#endif  // APS2SCALA
