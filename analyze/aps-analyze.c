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
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration decl = (Declaration)node;
    switch (Declaration_KEY(decl)) {
    case KEYmodule_decl:
      s = compute_dnc(decl);
      switch (analysis_state_cycle(s)) {
      case dependency:
	aps_error(decl,"Cycle detected; Attribute grammar is not DNC");
	break;
      case fiber_dependency:
	printf("Fiber cycle detected: cycle being removed\n");
	break_fiber_cycles(decl,s);
	/* fall through */
      case no_dependency:
	compute_oag(decl,s);
	break;
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
  traverse_Program(analyze_thing,p,p);
}
