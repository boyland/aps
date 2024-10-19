#include <string.h>
#include <algorithm>
#include <iostream>
extern "C" {
#include <stdio.h>
#include "aps-ag.h"
}
#include <functional>
#include <queue>
#include <set>

#include <sstream>
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

// Given an augmented dependency graph, it linearizes it recursively
static BlockItem* linearize_block_helper(AUG_GRAPH* aug_graph, vector<INSTANCE*> sorted_instances, bool* scheduled, CONDITION* cond, BlockItem* prev, int remaining) {
  // impossible merge condition
  if (CONDITION_IS_IMPOSSIBLE(*cond)) {
    return NULL;
  }

  int i, j;
  int n = aug_graph->instances.length;

  for (auto it = sorted_instances.begin(); it != sorted_instances.end(); it++) {
    INSTANCE* instance = *it;
    int i = instance->index;

    if (scheduled[i]) continue;

    // impossible merge condition, cannot schedule this instance
    if (MERGED_CONDITION_IS_IMPOSSIBLE(*cond, instance_condition(instance))) {
      scheduled[i] = true;
      BlockItem* result = linearize_block_helper(aug_graph, sorted_instances, scheduled, cond, prev, remaining - 1);
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
      struct block_item_condition* item = (struct block_item_condition*)malloc(sizeof(struct block_item_condition));
      item_base = (BlockItem*)item;

      item->key = KEY_BLOCK_ITEM_CONDITION;
      item->instance = instance;
      item->condition = instance->fibered_attr.attr;
      item->prev = prev;
      
      int cmask = 1 << (if_rule_index(instance->fibered_attr.attr));
      cond->positive |= cmask;
      item->next_positive = linearize_block_helper(aug_graph, sorted_instances, scheduled, cond, item_base, remaining - 1);
      cond->positive &= ~cmask;
      cond->negative |= cmask;
      item->next_negative = linearize_block_helper(aug_graph, sorted_instances, scheduled, cond, item_base, remaining - 1);
      cond->negative &= ~cmask;
    } else {
      struct block_item_instance* item = (struct block_item_instance*)malloc(sizeof(struct block_item_instance));
      item_base = (BlockItem*)item;
      item->key = KEY_BLOCK_ITEM_INSTANCE;
      item->instance = instance;
      item->next = linearize_block_helper(aug_graph, sorted_instances, scheduled, cond, item_base, remaining - 1);
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

static bool is_instance_applicable_for_synth_function(INSTANCE* instance) {
  bool is_synthesized = instance_direction(instance) == instance_outward;
  bool is_local = instance_direction(instance) == instance_local;
  bool is_shared_info = ATTR_DECL_IS_SHARED_INFO(instance->fibered_attr.attr);
  bool is_fiber = instance->fibered_attr.fiber != NULL;

  return (is_synthesized || is_local) && !is_shared_info && !is_fiber;
}

static bool is_instance_applicable_for_synth_function_formal(INSTANCE* instance) {
  bool is_synthesized = instance_direction(instance) == instance_outward;
  bool is_local = instance_direction(instance) == instance_local;
  bool is_shared_info = ATTR_DECL_IS_SHARED_INFO(instance->fibered_attr.attr);
  bool is_fiber = instance->fibered_attr.fiber != NULL;

  return !is_shared_info && !is_fiber;
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

  // foreach phylum
  for (i = 0; i < s->phyla.length; i++) {
    PHY_GRAPH* pg = &s->phy_graphs[i];
    if (Declaration_KEY(pg->phylum) != KEYphylum_decl) continue;

    // foreach attribute of phylum
    for (j = 0; j < pg->instances.length; j++) {
      INSTANCE* in = &pg->instances.array[j];

      // if attribute is synthesized
      if (!is_instance_applicable_for_synth_function(in)) continue;

      // dump synth function header
      os << indent() << "def eval_" << decl_name(pg->phylum) << "_" << in << "(";

      // dump node itself
      os << "node: T_" << decl_name(pg->phylum);

      // dump its inherited attributes that depend on this particular synthesized attribute
      for (k = 0; k < pg->instances.length; k++) {
        INSTANCE* source_instance = &pg->instances.array[k];
        // if there is a dependency AND dependent attribute is inherited
        DEPENDENCY dep = pg->mingraph[source_instance->index * pg->instances.length + in->index];
        bool is_shared_info = ATTR_DECL_IS_SHARED_INFO(source_instance->fibered_attr.attr);
        Declaration fibered_attr = source_instance->fibered_attr.attr;
        bool is_fiber_attr = source_instance->fibered_attr.fiber != NULL;

        if (dep && is_instance_applicable_for_synth_function_formal(source_instance)) {
          os << ", v_" << source_instance << ": ";
          os << function_type_return_type(attribute_decl_type(fibered_attr));
        }
      }

      os << "): ";
      os << function_type_return_type(attribute_decl_type(in->fibered_attr.attr));
      os << " = {\n";
      nesting_level++;

      os << indent() << "a_" << in << ".checkNode(node).status match {\n";
      os << indent(nesting_level + 1) << "case Evaluation.EVALUATED => return a_" << in << ".get(node)\n";
      os << indent(nesting_level + 1) << "case _ => ()\n";
      os << indent() << "};\n";
      
      os << indent() << "val result = node match {\n";
      nesting_level++;

      // foreach augmented dependency graph
      for (k = 0; k < aug_graph_count; k++) {
        AUG_GRAPH* aug_graph = &s->aug_graphs[k];
        PHY_GRAPH* aug_graph_pg = Declaration_info(aug_graph->lhs_decl)->node_phy_graph;

        // if LHS is the current phylum
        if (aug_graph_pg != pg) continue;

        Declaration tplm = top_level_match_constructor_decl(aug_graph->match_rule);
        Declaration cd = aug_graph->syntax_decl;
        os << indent() << "case " << matcher_pat(top_level_match_m(aug_graph->match_rule)) << " => {\n";
        nesting_level++;

        os << indent();

        current_aug_graph = aug_graph;
        current_blocks.push_back(matcher_body(top_level_match_m(aug_graph->match_rule)));

        current_scope_block = linearize_block(aug_graph);
        // print_linearized_block(current_scope_block);
        // printf("\n");

        INSTANCE* instance_out;
        if (impl->find_instance(aug_graph, aug_graph->lhs_decl, in->fibered_attr.attr, &instance_out)) {
          impl->dump_rhs_of_instance(aug_graph, instance_out, os);
        } else {
          fatal_error("something is wrong with instances in aug graph %s", aug_graph_name(aug_graph));
        }

        current_blocks.clear();

        os << "\n";

        nesting_level--;
        os << indent() << "}\n";
      }

      os << indent() << "case _ => throw new RuntimeException(\"failed pattern matching: \" + node)\n";

      nesting_level--;
      os << indent() << "};\n";


      os << indent() << "a_" << in << ".assign(node, result)\n";
      os << indent() << "result\n";
      nesting_level--;
      os << indent() << "}\n\n";
    }
  }

  for (i = 0; i < aug_graph_count; i++) {
    AUG_GRAPH* aug_graph = &s->aug_graphs[i];

    for (j = 0; j < aug_graph->instances.length; j++) {
      INSTANCE* instance = &aug_graph->instances.array[j];
      bool is_local = instance_direction(instance) == instance_local;
      bool is_fiber = instance->fibered_attr.fiber != NULL;

      if (is_local && !is_fiber && !if_rule_p(instance->fibered_attr.attr)) {
        switch (Declaration_KEY(instance->fibered_attr.attr))
        {
        case KEYformal:
          continue;
          break;
        default:
          break;
        }

        Declaration attr = instance->fibered_attr.attr;
        PHY_GRAPH* lhs_phygraph = Declaration_info(aug_graph->lhs_decl)->node_phy_graph;
        Declaration tdecl = canonical_type_decl(canonical_type(value_decl_type(attr)));

        current_aug_graph = aug_graph;
        current_blocks.push_back(matcher_body(top_level_match_m(aug_graph->match_rule)));

        current_scope_block = linearize_block(aug_graph);

        os << indent() << "def eval_a" << LOCAL_UNIQUE_PREFIX(attr) << "_" << instance << "(node: T_" << decl_name(lhs_phygraph->phylum) << "): " << decl_name(tdecl) << " = {\n";
        nesting_level++;

        os << indent() << "a" << LOCAL_UNIQUE_PREFIX(attr) << "_" << instance << ".checkNode(node).status match {\n";
        os << indent(nesting_level + 1) << "case Evaluation.EVALUATED => return a" << LOCAL_UNIQUE_PREFIX(attr) << "_" << instance << ".get(node)\n";
        os << indent(nesting_level + 1) << "case _ => ()\n";
        os << indent() << "};\n";
        
        os << indent() << "val result = ";
        impl->dump_rhs_of_instance(aug_graph, instance, os);
        os << ";\n";
  
        os << indent() << "a" << LOCAL_UNIQUE_PREFIX(attr) << "_" << instance << ".assign(node, result)\n";
        os << indent() << "result\n";

        nesting_level--;
        os << indent() << "}\n\n";
      }
    }
  }
}

class SynthScc : public Implementation {
 public:
  typedef Implementation::ModuleInfo Super;
  class ModuleInfo : public Super {
   public:
    ModuleInfo(Declaration mdecl) : Implementation::ModuleInfo(mdecl) {}

    void note_top_level_match(Declaration tlm, GEN_OUTPUT& oss) {
      Super::note_top_level_match(tlm, oss);
    }

    void note_local_attribute(Declaration ld, GEN_OUTPUT& oss) {
      Super::note_local_attribute(ld, oss);
      Declaration_info(ld)->decl_flags |= LOCAL_ATTRIBUTE_FLAG;
    }

    void note_attribute_decl(Declaration ad, GEN_OUTPUT& oss) {
      Declaration_info(ad)->decl_flags |= ATTRIBUTE_DECL_FLAG;
      Super::note_attribute_decl(ad, oss);
    }

    void note_var_value_decl(Declaration vd, GEN_OUTPUT& oss) {
      Super::note_var_value_decl(vd, oss);
    }

#ifdef APS2SCALA
    void implement(ostream& os){
#else  /* APS2SCALA */
    void implement(output_streams& oss) {
#endif /* APS2SCALA */
        STATE* s = (STATE*)Declaration_info(module_decl)->analysis_state;

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

      if (!is_instance_applicable_for_synth_function(in)) continue;

      os << indent() << "eval_" << phy_graph_name(start_phy_graph) << "_" << &start_phy_graph->instances.array[i] << "(root);\n";
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
}

void implement_value_use(Declaration vd, ostream& os) {
  int flags = Declaration_info(vd)->decl_flags;
  if (flags & LOCAL_ATTRIBUTE_FLAG) {
    os << "eval_" << "a" << LOCAL_UNIQUE_PREFIX(vd) << "_" << decl_name(vd) << "(node)";
  } else if (flags & ATTRIBUTE_DECL_FLAG) {
    if (ATTR_DECL_IS_INH(vd)) {
      os << "v_" << decl_name(vd);
    } else {
      os << "eval_" << decl_name(vd);
    }

    // os << "a" << "_" << decl_name(vd) << DEREF << "get";
  } else if (flags & LOCAL_VALUE_FLAG) {
    os << "v" << LOCAL_UNIQUE_PREFIX(vd) << "_" << decl_name(vd);
  } else {
    aps_error(vd,"internal_error: What is special about this?");
  }
}

  void dump_rhs_of_instance_helper(AUG_GRAPH* aug_graph, BlockItem* block, INSTANCE* instance, ostream& o) {
    if (block->key == KEY_BLOCK_ITEM_INSTANCE) {
      for (auto it = current_blocks.rbegin(); it != current_blocks.rend(); it++) {
        Declarations ds = block_body(*it);
        for (Declaration d = first_Declaration(ds); d; d = DECL_NEXT(d)) {
          switch (Declaration_KEY(d)) {
            default:
              break;
            case KEYvalue_decl: {
              if (instance_direction(instance) == instance_local && instance->node == NULL && instance->fibered_attr.attr == d) {
                dump_vd_Default(d, o);
                return;
              }

              break;
            }
            case KEYassign: {
              Expression rhs = assign_rhs(d);
              Expression lhs = assign_lhs(d);
              INSTANCE* rhs_instance = Expression_info(rhs)->value_for;
              if (attr_ref_p(lhs) != NULL) {
                Declaration lhs_node = USE_DECL(value_use_use(first_Actual(funcall_actuals(lhs))));
                Declaration lhs_attr = attr_ref_p(lhs);

                if (lhs_node == instance->node && lhs_attr == instance->fibered_attr.attr) {
                  dump_Expression(assign_rhs(d), o);
                  return;
                }
              }
              break;
            }
          }
        }

        printf("(block lineno %d)=>", tnode_line_number(*it));
        print_instance(instance, stdout);
        printf("<=\n");
        aps_warning(instance, "could not find assignment for instance");
      }
    } else if (block->key == KEY_BLOCK_ITEM_CONDITION) {
      struct block_item_condition* cond = (struct block_item_condition*)block;
      Declaration if_stmt = cond->condition;

      o << "if (";
      dump_Expression(if_stmt_cond(if_stmt), o);
      o << ") {\n";
      nesting_level++;
      current_blocks.push_back(if_stmt_if_true(if_stmt));
      o << indent();
      dump_rhs_of_instance_helper(aug_graph, cond->next_positive, instance, o);
      current_blocks.pop_back();
      o << "\n";
      nesting_level--;
      o << indent() << "} else {\n";
      nesting_level++;
      current_blocks.push_back(if_stmt_if_false(if_stmt));
      o << indent();
      dump_rhs_of_instance_helper(aug_graph, cond->next_negative, instance, o);
      current_blocks.pop_back();
      nesting_level--;
      o << "\n";
      o << indent() << "}";
    }
  }

  // given LHS, it tries to find assignment that assigns to the LHS and then dumps the RHS
  virtual void dump_rhs_of_instance(AUG_GRAPH* aug_graph, INSTANCE* instance, ostream& o) override {
    BlockItem* block = find_surrounding_block(current_scope_block, instance);

    dump_rhs_of_instance_helper(aug_graph, block, instance, o);
  }
}
;

Implementation* synth_impl = new SynthScc();
