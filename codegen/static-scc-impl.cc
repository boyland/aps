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

template<typename T>
static std::string any_to_string(const T & value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

typedef void (*OutputWriterT)(int, std::ostream&);

class OutputWriter {
  struct Item {
    OutputWriterT function;
    int marker;
  };

 private:
  std::ostream& _os;
  std::vector<Item> _items;

  void dump_queue() {
    for (std::vector<Item>::iterator it = _items.begin(); it != _items.end();
         it++) {
      ((OutputWriterT)it->function)(it->marker, _os);
    }

    _items.clear();
  }

  bool contains_marker(int marker) {
    for (std::vector<Item>::iterator it = _items.begin(); it != _items.end();
         it++) {
      if (it->marker == marker) {
        return true;
      }
    }

    return false;
  }

 public:
  OutputWriter(std::ostream& os) : _os(os), _items(std::vector<Item>()) {}

  std::ostream& get_outstream() {
    dump_queue();
    return _os;
  }

  void queue_write(int marker,
                   OutputWriterT lambda) {
    if (!contains_marker(marker)) {
      struct Item* item = new Item;
      item->marker = marker;
      item->function = lambda;
      _items.push_back(*item);
    } else {
      fatal_error("Already enqueued to write marker: %d", VOIDP2INT(marker));
    }
  }

  bool any_write_since(int marker) { return !contains_marker(marker); }

  void clear_since(int marker) {
    bool found_marker = false;
    for (std::vector<Item>::iterator it = _items.begin(); it != _items.end();) {
      if (it->marker == marker || found_marker) {
        it = _items.erase(it);
        found_marker = true;
      } else {
        it++;
      }
    }
  }

  virtual ~OutputWriter() { dump_queue(); }
};

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
static vector<std::set<Expression> > make_instance_assignment(
    AUG_GRAPH* aug_graph,
    Block block,
    vector<std::set<Expression> > from) {
  int n = aug_graph->instances.length;
  vector<std::set<Expression> > array(from);

  for (int i = 0; i < n; ++i) {
    INSTANCE* in = &aug_graph->instances.array[i];
    Declaration ad = in->fibered_attr.attr;
    if (ad != 0 && in->fibered_attr.fiber == 0 &&
        ABSTRACT_APS_tnode_phylum(ad) == KEYDeclaration) {
      // get default!
      switch (Declaration_KEY(ad)) {
        case KEYattribute_decl:
          array[i].insert(default_init(attribute_decl_default(ad)));
          break;
        case KEYvalue_decl:
          array[i].insert(default_init(value_decl_default(ad)));
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

  return array;
}

// visit procedures are called:
// visit_n_m
// where n is the number of the phy_graph and m is the phase.
// This does a dispatch to visit_n_m_p
// where p is the production number (0-based constructor index)
#define PHY_GRAPH_NUM(pg) (pg - pg->global_state->phy_graphs)

static void dump_loop_end(AUG_GRAPH* aug_graph,
                          int parent_ph,
                          int loop_id,
                          OutputWriter* ow) {
#ifdef APS2SCALA
  if (ow->any_write_since(loop_id)) {
    std::ostream& os = ow->get_outstream();
    string suffix = any_to_string(loop_id);
    std::replace(suffix.begin(), suffix.end(), '-', '_');
    --nesting_level;
    os << "\n";
    os << indent() << "} while(changed);"
       << "\n";
    os << indent() << "changed = prevChanged_" << suffix << ";\n"
       << "\n";
  } else {
    ow->clear_since(loop_id);
  }
#endif /* APS2SCALA */
}

static void dump_loop_start_helper(int loop_id, std::ostream& os)
{
  // ABS value of component_index because initially it is -1
  string suffix = any_to_string(loop_id);
  std::replace(suffix.begin(), suffix.end(), '-', '_');

  os << indent() << "val prevChanged_" << suffix << " = changed;\n";
  os << indent() << "do {\n";
  ++nesting_level;
  os << indent() << "changed = false;\n";
}

static void dump_loop_start(AUG_GRAPH* aug_graph,
                            int parent_ph,
                            int loop_id,
                            OutputWriter* ow) {
#ifdef APS2SCALA

  ow->queue_write(
      loop_id,
      dump_loop_start_helper);

#endif /* APS2SCALA */
}

// phase is what we are generating code for,
// current is the current value of ph
// return true if there are still more instances after this phase:
static bool implement_visit_function(
    AUG_GRAPH* aug_graph,
    int phase, /* phase to impl. */
    CTO_NODE* cto,
    vector<std::set<Expression> > instance_assignment,
    int nch,
    CONDITION* cond,
    int chunk_index,
    bool loop_allowed,
    int loop_id,
    bool skip_previous_visit_code,
    OutputWriter* ow) {
  for (; cto; cto = cto->cto_next) {
    INSTANCE* in = cto->cto_instance;
    int ch = cto->child_phase.ch;
    int ph = cto->child_phase.ph;
    bool chunk_changed = chunk_index != -1 && cto->chunk_index != chunk_index;
    PHY_GRAPH* pg_parent =
        Declaration_info(aug_graph->lhs_decl)->node_phy_graph;

    bool is_conditional = in != NULL && if_rule_p(in->fibered_attr.attr);

    bool is_mod = false;
    switch (Declaration_KEY(aug_graph->syntax_decl)) {
      case KEYsome_class_decl:
        is_mod = true;
        break;
      default:
        break;
    }

    if (!is_mod && chunk_changed) {
      if (include_comments) {
        ow->get_outstream()
            << indent() << "// Finished with chunk #" << chunk_index << "\n";
      }

      // Need to close the loop if any
      if (loop_allowed && loop_id != -1) {
        dump_loop_end(aug_graph, phase, loop_id, ow);
        loop_id = -1;
      }

      if (include_comments) {
        ow->get_outstream() << indent() << "// Started working on chunk #"
                            << cto->chunk_index << "\n";
      }

      if (loop_allowed && cto->chunk_circular) {
        if (!pg_parent->cyclic_flags[phase]) {
          loop_id = cto->chunk_index;
          dump_loop_start(aug_graph, phase, loop_id, ow);
        } else {
          if (include_comments) {
            ow->get_outstream()
                << indent() << "// Parent phase " << phase
                << " is circular, fixed-point loop cannot be added here\n";
          }
        }
      }
    }

    // Code generate if:
    // - CTO_NODE belongs to this visit
    // - OR CTO_NODE is conditional
    if (skip_previous_visit_code && !is_conditional) {
      // CTO_NODE belongs to this visit
      if (cto->visit != phase) {
        chunk_index = cto->chunk_index;
        continue;
      }
    }

    // Update chunk index
    chunk_index = cto->chunk_index;

    // Visit marker for when visit ends
    if (in == NULL && ch == -1) {
      // If we are the module (roots) level
      if (is_mod) {
        int n = PHY_GRAPH_NUM(
            Declaration_info(aug_graph->syntax_decl)->node_phy_graph);

        // Dump loop start if this phase of global dependency is circular and
        // loop is allowed
        if (pg_parent->cyclic_flags[ph] && loop_allowed) {
          loop_id = cto->chunk_index;
          dump_loop_start(aug_graph, phase, loop_id, ow);
        }
#ifdef APS2SCALA
        ow->get_outstream() << indent() << "for (root <- roots) {\n";
#else  /* APS2SCALA */
        ow->get_outstream()
            << indent() << "for (int i=0; i < n_roots; ++i) {\n";
#endif /* APS2SCALA */
        ++nesting_level;
#ifndef APS2SCALA
        ow->get_outstream()
            << indent() << "C_PHYLUM::Node *root = phylum->node(i);\n";
#endif /* APS2SCALA */

        ow->get_outstream() << indent() << "visit_" << n << "_" << ph << "(";
        ow->get_outstream() << "root";
        ow->get_outstream() << ");\n";

        --nesting_level;
        ow->get_outstream() << indent() << "}\n";

        // Dump loop end if this phase of global dependency was circular, and we
        // are inside the loop
        if (pg_parent->cyclic_flags[ph] && loop_allowed && loop_id != -1) {
          dump_loop_end(aug_graph, phase, loop_id, ow);
          loop_id = -1;
        }
      }

      if (include_comments) {
        ow->get_outstream()
            << indent() << "// End of parent (" << aug_graph_name(aug_graph)
            << ") phase visit marker for phase: " << ph << "\n";
      }
      if (!is_mod) {
        // Need to close the loop if any
        if (loop_allowed && loop_id != -1) {
          dump_loop_end(aug_graph, phase, loop_id, ow);
          loop_id = -1;
        }
      }

      // If ph == phase to implement then stop, cannot continue any further for
      // this visit
      if (ph == phase) {
        return false;
      }

      // Otherwise, continue, there is more instances to implement
      continue;
    }

    // Visit marker for when child visit happens
    if (in == NULL && ch > -1) {
      ow->get_outstream() << "\n";

      if (include_comments) {
        ow->get_outstream()
            << indent() << "// aug_graph: " << aug_graph_name(aug_graph)
            << "\n";
        ow->get_outstream()
            << indent() << "// visit marker(" << ph << "," << ch << ")\n";
      }

      PHY_GRAPH* pg = Declaration_info(cto->child_decl)->node_phy_graph;
      int n = PHY_GRAPH_NUM(pg);

      if (include_comments) {
        ow->get_outstream()
            << indent() << "// parent visit of " << decl_name(pg_parent->phylum)
            << " at phase " << phase << " is "
            << (pg_parent->cyclic_flags[phase] ? "circular" : "non-circular")
            << "\n";
        ow->get_outstream()
            << indent() << "// child visit of " << decl_name(pg->phylum)
            << " at phase " << ph << " is "
            << (pg->cyclic_flags[ph] ? "circular" : "non-circular") << "\n";
      }

      if (!pg_parent->cyclic_flags[phase] && pg->cyclic_flags[ph] &&
          loop_allowed && chunk_index == -1) {
        fatal_error(
            "The child visit(%d,%d) of %s should have been wrapped in a "
            "do-while loop.",
            n, ph, aug_graph_name(aug_graph));
      }

      ow->get_outstream() << indent() << "visit_" << n << "_" << ph << "(";
#ifdef APS2SCALA
      ow->get_outstream() << "v_" << decl_name(cto->child_decl);
#else  /* APS2SCALA */
      ow->get_outstream() << "v_" << decl_name(cto->child_decl);
#endif /* APS2SCALA */
      ow->get_outstream() << ");\n";

      continue;
    }

    // Instance should not be null for non-visit marker CTO nodes
    // Visit markers have a form of either: <ph,ch> or <-ph,-1>
    if (in == NULL) {
      fatal_error(
          "total_order is malformed: Instance should not be null for non-visit "
          "marker CTO nodes.");
    }

    Declaration ad = in->fibered_attr.attr;
    void* ad_parent = ad != NULL ? tnode_parent(ad) : NULL;

    bool node_is_lhs = in->node == aug_graph->lhs_decl;
    bool node_is_syntax = ch < nch || node_is_lhs;
    bool node_is_for_in_stmt = ad_parent != NULL &&
                               ABSTRACT_APS_tnode_phylum(ad_parent) == KEYDeclaration &&
                               Declaration_KEY((Declaration)ad_parent) == KEYfor_in_stmt;

    CONDITION icond = instance_condition(in);
    if (MERGED_CONDITION_IS_IMPOSSIBLE(*cond, icond)) {
      if (include_comments) {
        ow->get_outstream()
            << indent() << "// '" << in
            << "' attribute instance is impossible because cond: (+"
            << cond->positive << ",-" << cond->negative << ") icond: (+"
            << icond.positive << ",-" << icond.negative << ")\n";
      }
      continue;
    }

    if (node_is_for_in_stmt) {
      Declaration for_in_stmt_decl = (Declaration)ad_parent;
      Block body = for_in_stmt_body(for_in_stmt_decl);
      Declaration formal = for_in_stmt_formal(for_in_stmt_decl);
      Expression sequence = for_in_stmt_seq(for_in_stmt_decl);

      bool prev_loop_allowed = loop_allowed;
      if (loop_allowed) {
        // If loop is allowed, and we are not in the loop already then allow
        // loops inside the for-in-stmt.
        loop_allowed = loop_id == -1;
      }

#ifdef APS2SCALA
        ow->get_outstream() << indent() << "for (v_" << decl_name(formal) << " <- " << sequence << ") {\n";
        ++nesting_level;
#endif /* APS2SCALA */

      vector<std::set<Expression> > assignment =
          make_instance_assignment(aug_graph, body, instance_assignment);

      bool cont = implement_visit_function(aug_graph, phase, cto->cto_next,
                               assignment, nch, cond, cto->chunk_index,
                               loop_allowed, loop_id, skip_previous_visit_code,
                               ow);

#ifdef APS2SCALA
      --nesting_level;
      ow->get_outstream() << indent() << "}\n";


      // Restore previous value of loop allowed.
      loop_allowed = prev_loop_allowed;

      // Closing of the loop now that for-in-stmt is finished
      if (loop_allowed && loop_id != -1) {
        dump_loop_end(aug_graph, phase, loop_id, ow);
        loop_id = -1;
      }
#endif /* APS2SCALA */
      return cont;
    }

    if (if_rule_p(ad)) {
      bool prev_loop_allowed = loop_allowed;
      if (loop_allowed) {
        // If loop is allowed, and we are not in the loop already then allow
        // loops inside the true and false branch of the conditional.
        loop_allowed = loop_id == -1;
      };

      bool is_match = ABSTRACT_APS_tnode_phylum(ad) == KEYMatch;
      Block if_true;
      Block if_false;
      if (is_match) {
        Match m = (Match)ad;
        Pattern p = matcher_pat(m);
        Declaration header = Match_info(m)->header;
        // if first match in case, we evaluate variable:
        if (m == first_Match(case_stmt_matchers(header))) {
          Expression e = case_stmt_expr(header);
#ifdef APS2SCALA
          // Type ty = infer_expr_type(e);
          ow->get_outstream() << indent() << "val node = " << e << ";\n";
#else  /* APS2SCALA */
          Type ty = infer_expr_type(e);
          ow->get_outstream() << indent() << ty << " node=" << e << ";\n";
#endif /* APS2SCALA */
        }
#ifdef APS2SCALA
        ow->get_outstream() << indent() << "node match {\n";
        ow->get_outstream() << indent() << "case " << p << " => {\n";
#else  /* APS2SCALA */
        ow->get_outstream() << indent() << "if (";
        dump_Pattern_cond(p, "node", ow->get_outstream());
        ow->get_outstream() << ") {\n";
#endif /* APS2SCALA */
        nesting_level += 1;
#ifndef APS2SCALA
        dump_Pattern_bindings(p, ow->get_outstream());
#endif /* APS2SCALA */
        if_true = matcher_body(m);
        if (MATCH_NEXT(m)) {
          if_false = 0;  //? Why not the nxt match ?
        } else {
          if_false = case_stmt_default(header);
        }
      } else {
        // Symbol boolean_symbol = intern_symbol("Boolean");
        ow->get_outstream()
            << indent() << "if (" << if_stmt_cond(ad) << ") {\n";
        ++nesting_level;
        if_true = if_stmt_if_true(ad);
        if_false = if_stmt_if_false(ad);
      }

      int cmask = 1 << (if_rule_index(ad));
      vector<std::set<Expression> > true_assignment =
          make_instance_assignment(aug_graph, if_true, instance_assignment);

      cond->positive |= cmask;
      implement_visit_function(aug_graph, phase, cto->cto_if_true,
                               true_assignment, nch, cond, cto->chunk_index,
                               loop_allowed, loop_id, skip_previous_visit_code,
                               ow);
      cond->positive &= ~cmask;

      --nesting_level;
#ifdef APS2SCALA
      if (is_match) {
        ow->get_outstream() << indent() << "}\n";
        ow->get_outstream() << indent() << "case _ => {\n";
      } else {
        ow->get_outstream() << indent() << "} else {\n";
      }
#else  /* APS2SCALA */
      ow->get_outstream() << indent() << "} else {\n";
#endif /* APS2SCALA */
      ++nesting_level;
      vector<std::set<Expression> > false_assignment =
          if_false ? make_instance_assignment(aug_graph, if_false,
                                              instance_assignment)
                   : instance_assignment;

      cond->negative |= cmask;
      bool cont = implement_visit_function(
          aug_graph, phase, cto->cto_if_false, false_assignment, nch, cond,
          cto->chunk_index, loop_allowed, loop_id, skip_previous_visit_code,
          ow);
      cond->negative &= ~cmask;

      --nesting_level;
#ifdef APS2SCALA
      if (is_match) {
        ow->get_outstream() << indent() << "}}\n";
      } else {
        ow->get_outstream() << indent() << "}\n";
      }

      // Restore previous value of loop allowed.
      loop_allowed = prev_loop_allowed;

      // Delay closing of the loop until conditional is finished
      if (loop_allowed && loop_id != -1) {
        dump_loop_end(aug_graph, phase, loop_id, ow);
        loop_id = -1;
      }

#else  /* APS2SCALA */
      ow->get_outstream() << indent() << "}\n";
#endif /* APS2SCALA */
      return cont;
    }

    Symbol asym = ad ? def_name(declaration_def(ad)) : 0;

    if (instance_direction(in) == instance_inward) {
      if (include_comments) {
        ow->get_outstream() << indent() << "// " << in << " is ready now.\n";
      }
      continue;
    }

    for (std::set<Expression>::iterator rhs_it =
             instance_assignment[in->index].begin();
         rhs_it != instance_assignment[in->index].end(); rhs_it++) {
      Expression rhs = *rhs_it;

      if (rhs == NULL)
        continue;

      if (in->node && Declaration_KEY(in->node) == KEYnormal_assign) {
        // parameter value will be filled in at call site
        if (include_comments) {
          ow->get_outstream()
              << indent() << "// delaying " << in << " to call site.\n";
        }
        continue;
      }

      if (in->node && Declaration_KEY(in->node) == KEYpragma_call) {
        if (include_comments) {
          ow->get_outstream()
              << indent() << "// place holder for " << in << "\n";
        }
        continue;
      }

      if (in->fibered_attr.fiber != NULL) {
        if (rhs == NULL) {
          if (include_comments) {
            ow->get_outstream() << indent() << "// " << in << "\n";
          }
          continue;
        }

        Declaration assign = (Declaration)tnode_parent(rhs);
        Expression lhs = assign_lhs(assign);
        Declaration field = 0;
        ow->get_outstream() << indent();
        // dump the object containing the field
        switch (Expression_KEY(lhs)) {
          case KEYvalue_use:
            // shared global collection
            field = USE_DECL(value_use_use(lhs));
#ifdef APS2SCALA
            ow->get_outstream() << "a_" << decl_name(field) << ".";
            if (debug)
              ow->get_outstream() << "assign";
            else
              ow->get_outstream() << "set";
            ow->get_outstream() << "(" << rhs << ");\n";
#else  /* APS2SCALA */
            ow->get_outstream() << "v_" << decl_name(field) << "=";
            switch (Default_KEY(value_decl_default(field))) {
              case KEYcomposite:
                ow->get_outstream()
                    << composite_combiner(value_decl_default(field));
                break;
              default:
                ow->get_outstream()
                    << as_val(value_decl_type(field)) << "->v_combine";
                break;
            }
            ow->get_outstream()
                << "(v_" << decl_name(field) << "," << rhs << ");\n";
#endif /* APS2SCALA */
            break;
          case KEYfuncall:
            field = field_ref_p(lhs);
            if (field == 0)
              fatal_error("what sort of assignment lhs: %d",
                          tnode_line_number(assign));
            ow->get_outstream() << "a_" << decl_name(field) << DEREF;
            if (debug)
              ow->get_outstream() << "assign";
            else
              ow->get_outstream() << "set";
            ow->get_outstream()
                << "(" << field_ref_object(lhs) << "," << rhs << ");\n";
            break;
          default:
            fatal_error("what sort of assignment lhs: %d",
                        tnode_line_number(assign));
        }
        continue;
      }

      if (in->node == 0 && ad != NULL) {
        if (rhs) {
          if (Declaration_info(ad)->decl_flags & LOCAL_ATTRIBUTE_FLAG) {
            ow->get_outstream() << indent() << "a" << LOCAL_UNIQUE_PREFIX(ad)
                                << "_" << asym << DEREF;
            if (debug)
              ow->get_outstream() << "assign";
            else
              ow->get_outstream() << "set";
            ow->get_outstream() << "(anchor," << rhs << ");\n";
          } else {
            int i = LOCAL_UNIQUE_PREFIX(ad);
            if (i == 0) {
#ifdef APS2SCALA
              if (!def_is_constant(value_decl_def(ad))) {
                if (include_comments) {
                  ow->get_outstream()
                      << indent() << "// v_" << asym
                      << " is assigned/initialized by default.\n";
                }
              } else {
                if (include_comments) {
                  ow->get_outstream() << indent() << "// v_" << asym
                                      << " is initialized in module.\n";
                }
              }
#else
              ow->get_outstream()
                  << indent() << "v_" << asym << " = " << rhs << ";\n";
#endif
            } else {
              ow->get_outstream() << indent() << "v" << i << "_" << asym
                                  << " = " << rhs << "; // local\n";
            }
          }
        } else {
          if (Declaration_KEY(ad) == KEYvalue_decl &&
              !direction_is_collection(value_decl_direction(ad))) {
            aps_warning(ad, "Local attribute %s is apparently undefined",
                        decl_name(ad));
          }
          if (include_comments) {
            ow->get_outstream() << indent() << "// " << in << " is ready now\n";
          }
        }
        continue;
      } else if (node_is_syntax) {
        if (ATTR_DECL_IS_SHARED_INFO(ad) && ch < nch) {
          if (include_comments) {
            ow->get_outstream() << indent() << "// shared info for "
                                << decl_name(in->node) << " is ready.\n";
          }
        } else if (ATTR_DECL_IS_UP_DOWN(ad)) {
          if (include_comments) {
            ow->get_outstream() << indent() << "// " << decl_name(in->node)
                                << "." << decl_name(ad) << " implicit.\n";
          }
        } else if (rhs) {
          if (Declaration_KEY(in->node) == KEYfunction_decl) {
            if (direction_is_collection(value_decl_direction(ad))) {
              std::cout << "Not expecting collection here!\n";
              ow->get_outstream()
                  << indent() << "v_" << asym << " = somehow_combine(v_" << asym
                  << "," << rhs << ");\n";
            } else {
              int i = LOCAL_UNIQUE_PREFIX(ad);
              if (i == 0)
                ow->get_outstream() << indent() << "v_" << asym << " = " << rhs
                                    << "; // function\n";
              else
                ow->get_outstream() << indent() << "v" << i << "_" << asym
                                    << " = " << rhs << ";\n";
            }
          } else {
            ow->get_outstream() << indent() << "a_" << asym << DEREF;
            if (debug)
              ow->get_outstream() << "assign";
            else
              ow->get_outstream() << "set";
            ow->get_outstream()
                << "(v_" << decl_name(in->node) << "," << rhs << ");\n";
          }
        } else {
          aps_warning(in->node, "Attribute %s.%s is apparently undefined",
                      decl_name(in->node), symbol_name(asym));

          if (include_comments) {
            ow->get_outstream() << indent() << "// " << in << " is ready.\n";
          }
        }
        continue;
      } else if (Declaration_KEY(in->node) == KEYvalue_decl) {
        if (rhs) {
          // assigning field of object
          ow->get_outstream() << indent() << "a_" << asym << DEREF;
          if (debug)
            ow->get_outstream() << "assign";
          else
            ow->get_outstream() << "set";
          ow->get_outstream()
              << "(v_" << decl_name(in->node) << "," << rhs << ");\n";
        } else {
          if (include_comments) {
            ow->get_outstream()
                << indent() << "// " << in << " is ready now.\n";
          }
        }
        continue;
      }
      std::cout << "Problem assigning " << in << "\n";
      ow->get_outstream() << indent() << "// Not sure what to do for " << in
                          << "\n";
    }
  }

  // Close any dangling loop if any. This should never happen
  // because after circular visit is followed by non-empty or empty
  // non-circular visit so this code never happens.
  if (loop_allowed && loop_id != -1) {
    dump_loop_end(aug_graph, phase, loop_id, ow);
    loop_id = -1;
  }

  return false;  // no more!
}

// dump visit functions for constructors
static void dump_visit_functions(PHY_GRAPH* phy_graph,
                          AUG_GRAPH* aug_graph,
#ifdef APS2SCALA
                          ostream& os)
#else  /* APS2SCALA */
                          output_streams& oss)
#endif /* APS2SCALA */
{
  Declaration tlm = aug_graph->match_rule;
  Match m = top_level_match_m(tlm);
  Block block = matcher_body(m);
  STATE* s = phy_graph->global_state;

#ifndef APS2SCALA
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  // ostream& is = oss.is;
  ostream& os = inline_definitions ? hs : cpps;

#endif /* APS2SCALA */
  int pgn = PHY_GRAPH_NUM(phy_graph);
  int j = Declaration_info(aug_graph->syntax_decl)->instance_index;
  CTO_NODE* total_order = aug_graph->total_order;

  /* Count the children */
  int nch = 0;
  for (Declaration ch = aug_graph->first_rhs_decl; ch != 0; ch = DECL_NEXT(ch))
    ++nch;

  int phase;

  vector<std::set<Expression> > default_instance_assignments(
      aug_graph->instances.length, std::set<Expression>());
  vector<std::set<Expression> > instance_assignment =
      make_instance_assignment(aug_graph, block, default_instance_assignments);

  // the following loop is controlled in two ways:
  // (1) if total order is zero, there are no visits at all.
  // (2) otherwise, total_order never changes,
  //     but eventually when scheduling a phase, we find out
  //     that it is the last phase and we break the loop
  for (phase = 1; phase <= phy_graph->max_phase; phase++) {
#ifdef APS2SCALA
    os << indent() << "def visit_" << pgn << "_" << phase << "_" << j
       << "(anchor : T_" << decl_name(phy_graph->phylum)
       << ") : Unit = anchor match {\n";
    ++nesting_level;
    os << indent() << "case " << matcher_pat(m) << " => {\n";
#else  /* APS2SCALA */
    oss << header_return_type<Type>(0) << "void "
        << header_function_name("visit_") << pgn << "_" << phase << "_" << j
        << "(C_PHYLUM::Node* anchor)" << header_end();
    INDEFINITION;
    os << " {\n";
#endif /* APS2SCALA */
    ++nesting_level;
#ifndef APS2SCALA
    os << matcher_bindings("anchor", m);
    os << "\n";
#endif /* APS2SCALA */

    if (include_comments) {
      os << indent() << "// Implementing visit function for "
         << aug_graph_name(aug_graph) << " phase: " << phase << "\n";
    }

    CONDITION cond;
    cond.positive = 0;
    cond.negative = 0;

    OutputWriter ow(os);
    implement_visit_function(aug_graph, phase, total_order, instance_assignment,
                             nch, &cond, -1, s->loop_required, -1, true, &ow);

    --nesting_level;
#ifdef APS2SCALA
    os << indent() << "}\n";
    --nesting_level;
#endif /* APS2SCALA */
    os << indent() << "}\n";
  }
}

// The following function is only for Scala code generation
static void dump_constructor_owner(Declaration pd, ostream& os) {
  switch (Declaration_KEY(pd)) {
    default:
      aps_error(pd, "cannot attribute this phylum");
      break;
    case KEYphylum_formal:
      os << "t_" << decl_name(pd) << DEREF;
      break;
    case KEYphylum_decl:
      switch (Type_KEY(phylum_decl_type(pd))) {
        default:
          aps_error(pd, "cannot attribute this phylum");
          break;
        case KEYno_type:
          break;
        case KEYtype_inst:
          os << "t_" << decl_name(pd) << DEREF;
          break;
        case KEYtype_use:
          dump_constructor_owner(USE_DECL(type_use_use(phylum_decl_type(pd))),
                                 os);
          break;
      }
      break;
    case KEYtype_renaming:
      switch (Type_KEY(phylum_decl_type(pd))) {
        default:
          aps_error(pd, "cannot attribute this phylum");
          break;
        case KEYtype_use:
          dump_constructor_owner(USE_DECL(type_use_use(phylum_decl_type(pd))),
                                 os);
          break;
      }
      break;
  }
}

#ifdef APS2SCALA
static void dump_visit_functions(PHY_GRAPH* pg, ostream& os)
#else  /* APS2SCALA */
static void dump_visit_functions(PHY_GRAPH* pg, output_streams& oss)
#endif /* APS2SCALA */
{
  STATE* s = pg->global_state;
  int pgn = PHY_GRAPH_NUM(pg);

#ifdef APS2SCALA
  ostream& oss = os;
#else
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  // ostream& is = oss.is;
  ostream& os = inline_definitions ? hs : cpps;

#endif                            /* APS2SCALA */
  vector<AUG_GRAPH*> aug_graphs;  // not in order

  for (int i = 0; i < s->match_rules.length; ++i) {
    AUG_GRAPH* ag = &s->aug_graphs[i];
    if (ag->lhs_decl && Declaration_info(ag->lhs_decl)->node_phy_graph == pg) {
      aug_graphs.push_back(ag);
    }
  }

  int num_cons = aug_graphs.size();

  if (num_cons == 0) {
    fatal_error("no top-level-match match for phylum %s",
                decl_name(pg->phylum));
  }

  // The match clauses may be in a different order
  // that the constructor declarations in the tree module
  int cons_num = 0;
  Declaration cmodule =
      (Declaration)tnode_parent(aug_graphs.front()->syntax_decl);
  while (ABSTRACT_APS_tnode_phylum(cmodule) != KEYDeclaration ||
         Declaration_KEY(cmodule) != KEYmodule_decl)
    cmodule = (Declaration)tnode_parent(cmodule);
  Block cblock = module_decl_contents(cmodule);
  for (Declaration d = first_Declaration(block_body(cblock)); d;
       d = DECL_NEXT(d)) {
    if (Declaration_KEY(d) == KEYconstructor_decl) {
      for (int j = 0; j < num_cons; ++j) {
        if (aug_graphs[j]->syntax_decl == d) {
          Declaration_info(d)->instance_index = cons_num;
          ++cons_num;
          break;
        }
      }
    }
  }

  if (num_cons != cons_num) {
    fatal_error("Can't find all constructors");
  }

  for (int ph = 1; ph <= pg->max_phase; ++ph) {
#ifdef APS2SCALA
    os << indent() << "def visit_" << pgn << "_" << ph << "(node : T_"
       << decl_name(pg->phylum) << ") : Unit = node match {\n";
#else  /* APS2SCALA */
    oss << header_return_type<Type>(0) << "void "
        << header_function_name("visit_") << pgn << "_" << ph
        << "(C_PHYLUM::Node* node)" << header_end();
    INDEFINITION;
    os << " {\n";
#endif /* APS2SCALA */
    ++nesting_level;
#ifndef APS2SCALA
    os << indent() << "switch (node->cons->get_index()) {\n";
#endif /* APS2SCALA */
    for (int j = 0; j < num_cons; ++j) {
#ifdef APS2SCALA
      Declaration cd = aug_graphs[j]->syntax_decl;
      os << indent() << "case ";
      dump_constructor_owner(pg->phylum, os);
      os << "p_" << decl_name(cd) << "(_";
      Declarations fs = function_type_formals(constructor_decl_type(cd));
      for (Declaration f = first_Declaration(fs); f; f = DECL_NEXT(f)) {
        os << ",_";
      }
      os << ") => "
         << "visit_" << pgn << "_" << ph << "_"
         << Declaration_info(cd)->instance_index << "(node);\n";
#else  /* APS2SCALA */
      os << indent() << "case " << j << ":\n";
      ++nesting_level;
      os << indent() << "visit_" << pgn << "_" << ph << "_" << j << "(node);\n";
      os << indent() << "break;\n";
      --nesting_level;
#endif /* APS2SCALA */
    }
#ifndef APS2SCALA
    os << indent() << "default:\n";
    ++nesting_level;
    os << indent() << "throw std::runtime_error(\"bad constructor index\");\n";
#endif /* APS2SCALA */
    --nesting_level;
#ifdef APS2SCALA
    os << indent() << "};\n";
#else  /* APS2SCALA */
    os << indent() << "}\n";
    --nesting_level;
    os << indent() << "}\n";
#endif /* APS2SCALA */
  }

  // Now spit out visit procedures for each constructor
  for (int i = 0; i < num_cons; ++i) {
    dump_visit_functions(pg, aug_graphs[i], oss);
  }

  oss << "\n";  // some blank lines
}

#ifdef APS2SCALA
static void dump_visit_functions(STATE* s, ostream& os)
#else  /* APS2SCALA */
static void dump_visit_functions(STATE* s, output_streams& oss)
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
  int nphy = s->phyla.length;
  for (int j = 0; j < nphy; ++j) {
    PHY_GRAPH* pg = &s->phy_graphs[j];
    if (Declaration_KEY(pg->phylum) == KEYphylum_decl) {
      dump_visit_functions(pg, oss);
    }
  }

  Declaration sp = s->start_phylum;
#ifdef APS2SCALA
  os << indent() << "def visit() : Unit = {\n";
#else  /* APS2SCALA */
  oss << header_return_type<Type>(0) << "void " << header_function_name("visit")
      << "()" << header_end();
  INDEFINITION;
  os << " {\n";
#endif /* APS2SCALA */
  ++nesting_level;
#ifdef APS2SCALA
  os << indent() << "val roots = t_" << decl_name(sp) << ".nodes;\n";
#else  /* APS2SCALA */
  os << indent() << "Phylum* phylum = this->t_"
     << decl_name(sp)  //! bug sometimes
     << "->get_phylum();\n";
  os << indent() << "int n_roots = phylum->size();\n";
  os << "\n";  // blank line
#endif /* APS2SCALA */

  // printf("sp: %s and pointer %ld %d\n", decl_name(sp), (long)
  // (Declaration_info(s->start_phylum)->node_phy_graph),
  // tnode_line_number(s->));
  int phase = Declaration_info(s->module)->node_phy_graph->max_phase;

  vector<std::set<Expression> > default_instance_assignments(
      s->global_dependencies.instances.length, std::set<Expression>());
  vector<std::set<Expression> > instance_assignment = make_instance_assignment(
      &s->global_dependencies, module_decl_contents(s->module),
      default_instance_assignments);

  CONDITION cond;
  cond.positive = 0;
  cond.negative = 0;

  OutputWriter ow(os);
  implement_visit_function(
      &s->global_dependencies, phase, s->global_dependencies.total_order,
      instance_assignment, 1, &cond, -1, true, -1, false, &ow);
  --nesting_level;
  os << indent() << "}\n";
}

static void* dump_scheduled_local(void* pbs, void* node) {
  ostream& bs = *(ostream*)pbs;
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration d = (Declaration)node;
    if (Declaration_KEY(d) == KEYvalue_decl) {
      static int unique = 0;
      LOCAL_UNIQUE_PREFIX(d) = ++unique;
      Declaration_info(d)->decl_flags |= LOCAL_VALUE_FLAG;
#ifdef APS2SCALA
      bs << indent() << "var "
         << " v" << unique << "_" << decl_name(d) << " : " << value_decl_type(d)
         << " = null.asInstanceOf[" << value_decl_type(d) << "]"
         << ";\n";
#else  /* APS2SCALA */
      bs << indent() << value_decl_type(d) << " v" << unique << "_"
         << decl_name(d) << ";\n";
#endif /* APS2SCALA */
    }
  }
  return pbs;
}

static void dump_scheduled_function_body(Declaration fd, STATE* s, ostream& bs) {
  const char* name = decl_name(fd);
  Type ft = function_decl_type(fd);

  // dump any local values:
  traverse_Declaration(dump_scheduled_local, &bs, fd);

  int index;
  for (index = 0; index < s->match_rules.length; ++index)
    if (s->match_rules.array[index] == fd)
      break;

  if (index >= s->match_rules.length)
    fatal_error("Cannot find function %s in top-level-matches", name);

  int pindex;
  for (pindex = 0; pindex < s->phyla.length; ++pindex)
    if (s->phyla.array[pindex] == fd)
      break;

  if (pindex >= s->phyla.length)
    fatal_error("Cannot find function %s in phyla", name);

  AUG_GRAPH* aug_graph = &s->aug_graphs[index];
  CTO_NODE* schedule = aug_graph->total_order;

  vector<std::set<Expression> > default_instance_assignments(
      aug_graph->instances.length, std::set<Expression>());
  vector<std::set<Expression> > instance_assignment = make_instance_assignment(
      aug_graph, function_decl_body(fd), default_instance_assignments);

  CONDITION cond;
  cond.positive = 0;
  cond.negative = 0;

  int max_phase =
      Declaration_info(aug_graph->lhs_decl)->node_phy_graph->max_phase;

  OutputWriter ow(bs);
  bool cont = implement_visit_function(aug_graph, max_phase, schedule,
                                       instance_assignment, 0, &cond, -1, false,
                                       -1, false, &ow);

  Declaration returndecl = first_Declaration(function_type_return_values(ft));
  if (returndecl == 0) {
    bs << indent() << "return;\n";
  } else {
    bs << indent() << "return v" << LOCAL_UNIQUE_PREFIX(returndecl) << "_"
       << decl_name(returndecl) << ";\n";
  }

  if (cont) {
    std::cout << "Function " << name << " should not require a second pass!\n";
    int phase = 2;
    bs << "    /*\n";
    bs << "    // phase 2\n";

    while (implement_visit_function(aug_graph, phase, schedule,
                                    instance_assignment, 0, &cond, -1, false,
                                    -1, true, &ow)) {
      if (include_comments) {
        bs << indent() << "// phase " << ++phase << "\n";
      }
    }
    bs << indent() << "*/\n";
  }
}

class StaticScc : public Implementation {
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

    Declarations ds = block_body(module_decl_contents(module_decl));

    dump_visit_functions(s, oss);

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
    os << indent() << "visit();\n";

    // types actually should be scheduled...
    for (Declaration d = first_Declaration(ds); d; d = DECL_NEXT(d)) {
      const char* kind = NULL;
      switch (Declaration_KEY(d)) {
        case KEYphylum_decl:
        case KEYtype_decl:
          switch (Type_KEY(some_type_decl_type(d))) {
            case KEYno_type:
            case KEYtype_inst:
              kind = "t_";
              break;
            default:
              break;
          }
        default:
          break;
      }
      if (kind != NULL) {
        const char* n = decl_name(d);
        os << indent() << kind << n << DEREF << "finish();\n";
      }
    }
#ifdef APS2SCALA
    os << indent() << "super.finish();\n";
#endif /* ! APS2SCALA */
    --nesting_level;
    os << indent() << "}\n";

    clear_implementation_marks(module_decl);
  }
};

Super* get_module_info(Declaration m) {
  return new ModuleInfo(m);
}

void implement_function_body(Declaration f, ostream& os) {
  Declaration module = (Declaration)tnode_parent(f);
  while (module && (ABSTRACT_APS_tnode_phylum(module) != KEYDeclaration ||
                    Declaration_KEY(module) != KEYmodule_decl))
    module = (Declaration)tnode_parent(module);
  if (module) {
    STATE* s = (STATE*)Declaration_info(module)->analysis_state;
    dump_scheduled_function_body(f, s, os);
  } else {
    dynamic_impl->implement_function_body(f, os);
  }
}

void implement_value_use(Declaration vd, ostream& os) {
  int flags = Declaration_info(vd)->decl_flags;
  if (flags & LOCAL_ATTRIBUTE_FLAG) {
    os << "a" << LOCAL_UNIQUE_PREFIX(vd) << "_" << decl_name(vd) << DEREF
       << "get(anchor)";
  } else if (flags & ATTRIBUTE_DECL_FLAG) {
    os << "a"
       << "_" << decl_name(vd) << DEREF << "get";
  } else if (flags & LOCAL_VALUE_FLAG) {
    os << "v" << LOCAL_UNIQUE_PREFIX(vd) << "_" << decl_name(vd);
  } else {
    aps_error(vd, "internal_error: What is special about this?");
  }
}
}
;

Implementation* static_scc_impl = new StaticScc();
