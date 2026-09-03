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

static AUG_GRAPH* current_aug_graph = NULL;
static std::vector<synth_util::SynthFunctionState*> synth_functions_states;
static synth_util::SynthFunctionState* current_synth_functions_state = NULL;

static void* detect_program_fibers(void* scope, void* node) {
  bool* uses_fibers = static_cast<bool*>(scope);
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    STATE* state = (STATE*)Declaration_info((Declaration)node)->analysis_state;
    if (state != NULL && synth_util::state_uses_fibers(state)) {
      *uses_fibers = true;
    }
  }
  return scope;
}

#define DEREF "."

static SynthImplementation* farrow_impl_ptr;

static vector<Block> current_blocks;
static synth_util::BlockItem* current_scope_block;
static vector<synth_util::BlockItem*> dumped_conditional_block_items;
static vector<INSTANCE*> dumped_instances;

static void emit_start_phylum_evaluations(ostream& os, STATE* state) {
  PHY_GRAPH* start_graph = summary_graph_for(state, state->start_phylum);
  if (state->loop_required) {
    os << indent() << "implicit val " << synth_util::LOOP_VAR << ": Boolean = false;\n";
    os << indent() << "implicit val changed: AtomicBoolean = new AtomicBoolean(false);\n";
  }
  os << indent() << "for (root <- t_" << decl_name(state->start_phylum) << ".nodes) {\n";
  ++nesting_level;
  for (int index = 0; index < start_graph->instances.length; ++index) {
    INSTANCE* instance = &start_graph->instances.array[index];
    if (!synth_util::instance_is_synthesized(instance)) {
      continue;
    }
    os << indent() << "eval_" << synth_util::instance_to_string_with_nodetype(state->start_phylum, instance) << "(root);\n";
  }
  --nesting_level;
  os << indent() << "}\n";
}

static void dump_farrow_functions(STATE* s, ostream& os) {
  ostream& oss = os;
  os << "\n";

  synth_functions_states = synth_util::build_synth_function_states(s);
  bool needs_fixed_point = s->loop_required;

  for (auto state_it = synth_functions_states.begin(); state_it != synth_functions_states.end(); state_it++) {
    synth_util::SynthFunctionState* synth_functions_state = *state_it;
    current_synth_functions_state = synth_functions_state;

    string result_var = synth_util::RESULT_VAR_PREFIX + std::to_string(synth_functions_state->source->index);

    if (include_comments) {
      os << indent() << "// " << synth_functions_state->source << " (" << (synth_functions_state->is_phylum_instance ? "phylum" : "aug-graph") << ")\n";
    }
    if (synth_functions_state->is_side_effect_evaluation) {
      os << indent() << "val evaluated_map_" << synth_functions_state->fdecl_name << " = scala.collection.mutable.Map[Int"
         << ", Boolean]()"
         << "\n\n";
    }

    os << indent() << "def eval_" << synth_functions_state->fdecl_name << "(";
    os << "node: T_" << decl_name(synth_functions_state->source_phy_graph->phylum);

    for (auto it = synth_functions_state->regular_dependencies.begin(); it != synth_functions_state->regular_dependencies.end(); it++) {
      INSTANCE* source_instance = *it;
      if (synth_util::should_skip_synth_dependency(source_instance)) {
        continue;
      }

      os << ",\n";
      os << indent(nesting_level + 1);
      os << "v_";

      if (!synth_functions_state->is_phylum_instance) {
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

    if (synth_functions_state->is_side_effect_evaluation) {
      os << "Unit";
    } else {
      synth_util::dump_attribute_type(synth_functions_state->source, os);
    }
    os << " = {\n";
    nesting_level++;

    if (needs_fixed_point) {
      os << indent() << "if (!" << synth_util::LOOP_VAR << ") {\n";
      nesting_level++;
    }

    if (synth_functions_state->is_side_effect_evaluation) {
      os << indent() << "evaluated_map_" << synth_functions_state->fdecl_name << ".getOrElse(node.nodeNumber, false) match {\n";
      os << indent(nesting_level + 1) << "case true => ";
      os << "return ()\n";
    } else {
      os << indent() << synth_util::instance_to_attr(synth_functions_state->source) << ".checkNode(node).status match {\n";
      os << indent(nesting_level + 1) << "case Evaluation.ASSIGNED => ";
      if (include_comments) {
        os << "{\n";
        nesting_level++;
        os << indent(nesting_level + 1) << "Debug.out(\"cache hit for \" + node + \" with value \" + " << synth_util::instance_to_attr(synth_functions_state->source) << ".get(node));\n";
        os << indent(nesting_level + 1);
      }

      os << "return " << synth_util::instance_to_attr(synth_functions_state->source) << ".get(node)\n";

      if (include_comments) {
        nesting_level--;
        os << indent(nesting_level + 1) << "}\n";
      }
    }

    os << indent(nesting_level + 1) << "case _ => ()\n";
    os << indent() << "};\n";

    if (needs_fixed_point) {
      nesting_level--;
      os << indent() << "}";
      os << "\n";
    }

    if (synth_functions_state->is_side_effect_evaluation) {
      os << indent() << "node match {\n";
    } else {
      os << indent() << "val " << result_var << " = node match {\n";
    }
    nesting_level++;

    bool source_circular = needs_fixed_point && synth_util::synth_function_is_circular(synth_functions_state);

    for (auto it = synth_functions_state->aug_graphs.begin(); it != synth_functions_state->aug_graphs.end(); it++) {
      AUG_GRAPH* aug_graph = *it;
      int n = aug_graph->instances.length;

      current_aug_graph = aug_graph;
      current_blocks.push_back(matcher_body(top_level_match_m(aug_graph->match_rule)));

      os << indent() << "case " << matcher_pat(top_level_match_m(aug_graph->match_rule)) << " => {\n";
      nesting_level++;

      INSTANCE* aug_graph_instance = NULL;
      if (synth_functions_state->is_phylum_instance) {
        if (!synth_util::find_instance(aug_graph, aug_graph->lhs_decl, synth_functions_state->source->fibered_attr, &aug_graph_instance)) {
          fatal_error("something is wrong with instances in aug graph %s", aug_graph_name(aug_graph));
        }
      } else {
        aug_graph_instance = synth_functions_state->source;
      }

      current_scope_block = synth_util::linearize_block(aug_graph, aug_graph_instance);

      if (include_comments) {
        os << indent() << "/* Linearized schedule:\n";
        nesting_level++;
        synth_util::print_linearized_block(current_scope_block, os);
        nesting_level--;
        os << indent() << "*/\n";
      }

      bool declared_is_circular = instance_circular(aug_graph_instance);
      int instance_index = aug_graph_instance->index;
      bool depends_on_itself = edgeset_kind(aug_graph->graph[instance_index * n + instance_index]) != 0;

      if (!declared_is_circular && depends_on_itself) {
        aps_warning(aug_graph_instance->node, "Instance %s depends on itself but is not declared circular", synth_util::instance_to_string(aug_graph_instance).c_str());
      }

      if (!synth_functions_state->is_side_effect_evaluation) {
        std::vector<std::vector<INSTANCE*>> child_cycle_components = synth_util::collect_child_cycle_components(aug_graph, aug_graph_instance);
        for (size_t component_index = 0; component_index < child_cycle_components.size(); ++component_index) {
          std::string changed_flag = "localChanged" + std::to_string(component_index);
          synth_util::emit_fixed_point_loop_start(os, changed_flag);
          for (auto instance_it = child_cycle_components[component_index].begin(); instance_it != child_cycle_components[component_index].end(); ++instance_it) {
            dumped_conditional_block_items.clear();
            dumped_instances.clear();
            os << indent();
            farrow_impl_ptr->dump_synth_instance(*instance_it, os);
            os << ";\n";
          }
          synth_util::emit_fixed_point_loop_end(os);
        }
        dumped_conditional_block_items.clear();
        dumped_instances.clear();
      }

      if (synth_functions_state->is_side_effect_evaluation) {
        if (include_comments) {
          os << "\n";
        }
        os << indent();
        farrow_impl_ptr->dump_synth_instance(aug_graph_instance, os);
        os << "\n";
        dumped_conditional_block_items.clear();
        dumped_instances.clear();
      }

      if (!synth_functions_state->is_side_effect_evaluation) {
        os << indent();
        farrow_impl_ptr->dump_synth_instance(aug_graph_instance, os);
        os << "\n";
      }

      current_blocks.clear();
      dumped_conditional_block_items.clear();
      dumped_instances.clear();

      nesting_level--;
      os << indent() << "}\n";
    }

    os << indent()
       << "case _ => throw new RuntimeException(\"failed pattern matching: \" "
          "+ node)\n";

    nesting_level--;
    os << indent() << "};\n";

    if (synth_functions_state->is_side_effect_evaluation) {
      os << indent() << "evaluated_map_" << synth_functions_state->fdecl_name << ".update(node.nodeNumber, true);\n";
    } else {
      if (source_circular) {
        os << indent() << synth_util::instance_to_attr(synth_functions_state->source) << ".assign(node, " << result_var << ", changed);\n";
      } else {
        os << indent() << synth_util::instance_to_attr(synth_functions_state->source) << ".assign(node, " << result_var << ");\n";
      }
      os << indent() << synth_util::instance_to_attr(synth_functions_state->source) << ".get(node);\n";
    }

    if (!synth_functions_state->is_side_effect_evaluation) {
      os << indent() << result_var << "\n";
    }

    nesting_level--;
    os << indent() << "}\n\n";
  }

  synth_util::destroy_synth_function_states(synth_functions_states);
  synth_functions_states.clear();
}

class FarrowImpl : public SynthImplementation {
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
      ostream& oss = os;

      dump_farrow_functions(s, oss);

      bool needs_fixed_point = s->loop_required;

      os << indent() << "override def finish() : Unit = {\n";
      ++nesting_level;

      emit_start_phylum_evaluations(os, s);

      os << indent() << "super.finish();\n";
      --nesting_level;
      os << indent() << "};\n";

      clear_implementation_marks(module_decl);
    }
  };

  Super* get_module_info(Declaration m) { return new ModuleInfo(m); }

  void validate_program(Program program) {
    bool uses_fibers = false;
    traverse_Program(detect_program_fibers, &uses_fibers, program);
    if (uses_fibers) {
      fatal_error("-F0 does not support fibers");
    }
  }

  void implement_function_body(Declaration f, ostream& os) { dynamic_impl->implement_function_body(f, os); }

  void implement_value_use(Declaration vd, ostream& os) { synth_util::implement_value_use(vd, current_aug_graph, synth_functions_states, this, os); }

  void dump_assignment(INSTANCE* in, Expression rhs, ostream& o) {
    Declaration ad = in != NULL ? in->fibered_attr.attr : NULL;
    Symbol asym = ad ? def_name(declaration_def(ad)) : 0;
    bool node_is_syntax = in->node == current_aug_graph->lhs_decl;

    if (in->fibered_attr.fiber != NULL) {
      fatal_error("internal error: -F0 attempted to emit a fiber assignment");
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
          o << "(anchor," << rhs << ");\n";
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
            o << "v" << i << "_" << asym << " = " << rhs << "; // local\n";
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
            o << "v_" << asym << " = somehow_combine(v_" << asym << "," << rhs << ");\n";
          } else {
            int i = LOCAL_UNIQUE_PREFIX(ad);
            if (i == 0) {
              o << "v_" << asym << " = " << rhs << "; // function\n";
            } else {
              o << "v" << i << "_" << asym << " = " << rhs << ";\n";
            }
          }
        } else {
          o << "a_" << asym << DEREF;
          if (debug) {
            o << "assign";
          } else {
            o << "set";
          }
          o << "(v_" << decl_name(in->node) << "," << rhs << ");\n";
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
        o << "(v_" << decl_name(in->node) << "," << rhs << ");\n";
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
          if (valid_rhs.size() == 1) {
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
      print_instance(instance, stdout);
      printf(
          " is a non-fiber instance, but no assignment found in this block. "
          "%d\n",
          if_rule_p(instance->fibered_attr.attr));
      fatal_error("crashed since non-fiber instance is missing an assignment");
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

          vector<INSTANCE*> dumped_instanced_positive(dumped_instances);
          dump_rhs_instance_helper(aug_graph, cond->next_positive, instance, o);
          dumped_instances = dumped_instanced_positive;

          if (!visited_if_stmt) {
            current_blocks.pop_back();
            o << "\n";
            nesting_level--;
            o << indent() << "} else {\n";
            nesting_level++;
          }
          current_blocks.push_back(if_stmt_if_false(if_stmt));
          if (!visited_if_stmt) {
            o << indent();
          }

          vector<INSTANCE*> dumped_instanced_negative(dumped_instances);
          dump_rhs_instance_helper(aug_graph, cond->next_negative, instance, o);
          dumped_instances = dumped_instanced_negative;

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
          if (m == first_Match(case_stmt_matchers(header))) {
            Expression e = case_stmt_expr(header);
            o << "{\n";
            nesting_level++;
            o << indent() << "val node" << instance->index << " = " << e << ";\n";
          }
          o << indent() << "node" << instance->index << " match {\n";
          nesting_level++;
          o << indent() << "case " << p << " => {\n";
          nesting_level += 1;
          Block if_true;
          Block if_false;
          if_true = matcher_body(m);
          if (MATCH_NEXT(m)) {
            if_false = 0;
          } else {
            if_false = case_stmt_default(header);
          }

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

  void dump_synth_instance(INSTANCE* instance, ostream& o) {
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
    bool is_circular = edgeset_kind(current_aug_graph->graph[instance->index * current_aug_graph->instances.length + instance->index]);
    bool is_match_formal = synth_util::is_match_formal(instance->fibered_attr.attr);
    bool is_available = is_match_formal || is_inherited;

    if (is_circular && already_dumped && !is_available) {
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
      o << "v_" << synth_util::instance_to_string(instance, current_synth_functions_state->is_phylum_instance);
    } else if (is_inherited) {
      if (is_parent_instance) {
        o << "v_" << synth_util::instance_to_string(instance, current_synth_functions_state->is_phylum_instance);
      } else {
        dump_rhs_instance_helper(aug_graph, block, instance, o);
      }
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
            for (auto it = dependencies.begin(); it != dependencies.end(); it++) {
              INSTANCE* source_instance = *it;

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

Implementation* farrow_impl = farrow_impl_ptr = new FarrowImpl();

#endif /* APS2SCALA */
