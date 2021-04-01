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
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration)
  {
    Declaration decl = (Declaration)node;
    switch (Declaration_KEY(decl))
    {
    case KEYmodule_decl:
      s = compute_dnc(decl);
      if (!(d = analysis_state_cycle(s)))
      {
        // Do nothing; no cycle to remove
      }
      else if (!(d & DEPENDENCY_MAYBE_SIMPLE) || !(d & DEPENDENCY_NOT_JUST_FIBER))
      {
        printf("Fiber cycle detected; cycle being removed\n");
        break_fiber_cycles(decl, s);
        d = 0;  // clear dependency
      }
      else
      {
        aps_error(decl, "Unable to handle dependency (%d); Attribute grammar is not DNC", d);
      }

      if (!d) {
	compute_oag(decl, s); // calculate OAG if grammar is DNC
	d = analysis_state_cycle(s); // check again for type-3 errors
      }
      
      if (d)
      {
        if (cycle_debug & PRINT_CYCLE)
        {
          print_cycles(s, stdout);
        }

        aps_error(decl, "Cycle detected (%d); Attribute grammar is not OAG", d);
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
