#include <stdio.h>
#include <jbb.h>
#include "aps-ag.h"

int oag_debug;

void schedule_summary_dependency_graph(PHY_GRAPH *phy_graph) {
  int n = phy_graph->instances.length;
  int done = 0;
  int local_done = 0;
  int i,j;
  int phase = 0;
  if (oag_debug & TOTAL_ORDER) {
    printf("Scheduling order for %s\n",decl_name(phy_graph->phylum));
  }
  while (done < n) {
    ++phase; local_done = 0;
    /* find inherited instances for the phase. */
    for (i=0; i < n; ++i) {
      INSTANCE *in = &phy_graph->instances.array[i];
      if (instance_direction(in) == instance_inward &&
	  phy_graph->summary_schedule[i] == 0) {
	for (j=0; j < n; ++j) {
	  if (phy_graph->summary_schedule[j] == 0 &&
	      phy_graph->mingraph[j*n+i] != no_dependency)
	    break;
	}
	if (j == n) {
	  phy_graph->summary_schedule[i] = -phase;
	  ++done; ++local_done;
	  for (j=0; j < n; ++j) { /* force extra dependencies */
	    int sch = phy_graph->summary_schedule[j];
	    if (sch != 0 && sch != -phase)
	      phy_graph->mingraph[j*n+i] = fiber_dependency;
	  }
	  if (oag_debug & TOTAL_ORDER) {
	    printf("%d- ",phase);
	    print_instance(in,stdout);
	    printf("\n");
	  }
	}
      }
    }
    /* now schedule synthesized attributes */
    for (i=0; i < n; ++i) {
      INSTANCE *in = &phy_graph->instances.array[i];
      if (instance_direction(in) == instance_outward &&
	  phy_graph->summary_schedule[i] == 0) {
	for (j=0; j < n; ++j) {
	  if (phy_graph->summary_schedule[j] == 0 &&
	      phy_graph->mingraph[j*n+i] != no_dependency)
	    break;
	}
	if (j == n) {
	  phy_graph->summary_schedule[i] = phase;
	  ++done; ++local_done;
	  for (j=0; j < n; ++j) { /* force extra dependencies */
	    int sch = phy_graph->summary_schedule[j];
	    if (sch != 0 && sch != phase)
	      phy_graph->mingraph[j*n+i] = fiber_dependency;
	  }
	  if (oag_debug & TOTAL_ORDER) {
	    printf("%d+ ",phase);
	    print_instance(in,stdout);
	    printf("\n");
	  }
	}
      }
    }
      if (local_done == 0)
	fatal_error("Cycle detected when scheduling phase %d for %s",
		    phase,decl_name(phy_graph->phylum));
  }
}

void schedule_augmented_dependency_graph(AUG_GRAPH *aug_graph) {
  (void)close_augmented_dependency_graph(aug_graph);
}

void compute_oag(Declaration module, STATE *s) {
  int j;
  for (j=0; j < s->phyla.length; ++j) {
    schedule_summary_dependency_graph(&s->phy_graphs[j]);
  }
  for (j=0; j < s->match_rules.length; ++j) {
    if (analysis_debug & DNC_ITERATE) {
      printf("Checking rule %d\n",j);
    }
    schedule_augmented_dependency_graph(&s->aug_graphs[j]);
  }
  if (analysis_debug & DNC_ITERATE) {
    printf("Checking global dependencies\n",j);
  }
  schedule_augmented_dependency_graph(&s->global_dependencies);
  if (analysis_debug & (DNC_ITERATE|DNC_FINAL)) {
    printf("*** FINAL OAG ANALYSIS STATE ***\n");
    print_analysis_state(s,stdout);
    print_cycles(s,stdout);
  }
}



