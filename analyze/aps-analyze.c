#include <stdio.h>
#include <jbb.h>

#include "aps-ag.h"
#include "aps-schedule.h"

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
    default: break;
    case KEYmodule_decl:
      s = compute_dnc(decl);
      if (!(d = (s->original_state_dependency = analysis_state_cycle(s))))
      {
        // Do nothing; no cycle to remove
        s->loop_required = false;
      }
      else if (!(d & DEPENDENCY_MAYBE_SIMPLE) || !(d & DEPENDENCY_NOT_JUST_FIBER))
      {
        printf("Fiber cycle detected (%d); cycle being removed\n", d);

        if (cycle_debug & PRINT_CYCLE)
        {
          print_cycles(s, stdout);
        }
        break_fiber_cycles(decl, s, d);
        s->loop_required = !(d & DEPENDENCY_MAYBE_SIMPLE);
        d = no_dependency;  // clear dependency
      }

      d = analysis_state_cycle(s); // check again for type-3 errors

      // Pure fiber cycles should have been broken when reaching this step
      if (d & DEPENDENCY_MAYBE_SIMPLE)
      {
        if (cycle_debug & PRINT_CYCLE)
        {
          print_cycles(s, stdout);
        }

        aps_error(decl, "Cycle detected (%d); Attribute grammar is not OAG", d);
      }

      compute_static_schedule(s);  // calculate OAG if grammar is DNC
      d = analysis_state_cycle(s); // check again for type-3 errors

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
