#include <stdio.h>
#include <jbb.h>

#include "aps-ag.h"

/*
Several phases:
  1> fibering
  2> constructing production dependency graphs
  3> iterating to DNC solution
  4> fiber cycle detection and removal
  5> OAG construction and test.
  6> schedule creation.
  7> diagnostic output
*/

static void *analyze_thing(void *ignore, void *node)
{
  STATE *s;
  DEPENDENCY d;
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration decl = (Declaration)node;
    switch (Declaration_KEY(decl)) {
    case KEYmodule_decl:
      s = compute_dnc(decl);
      d = analysis_state_cycle(s);
      switch (d) {
      default:
	aps_error(decl,"Cycle detected; Attribute grammar is not DNC %d", d);
	break;
      case indirect_circular_dependency:
        break_fiber_cycles(decl,s, indirect_circular_dependency);
        compute_oag(decl,s);
        break;
      case indirect_control_fiber_dependency:
      case control_fiber_dependency:
      case indirect_fiber_dependency:
      case fiber_dependency:
	printf("Fiber cycle detected; cycle being removed\n");
	break_fiber_cycles(decl,s, fiber_dependency);
	/* fall through */
      case no_dependency:
	compute_oag(decl,s);
  d = analysis_state_cycle(s);
	switch (d) {
	case no_dependency:
	  break;
	default:
	  aps_error(decl,"Cycle detected; Attribute grammar is not OAG %d", d);
	  break;
	}
	break;
      }
      if (cycle_debug & PRINT_CYCLE) {
      print_cycles(s,stdout);
      }
      Declaration_info(decl)->analysis_state = s;
      break;
    }
    return NULL;
  }
  return ignore;
}

void analyze_Program(Program p)
{
  char *saved_filename = aps_yyfilename;
  aps_yyfilename = (char *)program_name(p);
  traverse_Program(analyze_thing,p,p);
  aps_yyfilename = saved_filename;
}
