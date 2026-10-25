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

// Boolean indicating whether to use SCC chunk scheduling
bool static_scc_schedule = false;
bool anc_analysis = false;

static BOOL global_dependencies_have_cycle(STATE *s)
{
  AUG_GRAPH *aug_graph = &s->global_dependencies;
  int n = aug_graph->instances.length;
  int i;

  for (i = 0; i < n; ++i) {
    if (edgeset_kind(aug_graph->graph[i*n+i]) != no_dependency) return TRUE;
  }
  return FALSE;
}

static void *check_circular_declarations(void *context, void *node)
{
  STATE *s = (STATE *)context;
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration && decl_is_circular((Declaration)node)) {
    s->requires_circular_update_support = TRUE;
  }
  return context;
}

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
      s = compute_dnc(decl, anc_analysis);
      if (anc_analysis) {
        if (cycle_debug & PRINT_CYCLE)
        {
          print_cycles(s, stdout);
        }
        d = (s->original_state_dependency = analysis_state_cycle(s));
        s->requires_circular_update_support = d != no_dependency;
        traverse_Declaration(check_circular_declarations, s, decl);
        s->requires_fixed_point_loop = global_dependencies_have_cycle(s);
        break;
      }
      if (!(d = (s->original_state_dependency = analysis_state_cycle(s))))
      {
        // Do nothing; no cycle to remove
        s->requires_fixed_point_loop = false;
      }
      else if (!(d & DEPENDENCY_MAYBE_SIMPLE) || !(d & DEPENDENCY_NOT_JUST_FIBER))
      {
        printf("Fiber cycle detected (%d); cycle being removed\n", d);
        if (cycle_debug & PRINT_CYCLE)
        {
          print_cycles(s, stdout);
        }
        break_fiber_cycles(decl, s, d);
        s->requires_fixed_point_loop = !(d & DEPENDENCY_MAYBE_SIMPLE);
        d = no_dependency;  // clear dependency
      }
      else
      {
        aps_error(decl, "Unable to handle dependency (%d); Attribute grammar is not DNC", d);
        return NULL;
      }

      s->requires_circular_update_support = s->requires_fixed_point_loop;

      // If SCC scheduling is in-progress
      if (static_scc_schedule)
      {
        // Pure fiber cycles should have been broken when reaching this step
        // If there is a non-monotone cycle, then scheduling is no longer possible
        if (d & DEPENDENCY_MAYBE_SIMPLE)
        {
          if (cycle_debug & PRINT_CYCLE)
          {
            print_cycles(s, stdout);
          }

          aps_error(decl, "Non-monotone Cycle detected (%d); Attribute grammar is not COAG", d);
          return NULL;
        }

        // SCC chunk scheduling supports CRAG without conditional cycles
        compute_static_schedule(s);
      }
      else
      {
        if (!d)
        {
          // RAG scheduling with conditional cycles
          compute_oag(decl, s);  // calculate OAG if grammar is DNC
          d = analysis_state_cycle(s); // check again for type-3 errors
        }

        if (d)
        {
          if (cycle_debug & PRINT_CYCLE)
          {
            print_cycles(s, stdout);
          }

          aps_error(decl, "Cycle detected (%d); Attribute grammar is not OAG", d);
          return NULL;
        }
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
