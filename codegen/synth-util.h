#ifndef SYNTH_UTIL_H
#define SYNTH_UTIL_H

#include <ostream>
#include <set>
#include <string>
#include <vector>

extern "C" {
#include <stdio.h>

#include "aps-ag.h"
}

class SynthImplementation;

namespace synth_util {
extern const std::string LOOP_VAR;
extern const std::string RESULT_VAR_PREFIX;
constexpr int LOCAL_VALUE_FLAG = 1 << 28;

void emit_loop_implicits(std::ostream& output, const std::string& flag);

void emit_fixed_point_loop_start(std::ostream& output, const std::string& flag);

void emit_fixed_point_loop_end(std::ostream& output);

struct SynthFunctionState {
  std::string fdecl_name;
  INSTANCE* source;
  PHY_GRAPH* source_phy_graph;
  std::vector<INSTANCE*> regular_dependencies;
  std::vector<AUG_GRAPH*> aug_graphs;
  bool is_phylum_instance;
  bool is_side_effect_evaluation;
};

enum BlockItemKind { KEY_BLOCK_ITEM_CONDITION, KEY_BLOCK_ITEM_INSTANCE };

struct BlockItem {
  BlockItemKind key;
  INSTANCE* instance;
  BlockItem* prev;
};

struct BlockItemCondition {
  BlockItemKind key;
  INSTANCE* instance;
  BlockItem* prev;
  Declaration condition;
  BlockItem* next_positive;
  BlockItem* next_negative;
};

struct BlockItemInstance {
  BlockItemKind key;
  INSTANCE* instance;
  BlockItem* prev;
  BlockItem* next;
};

bool state_uses_fibers(STATE* state);

bool instance_is_synthesized(INSTANCE* instance);

bool instance_is_inherited(INSTANCE* instance);

bool instance_is_pure_shared_info(INSTANCE* instance);

bool instance_is_parent(INSTANCE* instance, AUG_GRAPH* graph);

std::string instance_to_string(INSTANCE* instance, bool trim_node = false);

std::string instance_to_string_with_nodetype(Declaration node_type, INSTANCE* instance);

std::string instance_to_attr(INSTANCE* instance);

bool is_match_formal(void* node);

bool should_skip_synth_dependency(INSTANCE* instance);

bool find_instance(AUG_GRAPH* graph, Declaration node, const FIBERED_ATTRIBUTE& attribute, INSTANCE** result);

std::vector<SynthFunctionState*> build_synth_function_states(STATE* state);

void destroy_synth_function_states(const std::vector<SynthFunctionState*>& states);

void implement_value_use(Declaration declaration, AUG_GRAPH* graph, const std::vector<SynthFunctionState*>& states, SynthImplementation* implementation, std::ostream& output);

bool try_dump_funcall(Expression expression, AUG_GRAPH* graph, SynthImplementation* implementation, std::ostream& output);

void dump_attribute_type(INSTANCE* instance, std::ostream& output);

bool synth_function_is_circular(SynthFunctionState* state);

std::vector<std::vector<INSTANCE*>> collect_child_cycle_components(AUG_GRAPH* graph, INSTANCE* sink);

std::vector<std::set<Expression>> make_instance_assignments(AUG_GRAPH* graph, const std::vector<Block>& blocks);

BlockItem* linearize_block(AUG_GRAPH* graph, INSTANCE* sink);

void print_linearized_block(BlockItem* block, std::ostream& output);

BlockItem* find_surrounding_block(BlockItem* block, INSTANCE* instance);

}  // namespace synth_util

#endif