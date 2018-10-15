#include <iostream>
extern "C" {
#include <stdio.h>
#include "aps-ag.h"
}
#include "dump-cpp.h"
#include "implement.h"

#define LOCAL_VALUE_FLAG (1<<28)

using namespace std;

/** Return phase (synthesized) or -phase (inherited)
 * for fibered attribute, given the phylum's summary dependence graph.
 */
int attribute_schedule(PHY_GRAPH *phy_graph, const FIBERED_ATTRIBUTE& key)
{
  int n = phy_graph->instances.length;
  for (int i=0; i < n; ++i) {
    const FIBERED_ATTRIBUTE& fa = phy_graph->instances.array[i].fibered_attr;
    if (fa.attr == key.attr && fa.fiber == key.fiber)
      return phy_graph->summary_schedule[i];
  }
  fatal_error("Could not find summary schedule for instance");
  return 0;
}

Expression default_init(Default def)
{
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
Expression* make_instance_assignment(AUG_GRAPH* aug_graph,
				     Block block,
				     Expression from[])
{
  int n = aug_graph->instances.length;
  Expression* array = new Expression[n];

  if (from) {
    for (int i=0; i < n; ++i) {
      array[i] = from[i];
    }
  } else {
    for (int i=0; i < n; ++i) {
      INSTANCE *in = &aug_graph->instances.array[i];
      array[i] = 0;
      Declaration ad = in->fibered_attr.attr;
      if (ad != 0 && in->fibered_attr.fiber == 0 &&
	  ABSTRACT_APS_tnode_phylum(ad) == KEYDeclaration) {
	// get default!
	switch (Declaration_KEY(ad)) {
	case KEYattribute_decl:
	  array[i] = default_init(attribute_decl_default(ad));
	  break;
	case KEYvalue_decl:
	  array[i] = default_init(value_decl_default(ad));
	  break;
	default:
	  break;
	}
      }
    }
  }  
  
  Declarations ds = block_body(block);
  for (Declaration d = first_Declaration(ds); d; d = DECL_NEXT(d)) {
    switch (Declaration_KEY(d)) {
    case KEYassign:
      if (INSTANCE* in = Expression_info(assign_rhs(d))->value_for) {
	if (in->index >= n) fatal_error("bad index for instance");
	array[in->index] = assign_rhs(d);
      }
      break;
    default:
      break;
    }
  }

  return array;
}

// visit procedures are called:
// visit_n_m
// where n is the number of the phy_graph and m is the phase.
// This does a dispatch to visit_n_m_p
// where p is the production number (0-based constructor index)
#define PHY_GRAPH_NUM(pg) (pg - pg->global_state->phy_graphs)

// return true if there are still more instances after this phase:
static bool implement_visit_function(AUG_GRAPH* aug_graph,
				     int phase, /* phase to impl. */
				     int current, /* phase currently at. */
				     CTO_NODE* cto,
				     Expression instance_assignment[],
				     int nch,
				     Declaration children[],
				     int child_phase[],
				     ostream& os)
{
  // STATE *s = aug_graph->global_state;

  for (; cto; cto = cto->cto_next) {
    INSTANCE* in = cto->cto_instance;
    
    Declaration ad = in->fibered_attr.attr;
    
    bool node_is_lhs = in->node == aug_graph->lhs_decl;
    int ch=0; /* == nch if not a child */
    while (ch < nch && children[ch] != in->node) ++ch;
    bool node_is_syntax = ch < nch || node_is_lhs;
    
    PHY_GRAPH* npg = node_is_syntax ?
      Declaration_info(in->node)->node_phy_graph : 0;
    int ph = node_is_syntax ? attribute_schedule(npg,in->fibered_attr) : -1;

    // check for phase change of parent:
    if (node_is_lhs && ph != current && ph != -current) {
      // phase change!
      if (ph != -(current+1)) {
	cout << "Phase " << ph << ": " << in;
	cout << " out of order (expected -" << current+1 << ")" << endl;
	cout << endl;
      }
      if (current == phase) return true; // phase is over
      ++current;
    }

    // check for phase change of child:
    // we allow inherited attributes of next phase to go by:
    if (ch < nch && ph != child_phase[ch] && ph != -child_phase[ch]-1) {
      if (current == phase) {
	bool is_mod = Declaration_KEY(in->node) == KEYmodule_decl;

	if (is_mod) {
	  os << indent() << "for (int i=0; i < n_roots; ++i) {\n";
	  ++nesting_level;
	  os << indent() << "C_PHYLUM::Node *root = phylum->node(i);\n";
	}
	os << indent() << "visit_" << PHY_GRAPH_NUM(npg)
	   << "_" << ph << "(";	
	if (is_mod)
	  os << "root";
	else
	  os << "v_" << decl_name(in->node);
	os << ");\n";
	if (is_mod) {
	  --nesting_level;
	  os << indent() << "}\n";
	}
      }
      ++child_phase[ch];
      bool cont = implement_visit_function(aug_graph,phase,current,cto,
					   instance_assignment,
					   nch,children,child_phase,os);
      --child_phase[ch];
      return cont;
    }

    if (if_rule_p(ad)) {
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
	  Type ty = infer_expr_type(e);
	  os << indent() << ty << " node=" << e << ";\n";
	}
	os << indent() << "if (";
	dump_Pattern_cond(p,"node",os);
	os << ") {\n";
	nesting_level+=1;
	dump_Pattern_bindings(p,os);
	if_true = matcher_body(m);
	if (MATCH_NEXT(m)) {
	  if_false = 0; //? Why not the nxt match ?
	} else {
	  if_false = case_stmt_default(header);
	}
      } else {
	// Symbol boolean_symbol = intern_symbol("Boolean");
	os << indent() << "if (" << if_stmt_cond(ad) << ") {\n";
	++nesting_level;
	if_true = if_stmt_if_true(ad);
	if_false = if_stmt_if_false(ad);
      }
      Expression* true_assignment =
	make_instance_assignment(aug_graph,if_true,instance_assignment);
      implement_visit_function(aug_graph,phase,current,cto->cto_if_true,
			       true_assignment,
			       nch,children,child_phase,os);
      delete[] true_assignment;
      --nesting_level;
      os << indent() << "} else {\n";
      ++nesting_level;
      Expression* false_assignment = if_false
	? make_instance_assignment(aug_graph,if_false,instance_assignment)
	: instance_assignment;
      bool cont = implement_visit_function(aug_graph,phase,current,
					   cto->cto_if_false,
					   false_assignment,
					   nch,children,child_phase,os);
      if (if_false) delete[] false_assignment;
      --nesting_level;
      os << indent() << "}\n";
      return cont;
    }

    // otherwise, if we're not yet in proper phase,
    // we skip along:
    if (phase != current) continue;

    Symbol asym = ad ? def_name(declaration_def(ad)) : 0;
    
    if (instance_direction(in) == instance_inward) {
      os << indent() << "// " << in
	 << " is ready now." << endl;
      continue;
    }

    Expression rhs = instance_assignment[in->index];

    if (in->node && Declaration_KEY(in->node) == KEYnormal_assign) {
      // parameter value will be filled in at call site
      os << indent() << "// delaying " << in << " to call site." << endl;
      continue;
    }

    if (in->node && Declaration_KEY(in->node) == KEYpragma_call) {
      os << indent() << "// place holder for " << in << endl;
      continue;
    }

    if (in->fibered_attr.fiber != 0) {
      if (rhs == 0) {
	os << indent() << "// " << in << endl;
	continue;
      }

      Declaration assign = (Declaration)tnode_parent(rhs);
      Expression lhs = assign_lhs(assign);
      Declaration field = 0;
      os << indent();
      // dump the object containing the field
      switch (Expression_KEY(lhs)) {
      case KEYvalue_use:
	// shared global collection
	field = USE_DECL(value_use_use(lhs));
	os << "v_" << decl_name(field) << "=";
	switch (Default_KEY(value_decl_default(field))) {
	case KEYcomposite:
	  os << composite_combiner(value_decl_default(field));
	  break;
	default:
	  os << as_val(value_decl_type(field)) << "->v_combine";
	  break;
	}
	os << "(v_" << decl_name(field) << "," << rhs << ");\n";
	break;
      case KEYfuncall:
	field = field_ref_p(lhs);
	if (field == 0) fatal_error("what sort of assignment lhs: %d",
				    tnode_line_number(assign));
	os << "a_" << decl_name(field) << "->";
	if (debug) os << "assign"; else os << "set";
	os << "(" << field_ref_object(lhs) << "," << rhs << ");\n";
	break;
      default:
	fatal_error("what sort of assignment lhs: %d",
		    tnode_line_number(assign));
      }
      continue;
    }

    os << indent();
    
    if (in->node == 0 && ad != 0) {
      if (rhs) {
	if (Declaration_info(ad)->decl_flags & LOCAL_ATTRIBUTE_FLAG) {
	  os << "a" << LOCAL_UNIQUE_PREFIX(ad) << "_" << asym << "->";
	  if (debug) os << "assign"; else os << "set";
	  os << "(anchor," << rhs << ");\n";
	} else {
	  int i = LOCAL_UNIQUE_PREFIX(ad);
	  if (i == 0)
	    os << "v_" << asym << " = " << rhs << ";\n";
	  else
	    os << "v" << i << "_" << asym << " = " << rhs << ";\n";
	}
      } else {
	if (Declaration_KEY(ad) == KEYvalue_decl &&
	    !direction_is_collection(value_decl_direction(ad))) {
	  aps_warning(ad,"Local attribute %s is apparently undefined",
			  decl_name(ad));
        }
	os << "// " << in << " is ready now\n";
      }
      continue;
    } else if (node_is_syntax) {
      if (ATTR_DECL_IS_SHARED_INFO(ad) && ch < nch) {
	os << "// shared info for " << decl_name(in->node)
	   << " is ready." << endl;
      } else if (ATTR_DECL_IS_UP_DOWN(ad)) {
	os << "// " << decl_name(in->node) << "." << decl_name(ad) 
	   << " implicit." << endl;
      } else if (rhs) {
	if (Declaration_KEY(in->node) == KEYfunction_decl) {
	  if (direction_is_collection(value_decl_direction(ad))) {
	    cout << "Not expecting collection here!" << endl;
	    os << "v_" << asym << " = somehow_combine(v_"
	       << asym << "," << rhs << ");\n";
	  } else {
	    int i = LOCAL_UNIQUE_PREFIX(ad);
	    if (i == 0)
	      os << "v_" << asym << " = " << rhs << ";\n";
	    else
	      os << "v" << i << "_" << asym << " = " << rhs << ";\n";
	  }
	} else {
	  os << "a_" << asym << "->";
	  if (debug) os << "assign"; else os << "set";
	  os << "(v_" << decl_name(in->node)
	     << "," << rhs << ");\n";
	}
      } else {
	aps_warning(in->node,"Attribute %s.%s is apparently undefined",
		    decl_name(in->node),symbol_name(asym));
	os << "// " << in << " is ready.\n";
      }
      continue;
    } else if (Declaration_KEY(in->node) == KEYvalue_decl) {
      if (rhs) {
	// assigning field of object
	os << "a_" << asym << "->";
	if (debug) os << "assign"; else os << "set";
	os << "(v_" << decl_name(in->node)
	   << "," << rhs << ");\n";
      } else {
	os << "// " << in << " is ready now.\n";
      }
      continue;
    }
    cout << "Problem assigning " << in << endl;
    os << "// Not sure what to do for " << in << endl;
  }
  return 0; // no more!
}

// dump visit functions for constructors
void dump_visit_functions(PHY_GRAPH *phy_graph,
			  AUG_GRAPH *aug_graph,
			  const output_streams& oss)
{
  Declaration tlm = aug_graph->match_rule;
  Match m = top_level_match_m(tlm);
  Block block = matcher_body(m);

  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  // ostream& is = oss.is;
  ostream& bs = inline_definitions ? hs : cpps;

  int pgn = PHY_GRAPH_NUM(phy_graph);
  int j = Declaration_info(aug_graph->syntax_decl)->instance_index;
  CTO_NODE *total_order = aug_graph->total_order;

  /* Count the children */
  int nch = 0;
  for (Declaration ch = aug_graph->first_rhs_decl; ch != 0; ch=DECL_NEXT(ch))
    ++nch;
  Declaration *children = new Declaration[nch];
  nch = 0;
  for (Declaration ch = aug_graph->first_rhs_decl; ch != 0; ch=DECL_NEXT(ch))
    children[nch++] = ch;  
  
  int phase = 0;
  int *child_phase = new int[nch];
  for (int i=0; i < nch; ++i) child_phase[i] = 0;

  Expression* instance_assignment =
    make_instance_assignment(aug_graph,block,0);

  // the following loop is controlled in two ways:
  // (1) if total order is zero, there are no visits at all.
  // (2) otherwise, total_order never changes,
  //     but eventually when scheduling a phase, we find out
  //     that it is the last phase and we break the loop
  while (total_order) {
    ++phase;

    oss << header_return_type<Type>(0) << "void "
	<< header_function_name("visit_") << pgn << "_" << phase << "_" << j
	<< "(C_PHYLUM::Node* anchor)" << header_end();
    INDEFINITION;
    bs << " {\n";
    ++nesting_level;
    bs << matcher_bindings("anchor",m);
    bs << "\n";

    bool cont =
      implement_visit_function(aug_graph,phase,0,total_order,
			       instance_assignment,
			       nch,children,child_phase,bs);

    --nesting_level;
    bs << indent() << "}\n" << endl;

    if (!cont) break;
  }
    
  delete[] instance_assignment;
}

void dump_visit_functions(PHY_GRAPH *pg, const output_streams& oss)
{
  STATE *s = pg->global_state;
  int pgn = PHY_GRAPH_NUM(pg);

  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  // ostream& is = oss.is;
  ostream& bs = inline_definitions ? hs : cpps;

  int max_phase = 0;
  for (int i=0; i < pg->instances.length; ++i) {
    int ph = pg->summary_schedule[i];
    if (ph < 0) ph = -ph;
    if (ph > max_phase) max_phase = ph;
  }

  vector<AUG_GRAPH*> aug_graphs; // not in order

  for (int i=0; i < s->match_rules.length; ++i) {
    AUG_GRAPH *ag = &s->aug_graphs[i];
    if (ag->lhs_decl &&
	Declaration_info(ag->lhs_decl)->node_phy_graph == pg) {
      aug_graphs.push_back(ag);
    }
  }
  
  int num_cons = aug_graphs.size();

  if (num_cons == 0) {
    fatal_error("no constructors for phylum %s", decl_name(pg->phylum));
  }
  
  int cons_num = 0;
  Declaration cmodule =
    (Declaration)tnode_parent(aug_graphs.front()->syntax_decl);
  while (ABSTRACT_APS_tnode_phylum(cmodule) != KEYDeclaration ||
	 Declaration_KEY(cmodule) != KEYmodule_decl)
    cmodule = (Declaration)tnode_parent(cmodule);
  Block cblock = module_decl_contents(cmodule);
  for (Declaration d = first_Declaration(block_body(cblock));
       d; d=DECL_NEXT(d)) {
    if (Declaration_KEY(d) == KEYconstructor_decl) {
      for (int j=0; j < num_cons; ++j) {
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
  
  for (int ph = 1; ph <= max_phase; ++ph) {
    oss << header_return_type<Type>(0) << "void "
	<< header_function_name("visit_") << pgn << "_" << ph
	<< "(C_PHYLUM::Node* node)" << header_end();
    INDEFINITION;
    bs << " {\n";
    ++nesting_level;
    bs << indent() << "switch (node->cons->get_index()) {\n";
    for (int j=0; j < num_cons; ++j) {
      bs << indent() << "case " << j << ":\n";
      ++nesting_level;
      bs << indent() << "visit_" << pgn << "_" << ph << "_" << j
	 << "(node);\n";
      bs << indent() << "break;\n";
      --nesting_level;
    }
    bs << indent() << "default:\n";
    ++nesting_level;
    bs << indent()
       << "throw std::runtime_error(\"bad constructor index\");\n";
    --nesting_level;
    bs << indent() << "}\n";
    --nesting_level;
    bs << indent() << "}\n";
  }

  // Now spit out visit procedures for each constructor
  for (int i=0; i < num_cons; ++i) {
    dump_visit_functions(pg,aug_graphs[i],oss);
  }

  oss << "\n"; // some blank lines
}

void dump_visit_functions(STATE*s, const output_streams& oss)
{
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  // ostream& is = oss.is;
  ostream& bs = inline_definitions ? hs : cpps;

  // first dump all visit functions for each phylum:
  int nphy = s->phyla.length;
  for (int j=0; j < nphy; ++j) {
    PHY_GRAPH *pg = &s->phy_graphs[j];
    if (Declaration_KEY(pg->phylum) == KEYphylum_decl) {
      dump_visit_functions(pg,oss);
    }
  }
  
  Declaration sp = s->start_phylum;
  oss << header_return_type<Type>(0) << "void "
      << header_function_name("visit") << "()" << header_end();
  INDEFINITION;
  bs << " {\n";
  ++nesting_level;
  bs << indent() << "Phylum* phylum = this->t_" << decl_name(sp) //! bug sometimes
     << "->get_phylum();\n";
  bs << indent() << "int n_roots = phylum->size();\n";
  bs << "\n"; // blank line

  int phase = 1;
  Declaration root_decl[1] ;
  root_decl[0] = s->module;
  int root_phase[1];
  root_phase[0] = 0;

  Expression* instance_assignment =
    make_instance_assignment(&s->global_dependencies,
			     module_decl_contents(s->module),0);
  
  while (implement_visit_function(&s->global_dependencies,phase,0,
				  s->global_dependencies.total_order,
				  instance_assignment,
				  1,root_decl,root_phase,bs))
    ++phase;

  delete[] instance_assignment;

  --nesting_level;
  bs << indent() << "}\n\n";
}

static void* dump_scheduled_local(void *pbs, void *node) {
  ostream& bs = *(ostream*)pbs;
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration d = (Declaration)node;
    if (Declaration_KEY(d) == KEYvalue_decl) {
      static int unique = 0;
      LOCAL_UNIQUE_PREFIX(d) = ++unique;
      Declaration_info(d)->decl_flags |= LOCAL_VALUE_FLAG;
      bs << indent() << value_decl_type(d)
	 << " v" << unique << "_" << decl_name(d) << ";\n";
    }
  }
  return pbs;
}

void dump_scheduled_function_body(Declaration fd, STATE*s, ostream& bs)
{
  char *name = decl_name(fd);
  Type ft = function_decl_type(fd);

  // dump any local values:
  traverse_Declaration(dump_scheduled_local,&bs,fd);

  int index;
  for (index = 0; index < s->match_rules.length; ++index)
    if (s->match_rules.array[index] == fd) break;

  if (index >= s->match_rules.length)
    fatal_error("Cannot find function %s in top-level-matches",name);

  int pindex;
  for (pindex = 0; pindex < s->phyla.length; ++pindex)
    if (s->phyla.array[pindex] == fd) break;
  
  if (pindex >= s->phyla.length)
    fatal_error("Cannot find function %s in phyla",name);

  AUG_GRAPH *aug_graph = &s->aug_graphs[index];
  CTO_NODE *schedule = aug_graph->total_order;

  Expression* instance_assignment =
    make_instance_assignment(aug_graph,function_decl_body(fd),0);

  bool cont = implement_visit_function(aug_graph,1,0,schedule,
				       instance_assignment,0,0,0,bs);

  Declaration returndecl = first_Declaration(function_type_return_values(ft));
  if (returndecl == 0) {
    bs << indent() << "return;" << endl;
  } else {
    bs << indent() << "return v" << LOCAL_UNIQUE_PREFIX(returndecl)
       << "_" << decl_name(returndecl) << ";" << endl;
  }

  if (cont) {
    cout << "Function " << name << " should not require a second pass!"
	 << endl;
    int phase = 2;
    bs << "    /*" << endl;
    bs << "    // phase 2" << endl;
    while (implement_visit_function(aug_graph,phase,0,schedule,
				    instance_assignment,0,0,0,bs)) 
      bs << "    // phase " << ++phase << endl;
    bs << "    */" << endl;
  }
  
  delete[] instance_assignment;
}

class Static : public Implementation
{
public:
  typedef Implementation::ModuleInfo Super;
  class ModuleInfo : public Super {
  public:
    ModuleInfo(Declaration mdecl) : Implementation::ModuleInfo(mdecl) {}

    void note_top_level_match(Declaration tlm, const output_streams& oss) {
      Super::note_top_level_match(tlm,oss);
    }

    void note_local_attribute(Declaration ld, const output_streams& oss) {
      Super::note_local_attribute(ld,oss);
      Declaration_info(ld)->decl_flags |= LOCAL_ATTRIBUTE_FLAG;
    }
    
    void note_attribute_decl(Declaration ad, const output_streams& oss) {
      Declaration_info(ad)->decl_flags |= ATTRIBUTE_DECL_FLAG;
      Super::note_attribute_decl(ad,oss);
    }
    
    void note_var_value_decl(Declaration vd, const output_streams& oss) {
      Super::note_var_value_decl(vd,oss);
    }

    void implement(const output_streams& oss) {
      STATE *s = (STATE*)Declaration_info(module_decl)->analysis_state;

      ostream& hs = oss.hs;
      ostream& cpps = oss.cpps;
      ostream& bs = inline_definitions ? hs : cpps;
      // char *name = decl_name(module_decl);

      Declarations ds = block_body(module_decl_contents(module_decl));
      
      dump_visit_functions(s,oss);

      // Implement finish routine:
      hs << indent() << "void finish()";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << "void " << oss.prefix << "finish()";
      }
      INDEFINITION;
      bs << " {\n";
      ++nesting_level;
      bs << indent() << "visit();\n";
      // types actually should be scheduled...
      for (Declaration d = first_Declaration(ds); d; d = DECL_NEXT(d)) {
	char* kind = NULL;
	switch(Declaration_KEY(d)) {
	case KEYphylum_decl:
	case KEYtype_decl:
	  kind = "t_";
	  break;
	default:
	  break;
	}
	if (kind != NULL) {
	  char *n = decl_name(d);
	  bs << indent() << kind << n << "->finish();\n";
	}
      }
      --nesting_level;
      bs << indent() << "}\n\n";

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
      STATE *s = (STATE*)Declaration_info(module)->analysis_state;
      dump_scheduled_function_body(f,s,os);
    } else {
      dynamic_impl->implement_function_body(f,os);
    }
  }

  void implement_value_use(Declaration vd, ostream& os) {
    int flags = Declaration_info(vd)->decl_flags;
    if (flags & LOCAL_ATTRIBUTE_FLAG) {
      os << "a" << LOCAL_UNIQUE_PREFIX(vd) << "_"
	 << decl_name(vd) << "->get(anchor)"; 
    } else if (flags & ATTRIBUTE_DECL_FLAG) {
      os << "a" << "_" << decl_name(vd) << "->get";
    } else if (flags & LOCAL_VALUE_FLAG) {
      os << "v" << LOCAL_UNIQUE_PREFIX(vd) << "_" << decl_name(vd);
    } else {
      aps_error(vd,"internal_error: What is special about this?");
    }
  }
};

// not working yet.

Implementation *static_impl = new Static();
