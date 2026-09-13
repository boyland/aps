#include "synth-util.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <sstream>
#include <stack>
#include <vector>

#include "aps-scc.h"
#include "dump.h"
#include "implement.h"

namespace synth_util {

const std::string LOOP_VAR = "isInsideFixedPoint";
const std::string RESULT_VAR_PREFIX = "result";

void emit_loop_implicits(std::ostream& output, const std::string& flag) {
  output << indent() << "implicit val " << LOOP_VAR << ": Boolean = true;\n";
  output << indent() << "implicit val changed: AtomicBoolean = " << flag << ";\n";
}

void emit_fixed_point_loop_start(std::ostream& output, const std::string& flag) {
  output << indent() << "val " << flag << " = new AtomicBoolean(true);\n";
  output << indent() << "while (" << flag << ".get) {\n";
  ++nesting_level;
  output << indent() << flag << ".set(false);\n";
  emit_loop_implicits(output, flag);
}

void emit_fixed_point_loop_end(std::ostream& output) {
  --nesting_level;
  output << indent() << "}\n";
}

static enum instance_direction instance_direction_for(INSTANCE* instance) {
  enum instance_direction direction = fibered_attr_direction(&instance->fibered_attr);
  if (instance->node == NULL || DECL_IS_LHS(instance->node) || DECL_IS_RHS(instance->node)) {
    return direction;
  }
  if (DECL_IS_LOCAL(instance->node)) {
    return instance_local;
  }
  fatal_error("%d: unknown attributed node", tnode_line_number(instance->node));
  return direction;
}

bool instance_is_synthesized(INSTANCE* instance) { return instance_direction_for(instance) == instance_outward; }

bool instance_is_inherited(INSTANCE* instance) { return instance_direction_for(instance) == instance_inward; }

bool find_instance(AUG_GRAPH* graph, Declaration node, const FIBERED_ATTRIBUTE& attribute, INSTANCE** result) {
  for (int index = 0; index < graph->instances.length; ++index) {
    INSTANCE* instance = &graph->instances.array[index];
    FIBERED_ATTRIBUTE candidate = attribute;
    if (instance->node == node && fibered_attr_equal(&instance->fibered_attr, &candidate)) {
      *result = instance;
      return true;
    }
  }
  return false;
}

bool state_uses_fibers(STATE* state) {
  for (int index = 0; index < state->match_rules.length; ++index) {
    AUG_GRAPH* graph = &state->aug_graphs[index];
    for (int instance_index = 0; instance_index < graph->instances.length; ++instance_index) {
      if (graph->instances.array[instance_index].fibered_attr.fiber != NULL) {
        return true;
      }
    }
  }
  for (int index = 0; index < state->phyla.length; ++index) {
    PHY_GRAPH* graph = &state->phy_graphs[index];
    for (int instance_index = 0; instance_index < graph->instances.length; ++instance_index) {
      if (graph->instances.array[instance_index].fibered_attr.fiber != NULL) {
        return true;
      }
    }
  }
  return false;
}

bool instance_is_pure_shared_info(INSTANCE* instance) { return instance->fibered_attr.fiber == NULL && ATTR_DECL_IS_SHARED_INFO(instance->fibered_attr.attr); }

static std::string attr_to_string(Declaration attribute) { return ATTR_DECL_IS_SHARED_INFO(attribute) ? "sharedinfo" : decl_name(attribute); }

static std::string fiber_to_string(FIBER fiber) {
  std::stringstream output;
  while (fiber != NULL && fiber->field != NULL) {
    std::string field = decl_name(fiber->field);
    field.erase(std::remove(field.begin(), field.end(), '!'), field.end());
    output << field;
    fiber = fiber->shorter;
    if (fiber->field != NULL) {
      output << "_";
    }
  }
  return output.str();
}

std::string instance_to_string(INSTANCE* instance, bool trim_node) {
  std::vector<std::string> parts;
  Declaration node = instance->node;
  if (!trim_node && node != NULL) {
    parts.push_back(Declaration_KEY(node) == KEYpragma_call ? symbol_name(pragma_call_name(node)) : decl_name(node));
  }
  if (instance->fibered_attr.attr != NULL) {
    parts.push_back(attr_to_string(instance->fibered_attr.attr));
  }
  if (instance->fibered_attr.fiber != NULL) {
    parts.push_back(fiber_to_string(instance->fibered_attr.fiber));
  }
  return std::accumulate(std::next(parts.begin()), parts.end(), parts[0], [](std::string left, std::string right) { return left + "_" + right; });
}

std::string instance_to_string_with_nodetype(Declaration node_type, INSTANCE* instance) {
  std::stringstream output;
  Declaration attribute = instance->fibered_attr.attr;
  if (Declaration_KEY(attribute) == KEYvalue_decl && LOCAL_UNIQUE_PREFIX(attribute) != 0) {
    output << "a" << LOCAL_UNIQUE_PREFIX(attribute) << "_";
  }
  output << decl_name(node_type) << "_" << instance_to_string(instance);
  return output.str();
}

std::string instance_to_attr(INSTANCE* instance) {
  Declaration attribute = instance->fibered_attr.attr;
  Declaration field = instance->fibered_attr.fiber != NULL ? instance->fibered_attr.fiber->field : NULL;
  std::stringstream output;
  if (Declaration_KEY(attribute) == KEYvalue_decl && LOCAL_UNIQUE_PREFIX(attribute) != 0) {
    output << "a" << LOCAL_UNIQUE_PREFIX(attribute);
  } else {
    output << "a";
  }
  if (attribute != NULL && !ATTR_DECL_IS_SHARED_INFO(attribute)) {
    output << "_" << decl_name(attribute);
  }
  if (field != NULL) {
    std::string field_name = decl_name(field);
    field_name.erase(std::remove(field_name.begin(), field_name.end(), '!'), field_name.end());
    output << "_" << field_name;
  }
  return output.str();
}

bool is_match_formal(void* node) {
  Declaration formal = NULL;
  void* match = NULL;
  return check_surrounding_decl(node, KEYnormal_formal, &formal) && check_surrounding_node(node, KEYMatch, &match);
}

bool should_skip_synth_dependency(INSTANCE* instance) { return instance->fibered_attr.fiber != NULL || if_rule_p(instance->fibered_attr.attr) || is_match_formal(instance->fibered_attr.attr); }

static std::vector<INSTANCE*> collect_phylum_graph_attr_dependencies(PHY_GRAPH* phylum_graph, INSTANCE* sink_instance) {
  std::vector<INSTANCE*> result;
  int instance_count = phylum_graph->instances.length;

  for (int index = 0; index < instance_count; ++index) {
    INSTANCE* source_instance = &phylum_graph->instances.array[index];
    if (!instance_is_pure_shared_info(source_instance) && source_instance->index != sink_instance->index && phylum_graph->mingraph[source_instance->index * instance_count + sink_instance->index]) {
      result.push_back(source_instance);
    }
  }
  return result;
}

bool instance_is_function_call_result(INSTANCE* instance) {
  if (instance->node == NULL || ABSTRACT_APS_tnode_phylum(instance->node) != KEYDeclaration || Declaration_KEY(instance->node) != KEYpragma_call) {
    return false;
  }
  Declaration function = Declaration_info(instance->node)->proxy_fdecl;
  return function != NULL && instance->fibered_attr.fiber == NULL &&
         instance->fibered_attr.attr == some_function_decl_result(function);
}

static bool is_some_function_decl(Declaration declaration) {
  switch (Declaration_KEY(declaration)) {
    case KEYsome_function_decl:
      return true;
    default:
      return false;
  }
}

static bool is_formal(Declaration declaration) {
  switch (Declaration_KEY(declaration)) {
    case KEYformal:
      return true;
    default:
      return false;
  }
}

static std::vector<INSTANCE*> collect_aug_graph_attr_dependencies(AUG_GRAPH* aug_graph, INSTANCE* sink_instance) {
  std::vector<INSTANCE*> result;
  int instance_count = aug_graph->instances.length;

  for (int index = 0; index < instance_count; ++index) {
    INSTANCE* source_instance = &aug_graph->instances.array[index];
    if (!instance_is_pure_shared_info(source_instance) && source_instance->index != sink_instance->index && !instance_is_function_call_result(source_instance) &&
        edgeset_kind(aug_graph->graph[source_instance->index * instance_count + sink_instance->index])) {
      result.push_back(source_instance);
    }
  }
  return result;
}

static std::vector<INSTANCE*> collect_field_assign_dependencies(AUG_GRAPH* graph,
                                                                INSTANCE* owner) {
  std::vector<INSTANCE*> result;
  Block body = matcher_body(top_level_match_m(graph->match_rule));
  for (Declaration declaration = first_Declaration(block_body(body)); declaration;
       declaration = DECL_NEXT(declaration)) {
    if (Declaration_KEY(declaration) != KEYnormal_assign) {
      continue;
    }
    Expression lhs = assign_lhs(declaration);
    if (Expression_KEY(lhs) != KEYfuncall || field_ref_p(lhs) == NULL) {
      continue;
    }
    Expression object = field_ref_object(lhs);
    if (Expression_KEY(object) != KEYvalue_use ||
        USE_DECL(value_use_use(object)) != owner->fibered_attr.attr) {
      continue;
    }
    INSTANCE* field_instance = Expression_info(assign_rhs(declaration))->value_for;
    if (field_instance == NULL) {
      continue;
    }
    std::vector<INSTANCE*> dependencies =
        collect_aug_graph_attr_dependencies(graph, field_instance);
    for (INSTANCE* dependency_instance : dependencies) {
      if (dependency_instance->index != owner->index &&
          !should_skip_synth_dependency(dependency_instance) &&
          instance_direction_for(dependency_instance) != instance_local &&
          !(dependency_instance->node != NULL && dependency_instance->node != graph->lhs_decl) &&
          std::find(result.begin(), result.end(), dependency_instance) == result.end()) {
        result.push_back(dependency_instance);
      }
    }
  }
  return result;
}

static std::vector<AUG_GRAPH*> collect_lhs_aug_graphs(STATE* state, PHY_GRAPH* phylum_graph) {
  std::vector<AUG_GRAPH*> result;
  for (int index = 0; index < state->match_rules.length; ++index) {
    AUG_GRAPH* aug_graph = &state->aug_graphs[index];
    PHY_GRAPH* lhs_graph = Declaration_info(aug_graph->lhs_decl)->node_phy_graph;
    if (lhs_graph == phylum_graph && !is_some_function_decl(aug_graph->lhs_decl)) {
      result.push_back(aug_graph);
    }
  }
  return result;
}

std::vector<SynthFunctionState*> build_synth_function_states(
  STATE* state, bool include_field_assign_dependencies) {
  std::vector<SynthFunctionState*> result;

  for (int phylum_index = 0; phylum_index < state->phyla.length; ++phylum_index) {
    PHY_GRAPH* phylum_graph = &state->phy_graphs[phylum_index];
    if (Declaration_KEY(phylum_graph->phylum) != KEYphylum_decl) {
      continue;
    }

    for (int instance_index = 0; instance_index < phylum_graph->instances.length; ++instance_index) {
      INSTANCE* instance = &phylum_graph->instances.array[instance_index];
      if (!instance_is_synthesized(instance)) {
        continue;
      }

      SynthFunctionState* function_state = new SynthFunctionState();
      function_state->fdecl_name = instance_to_string_with_nodetype(phylum_graph->phylum, instance);
      function_state->source = instance;
      function_state->source_phy_graph = phylum_graph;
      function_state->is_phylum_instance = true;
      function_state->is_side_effect_evaluation = instance->fibered_attr.fiber != NULL || ATTR_DECL_IS_SHARED_INFO(instance->fibered_attr.attr);
      function_state->regular_dependencies = collect_phylum_graph_attr_dependencies(phylum_graph, instance);
      function_state->aug_graphs = collect_lhs_aug_graphs(state, phylum_graph);
      result.push_back(function_state);
    }
  }

  for (int graph_index = 0; graph_index < state->match_rules.length; ++graph_index) {
    AUG_GRAPH* aug_graph = &state->aug_graphs[graph_index];
    if (is_some_function_decl(aug_graph->lhs_decl)) {
      continue;
    }

    for (int instance_index = 0; instance_index < aug_graph->instances.length; ++instance_index) {
      INSTANCE* instance = &aug_graph->instances.array[instance_index];
      if (instance_direction_for(instance) != instance_local || instance->fibered_attr.fiber != NULL || if_rule_p(instance->fibered_attr.attr) || is_formal(instance->fibered_attr.attr)) {
        continue;
      }

      Declaration attribute = instance->fibered_attr.attr;
      PHY_GRAPH* phylum_graph = Declaration_info(aug_graph->lhs_decl)->node_phy_graph;
      Declaration type_decl = canonical_type_decl(canonical_type(value_decl_type(attribute)));

      SynthFunctionState* function_state = new SynthFunctionState();
      function_state->fdecl_name = instance_to_string_with_nodetype(type_decl, instance);
      function_state->source = instance;
      function_state->source_phy_graph = phylum_graph;
      function_state->is_phylum_instance = false;
      function_state->is_side_effect_evaluation = ATTR_DECL_IS_SHARED_INFO(instance->fibered_attr.attr);
      function_state->regular_dependencies = collect_aug_graph_attr_dependencies(aug_graph, instance);
      if (include_field_assign_dependencies) {
        std::vector<INSTANCE*> field_dependencies =
            collect_field_assign_dependencies(aug_graph, instance);
        for (INSTANCE* dependency_instance : field_dependencies) {
          if (std::find(function_state->regular_dependencies.begin(),
                        function_state->regular_dependencies.end(), dependency_instance) ==
              function_state->regular_dependencies.end()) {
            function_state->regular_dependencies.push_back(dependency_instance);
          }
        }
      }
      function_state->aug_graphs.push_back(aug_graph);
      result.push_back(function_state);
    }
  }

  return result;
}

void destroy_synth_function_states(const std::vector<SynthFunctionState*>& states) {
  for (auto iterator = states.begin(); iterator != states.end(); ++iterator) {
    delete *iterator;
  }
}

void implement_value_use(Declaration declaration, AUG_GRAPH* graph, const std::vector<SynthFunctionState*>& states, SynthImplementation* implementation, std::ostream& output) {
  int flags = Declaration_info(declaration)->decl_flags;
  if (flags & LOCAL_ATTRIBUTE_FLAG) {
    int instance_index = Declaration_info(declaration)->instance_index;
    INSTANCE* instance = &graph->instances.array[instance_index];
    Declaration type_decl = canonical_type_decl(canonical_type(value_decl_type(declaration)));
    std::string target_name = instance_to_string_with_nodetype(type_decl, instance);

    output << "eval_" << target_name << "(\n";
    int saved_nesting = nesting_level;
    nesting_level = std::max(nesting_level + 2, 1);
    output << indent() << "node";

    for (auto state_iterator = states.begin(); state_iterator != states.end(); ++state_iterator) {
      SynthFunctionState* state = *state_iterator;
      if (state->fdecl_name != target_name) {
        continue;
      }
      for (auto dependency_iterator = state->regular_dependencies.begin(); dependency_iterator != state->regular_dependencies.end(); ++dependency_iterator) {
        INSTANCE* source_instance = *dependency_iterator;
        if (!should_skip_synth_dependency(source_instance)) {
          output << ",\n" << indent();
          implementation->dump_synth_instance(source_instance, output);
        }
      }
      break;
    }

    nesting_level = saved_nesting;
    output << "\n" << indent() << ")";
  } else if (flags & ATTRIBUTE_DECL_FLAG) {
    output << "v_" << decl_name(declaration);
  } else if (flags & LOCAL_VALUE_FLAG) {
    output << "v" << LOCAL_UNIQUE_PREFIX(declaration) << "_" << decl_name(declaration);
  } else {
    aps_error(declaration, "internal_error: What is special about this?");
  }
}

bool try_dump_funcall(Expression expression, AUG_GRAPH* graph, SynthImplementation* implementation, std::ostream& output) {
  Expression function = funcall_f(expression);
  if (Expression_KEY(function) == KEYvalue_use) {
    Declaration used_declaration = USE_DECL(value_use_use(function));
    if (Declaration_KEY(used_declaration) == KEYconstructor_decl) {
      Type constructor_type = constructor_decl_type(used_declaration);
      Declaration return_declaration = first_Declaration(function_type_return_values(constructor_type));
      Type return_type = value_decl_type(return_declaration);
      Declaration type_decl = USE_DECL(type_use_use(return_type));
      if (Declaration_KEY(type_decl) == KEYphylum_decl) {
        Declaration top_level_match;
        bool inside_match = check_surrounding_decl(expression, KEYtop_level_match, &top_level_match);
        dump_Expression(function, output);
        output << "(";
        bool first = true;
        FOR_SEQUENCE(Expression, argument, Actuals, funcall_actuals(expression), if (first) first = false; else output << ",\n"; dump_Expression(argument, output));
        if (first_Actual(funcall_actuals(expression)) != NULL) {
          output << ", ";
        }
        output << (inside_match ? "node" : "null") << ")";
        return true;
      }
    }
  }

  Declaration attribute = attr_ref_p(expression);
  if (attribute == NULL) {
    return false;
  }
  Declaration node = USE_DECL(value_use_use(first_Actual(funcall_actuals(expression))));
  FIBERED_ATTRIBUTE fibered_attribute = {attribute, NULL};
  INSTANCE* instance;
  if (find_instance(graph, node, fibered_attribute, &instance)) {
    implementation->dump_synth_instance(instance, output);
    return true;
  }
  fatal_error("failed to find instance");
  return false;
}

void dump_attribute_type(INSTANCE* instance, std::ostream& output) {
  CanonicalType* type = canonical_type(infer_some_value_decl_type(instance->fibered_attr.attr));
  switch (type->key) {
    case KEY_CANONICAL_USE:
      output << "T_" << decl_name(canonical_type_decl(type));
      break;
    case KEY_CANONICAL_FUNC: {
      struct Canonical_function_type* function_type = (struct Canonical_function_type*)type;
      output << "T_" << decl_name(canonical_type_decl(function_type->return_type));
      break;
    }
    default:
      break;
  }
}

bool synth_function_is_circular(SynthFunctionState* state) {
  if (state->is_side_effect_evaluation || instance_is_pure_shared_info(state->source)) {
    return false;
  }
  for (auto iterator = state->aug_graphs.begin(); iterator != state->aug_graphs.end(); ++iterator) {
    AUG_GRAPH* graph = *iterator;
    INSTANCE* instance = NULL;
    if (state->is_phylum_instance) {
      if (!find_instance(graph, graph->lhs_decl, state->source->fibered_attr, &instance)) {
        continue;
      }
    } else {
      instance = state->source;
    }
    if (instance_circular(instance)) {
      return true;
    }
  }
  return false;
}

bool synth_function_has_regular_dependency(SynthFunctionState* state,
                                           INSTANCE* instance) {
  for (INSTANCE* dependency_instance : state->regular_dependencies) {
    if (fibered_attr_equal(&dependency_instance->fibered_attr,
                           &instance->fibered_attr)) {
      return true;
    }
  }
  return false;
}

static bool instance_is_child(INSTANCE* instance, AUG_GRAPH* graph) { return instance->node != NULL && instance->node != graph->lhs_decl; }

bool instance_is_parent(INSTANCE* instance, AUG_GRAPH* graph) { return instance->node != NULL && instance->node == graph->lhs_decl; }

static bool is_local_cycle_candidate(INSTANCE* instance, AUG_GRAPH* graph) {
  return instance_is_child(instance, graph) &&
         instance->fibered_attr.fiber == NULL &&
         !if_rule_p(instance->fibered_attr.attr) &&
         instance_is_synthesized(instance) && instance_circular(instance);
}

static bool is_in_same_cycle(INSTANCE* inherited, INSTANCE* instance,
                             AUG_GRAPH* graph) {
  int instance_count = graph->instances.length;
  bool instance_feeds_inherited =
      edgeset_kind(graph->graph[instance->index * instance_count + inherited->index]) &
      DEPENDENCY_MAYBE_DIRECT;
  return inherited != instance && inherited->node == instance->node &&
         inherited->fibered_attr.fiber == NULL &&
         instance_is_inherited(inherited) && instance_circular(inherited) &&
         instance_feeds_inherited;
}

std::vector<INSTANCE*> collect_child_cycle_instances(AUG_GRAPH* graph) {
  std::vector<INSTANCE*> result;
  int instance_count = graph->instances.length;
  for (int index = 0; index < instance_count; ++index) {
    INSTANCE* instance = &graph->instances.array[index];
    if (!is_local_cycle_candidate(instance, graph)) {
      continue;
    }
    for (int inherited_index = 0; inherited_index < instance_count;
         ++inherited_index) {
      if (is_in_same_cycle(&graph->instances.array[inherited_index], instance, graph)) {
        result.push_back(instance);
        break;
      }
    }
  }
  return result;
}

bool child_cycle_is_independent(AUG_GRAPH* graph, INSTANCE* cycle_instance) {
  int instance_count = graph->instances.length;
  std::vector<int> cycle_indices;
  cycle_indices.push_back(cycle_instance->index);
  for (int index = 0; index < instance_count; ++index) {
    if (is_in_same_cycle(&graph->instances.array[index], cycle_instance, graph)) {
      cycle_indices.push_back(index);
    }
  }

  for (int index = 0; index < instance_count; ++index) {
    INSTANCE* instance = &graph->instances.array[index];
    if ((instance->fibered_attr.fiber == NULL &&
         !ATTR_DECL_IS_SHARED_INFO(instance->fibered_attr.attr)) ||
        !instance_circular(instance)) {
      continue;
    }
    for (int cycle_index : cycle_indices) {
      if (edgeset_kind(graph->graph[index * instance_count + cycle_index]) ||
        edgeset_kind(graph->graph[cycle_index * instance_count + index])) {
        return false;
      }
    }
  }
  return true;
}

static bool component_precedes(AUG_GRAPH* graph, SCC_COMPONENT* source, SCC_COMPONENT* sink) {
  int instance_count = graph->instances.length;
  for (int source_index = 0; source_index < source->length; ++source_index) {
    INSTANCE* source_instance = static_cast<INSTANCE*>(source->array[source_index]);
    for (int sink_index = 0; sink_index < sink->length; ++sink_index) {
      INSTANCE* sink_instance = static_cast<INSTANCE*>(sink->array[sink_index]);
      if (!MERGED_CONDITION_IS_IMPOSSIBLE(instance_condition(source_instance), instance_condition(sink_instance)) && edgeset_kind(graph->graph[source_instance->index * instance_count + sink_instance->index])) {
        return true;
      }
    }
  }
  return false;
}

static bool component_is_relevant(AUG_GRAPH* graph, SCC_COMPONENT* component, INSTANCE* sink) {
  int instance_count = graph->instances.length;
  for (int index = 0; index < component->length; ++index) {
    INSTANCE* instance = static_cast<INSTANCE*>(component->array[index]);
    if (instance == sink || edgeset_kind(graph->graph[instance->index * instance_count + sink->index])) {
      return true;
    }
  }
  return false;
}

std::vector<std::vector<INSTANCE*>> collect_child_cycle_components(AUG_GRAPH* graph, INSTANCE* sink) {
  std::vector<std::vector<INSTANCE*>> result;
  if (graph->components == NULL) {
    set_aug_graph_components(graph);
  }
  int component_count = graph->components->length;
  std::vector<bool> scheduled(component_count, false);

  for (int scheduled_count = 0; scheduled_count < component_count;) {
    bool found_ready = false;
    for (int component_index = 0; component_index < component_count; ++component_index) {
      if (scheduled[component_index]) {
        continue;
      }

      SCC_COMPONENT* component = graph->components->array[component_index];
      bool ready = true;
      for (int predecessor_index = 0; predecessor_index < component_count && ready; ++predecessor_index) {
        if (predecessor_index != component_index && !scheduled[predecessor_index] && component_precedes(graph, graph->components->array[predecessor_index], component)) {
          ready = false;
        }
      }
      if (!ready) {
        continue;
      }

      scheduled[component_index] = true;
      ++scheduled_count;
      found_ready = true;

      if (component->length < 2 || !component_is_relevant(graph, component, sink)) {
        continue;
      }

      std::vector<INSTANCE*> child_instances;
      for (int instance_index = 0; instance_index < component->length; ++instance_index) {
        INSTANCE* instance = static_cast<INSTANCE*>(component->array[instance_index]);
        if (instance_is_child(instance, graph) && instance->fibered_attr.fiber == NULL && !if_rule_p(instance->fibered_attr.attr) && instance_is_synthesized(instance)) {
          child_instances.push_back(instance);
        }
      }
      if (!child_instances.empty()) {
        result.push_back(child_instances);
      }
    }
    if (!found_ready) {
      fatal_error("failed to order circular dependency classes in %s", aug_graph_name(graph));
    }
  }
  return result;
}

static Expression default_init(Default value) {
  switch (Default_KEY(value)) {
    case KEYsimple:
      return simple_value(value);
    case KEYcomposite:
      return composite_initial(value);
    default:
      return 0;
  }
}

std::vector<std::set<Expression>> make_instance_assignments(AUG_GRAPH* graph, const std::vector<Block>& blocks) {
  int instance_count = graph->instances.length;
  std::vector<std::set<Expression>> assignments(instance_count);
  for (int index = 0; index < instance_count; ++index) {
    INSTANCE* instance = &graph->instances.array[index];
    Declaration attribute = instance->fibered_attr.attr;
    if (attribute != NULL && instance->fibered_attr.fiber == NULL && ABSTRACT_APS_tnode_phylum(attribute) == KEYDeclaration) {
      switch (Declaration_KEY(attribute)) {
        case KEYattribute_decl:
          assignments[instance->index].insert(default_init(attribute_decl_default(attribute)));
          break;
        case KEYvalue_decl:
          assignments[instance->index].insert(default_init(value_decl_default(attribute)));
          break;
        default:
          break;
      }
    }
  }

  for (auto block_iterator = blocks.begin(); block_iterator != blocks.end(); ++block_iterator) {
    bool is_outermost = block_iterator == blocks.begin();
    std::vector<std::set<Expression>> scoped_assignments(assignments);
    for (int step = 1; step <= 2; ++step) {
      Declarations declarations = block_body(*block_iterator);
      for (Declaration declaration = first_Declaration(declarations); declaration; declaration = DECL_NEXT(declaration)) {
        switch (Declaration_KEY(declaration)) {
          case KEYnormal_assign: {
            INSTANCE* instance = Expression_info(assign_rhs(declaration))->value_for;
            if (instance != NULL) {
              if (instance->index >= instance_count) {
                fatal_error("bad index [normal_assign] for instance");
              }
              scoped_assignments[instance->index].clear();
              if (assign_rhs(declaration) == NULL) {
                printf("Warning: assignment to %s is empty\n", instance_to_string(instance).c_str());
              }
              scoped_assignments[instance->index].insert(assign_rhs(declaration));
            }
            break;
          }
          case KEYcollect_assign: {
            INSTANCE* instance = Expression_info(assign_rhs(declaration))->value_for;
            if (instance != NULL) {
              if (instance->index >= instance_count) {
                fatal_error("bad index [collection_assign] for instance");
              }
              if (step == 1 && is_outermost) {
                scoped_assignments[instance->index].clear();
              } else if (step == 2) {
                scoped_assignments[instance->index].insert(assign_rhs(declaration));
              }
            }
            break;
          }
          default:
            break;
        }
      }
    }
    assignments = scoped_assignments;
  }
  return assignments;
}

// We prefer to have IF statements at the beginning
static std::vector<INSTANCE*> sort_instances(AUG_GRAPH* graph) {
  std::vector<INSTANCE*> result;
  int instance_count = graph->instances.length;

  for (int index = 0; index < instance_count; ++index) {
    INSTANCE* instance = &graph->instances.array[index];
    if (!if_rule_p(instance->fibered_attr.attr)) {
      result.push_back(instance);
    }
  }
  for (int index = 0; index < instance_count; ++index) {
    INSTANCE* instance = &graph->instances.array[index];
    if (if_rule_p(instance->fibered_attr.attr)) {
      result.push_back(instance);
    }
  }

  return result;
}

static std::vector<std::vector<INSTANCE*>> direct_cycle_groups(AUG_GRAPH* graph) {
  int instance_count = graph->instances.length;
  SccGraph scc_graph;
  scc_graph_initialize(&scc_graph, instance_count);
  for (int index = 0; index < instance_count; ++index) {
    scc_graph_add_vertex(&scc_graph, &graph->instances.array[index]);
  }
  for (int source = 0; source < instance_count; ++source) {
    for (int sink = 0; sink < instance_count; ++sink) {
      if (source != sink &&
          (edgeset_kind(graph->graph[source * instance_count + sink]) &
           DEPENDENCY_MAYBE_DIRECT)) {
        scc_graph_add_edge(&scc_graph, &graph->instances.array[source],
                           &graph->instances.array[sink]);
      }
    }
  }

  SCC_COMPONENTS* components = scc_graph_components(&scc_graph);
  std::vector<std::vector<INSTANCE*>> groups;
  for (int component_index = 0; component_index < components->length;
       ++component_index) {
    SCC_COMPONENT* component = components->array[component_index];
    std::vector<INSTANCE*> group;
    for (int index = 0; index < component->length; ++index) {
      group.push_back(static_cast<INSTANCE*>(component->array[index]));
    }
    groups.push_back(group);
  }
  scc_graph_destroy(&scc_graph);
  return groups;
}

static BlockItem* linearize_block_helper(AUG_GRAPH* graph, const std::vector<INSTANCE*>& sorted_instances, bool* scheduled, CONDITION* condition, BlockItem* previous, int remaining, INSTANCE* sink, const std::vector<int>& component_of) {
  if (CONDITION_IS_IMPOSSIBLE(*condition)) {
    return NULL;
  }

  int instance_count = graph->instances.length;
  for (auto iterator = sorted_instances.begin(); iterator != sorted_instances.end(); ++iterator) {
    INSTANCE* instance = *iterator;
    int index = instance->index;

    if (scheduled[index]) {
      continue;
    }
    if (MERGED_CONDITION_IS_IMPOSSIBLE(*condition, instance_condition(instance))) {
      scheduled[index] = true;
      BlockItem* result = linearize_block_helper(graph, sorted_instances, scheduled, condition, previous, remaining - 1, sink, component_of);
      scheduled[index] = false;
      return result;
    }
    if (sink != instance && !edgeset_kind(graph->graph[index * instance_count + sink->index])) {
      scheduled[index] = true;
      BlockItem* result = linearize_block_helper(graph, sorted_instances, scheduled, condition, previous, remaining - 1, sink, component_of);
      scheduled[index] = false;
      return result;
    }

    bool ready = true;
    for (int dependency_index = 0; dependency_index < instance_count && ready; ++dependency_index) {
      INSTANCE* predecessor = &graph->instances.array[dependency_index];
      if (scheduled[dependency_index] || MERGED_CONDITION_IS_IMPOSSIBLE(instance_condition(instance), instance_condition(predecessor)) || !(edgeset_kind(graph->graph[dependency_index * instance_count + index]) & DEPENDENCY_MAYBE_DIRECT)) {
        continue;
      }
      // Ignore edges within the same direct cycle so its instances can be linearized.
      if (component_of[dependency_index] == component_of[index]) {
        continue;
      }
      ready = false;
    }
    if (!ready) {
      continue;
    }

    scheduled[index] = true;
    BlockItem* item;
    if (if_rule_p(instance->fibered_attr.attr)) {
      BlockItemCondition* conditional = static_cast<BlockItemCondition*>(std::malloc(sizeof(BlockItemCondition)));
      item = reinterpret_cast<BlockItem*>(conditional);
      conditional->key = KEY_BLOCK_ITEM_CONDITION;
      conditional->instance = instance;
      conditional->condition = instance->fibered_attr.attr;
      conditional->prev = previous;

      int condition_mask = 1 << if_rule_index(instance->fibered_attr.attr);
      condition->positive |= condition_mask;
      conditional->next_positive = linearize_block_helper(graph, sorted_instances, scheduled, condition, item, remaining - 1, sink, component_of);
      condition->positive &= ~condition_mask;
      condition->negative |= condition_mask;
      conditional->next_negative = linearize_block_helper(graph, sorted_instances, scheduled, condition, item, remaining - 1, sink, component_of);
      condition->negative &= ~condition_mask;
    } else {
      BlockItemInstance* linear = static_cast<BlockItemInstance*>(std::malloc(sizeof(BlockItemInstance)));
      item = reinterpret_cast<BlockItem*>(linear);
      linear->key = KEY_BLOCK_ITEM_INSTANCE;
      linear->instance = instance;
      linear->prev = previous;
      linear->next = linearize_block_helper(graph, sorted_instances, scheduled, condition, item, remaining - 1, sink, component_of);
    }
    scheduled[index] = false;
    return item;
  }

  if (remaining != 0) {
    fatal_error("failed to schedule some instances, remaining: %d", remaining);
  }
  return NULL;
}

// Given an augmented dependency graph, it linearizes
// the direct dependency schedule.
BlockItem* linearize_block(AUG_GRAPH* graph, INSTANCE* sink) {
  int instance_count = graph->instances.length;
  bool* scheduled = static_cast<bool*>(alloca(sizeof(bool) * instance_count));
  std::memset(scheduled, 0, sizeof(bool) * instance_count);

  CONDITION condition = {0, 0};
  std::vector<INSTANCE*> sorted_instances = sort_instances(graph);
  std::vector<std::vector<INSTANCE*>> groups = direct_cycle_groups(graph);
  std::vector<int> component_of(instance_count, -1);
  for (size_t group_index = 0; group_index < groups.size(); ++group_index) {
    for (INSTANCE* instance : groups[group_index]) {
      component_of[instance->index] = static_cast<int>(group_index);
    }
  }
  return linearize_block_helper(graph, sorted_instances, scheduled, &condition,
                                NULL, instance_count, sink, component_of);
}

void print_linearized_block(BlockItem* block, std::ostream& output) {
  if (block == NULL) {
    return;
  }

  output << indent() << block->instance << "\n";
  if (block->key == KEY_BLOCK_ITEM_CONDITION) {
    BlockItemCondition* condition = reinterpret_cast<BlockItemCondition*>(block);
    if (condition->prev != NULL && condition->prev->key != KEY_BLOCK_ITEM_CONDITION) {
      output << indent() << condition->prev->instance << "\n";
    }

    output << indent() << "IF\n";
    ++nesting_level;
    print_linearized_block(condition->next_positive, output);
    --nesting_level;
    output << indent() << "ELSE\n";
    ++nesting_level;
    print_linearized_block(condition->next_negative, output);
    --nesting_level;
  } else {
    print_linearized_block(reinterpret_cast<BlockItemInstance*>(block)->next, output);
  }
}

// Given an instance it traverses the direct dependency schedule
// trying to find the instance and if it sees the condition
// along the way, it returns that condition.
BlockItem* find_surrounding_block(BlockItem* block, INSTANCE* instance) {
  while (block != NULL) {
    if (block->key == KEY_BLOCK_ITEM_CONDITION) {
      return block;
    }
    if (block->instance == instance) {
      return block;
    }
    block = reinterpret_cast<BlockItemInstance*>(block)->next;
  }
  return NULL;
}

}  // namespace synth_util