#include <jbb.h>
#include <stdio.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

struct state {
  SccGraph* graph;
  AUG_GRAPH* aug_graph;
};

typedef struct state State;

static void zack(AUG_GRAPH* aug_graph,
                 TopologicalSortGraph* topological_graph,
                 SCC_COMPONENT* comp1,
                 int comp1_index,
                 SCC_COMPONENT* comp2,
                 int comp2_index) {
  int i, j, k;

  for (i = 0; i < comp1->length; i++) {
    INSTANCE* in1 = &aug_graph->instances.array[comp1->array[i]];

    for (j = 0; j < comp2->length; j++) {
      INSTANCE* in2 = &aug_graph->instances.array[comp2->array[j]];

      if (edgeset_kind(
              aug_graph->graph[in1->index * aug_graph->instances.length +
                               in2->index])) {
        // printf("%d -> %d\n", comp1_index, comp2_index);
        topological_sort_add_edge(topological_graph, comp1_index, comp2_index);
      }
    }
  }
}

static void taha(AUG_GRAPH* aug_graph) {
  TopologicalSortGraph* graph =
      topological_sort_graph_create(aug_graph->components.length);
  int i, j;

  for (i = 0; i < aug_graph->components.length; i++) {
    for (j = 0; j < aug_graph->components.length; j++) {
      if (i != j) {
        zack(aug_graph, graph, &aug_graph->components.array[i], i,
             &aug_graph->components.array[j], j);
      }
    }
  }

  TOPOLOGICAL_SORT_ORDER* order = topological_sort_order(graph);
  for (i = 0; i < order->length; i++) {
    if (i > 0) {
      printf(" -> ");
    }

    printf("%d", order->array[i]);
  }

  printf("\n");
  aug_graph->scc_order = *order;
}

static void analyze_state(STATE* s) {
  int i, j, k;
  /* augmented dependency graph */
  for (i = 0; i <= s->match_rules.length; i++) {
    AUG_GRAPH* aug_graph = (i == s->match_rules.length)
                               ? &s->global_dependencies
                               : &s->aug_graphs[i];
    int n = aug_graph->instances.length;
    SccGraph* graph = scc_graph_create(n);
    for (j = 0; j < n; j++) {
      for (k = 0; k < n; k++) {
        if (edgeset_kind(aug_graph->graph[j * n + k]) != no_dependency) {
          scc_graph_add_edge(graph, j, k);
        }
      }
    }
    aug_graph->components = scc_graph_components(graph);

    if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
      printf("Components of %s\n", decl_name(aug_graph->syntax_decl));
      for (j = 0; j < aug_graph->components.length; j++) {
        SCC_COMPONENT comp = aug_graph->components.array[j];
        printf(" Component #%d\n", j);

        for (k = 0; k < comp.length; k++) {
          printf("   ");
          print_instance(&aug_graph->instances.array[comp.array[k]], stdout);
          printf("\n");
        }
      }
      printf("\n");
    }

    taha(aug_graph);
  }
}

void state_scc(STATE* s) {
  analyze_state(s);
}
