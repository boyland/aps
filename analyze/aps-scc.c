#include <jbb.h>
#include <stdio.h>
#include <string.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

struct state {
  SccGraph* graph;
  AUG_GRAPH* aug_graph;
};

typedef struct state State;

static void add_scc_edge(AUG_GRAPH* aug_graph,
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
        if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
          printf("%d -> %d\t", comp1_index, comp2_index);
          print_instance(in1, stdout);
          printf(" -> ");
          print_instance(in2, stdout);
          printf("\n");
        }
        topological_sort_add_edge(topological_graph, comp1_index, comp2_index);
      }
    }
  }
}

static void topological_sort_component(AUG_GRAPH* aug_graph,
                                       SCC_COMPONENT* comp) {
  TopologicalSortGraph* graph = topological_sort_graph_create(comp->length);

  printf("topological sorting circular component %s\n",
         aug_graph_name(aug_graph));
  int i, j, k;

  for (i = 0; i < comp->length; i++) {
    INSTANCE* in1 = &aug_graph->instances.array[comp->array[i]];

    for (j = 0; j < comp->length; j++) {
      INSTANCE* in2 = &aug_graph->instances.array[comp->array[j]];

      if (i != j &&
          edgeset_kind(
              aug_graph->graph[in1->index * aug_graph->instances.length +
                               in2->index])) {
        if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
          printf("%d -> %d\t", in1->index, in2->index);
          print_instance(in1, stdout);
          printf(" -> ");
          print_instance(in2, stdout);
          printf("\n");
        }
        topological_sort_add_edge(graph, i, j);
      }
    }
  }

  TOPOLOGICAL_SORT_ORDER* order = topological_sort_order(graph);

  for (i = 0; i < comp->length; i++) {
    comp->array[i] =
        aug_graph->instances.array[comp->array[order->array[i]]].index;
  }

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    for (i = 0; i < comp->length; i++) {
      if (i > 0) {
        printf(" -> ");
      }

      print_instance(&aug_graph->instances.array[comp->array[i]], stdout);
    }

    printf("\n");
  }
}

static void analyze_sccs(AUG_GRAPH* aug_graph) {
  TopologicalSortGraph* graph =
      topological_sort_graph_create(aug_graph->components.length);
  int i, j;

  for (i = 0; i < aug_graph->components.length; i++) {
    for (j = 0; j < aug_graph->components.length; j++) {
      if (i != j) {
        add_scc_edge(aug_graph, graph, &aug_graph->components.array[i], i,
                     &aug_graph->components.array[j], j);
      }
    }
  }

  TOPOLOGICAL_SORT_ORDER* order = topological_sort_order(graph);
  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    for (i = 0; i < order->length; i++) {
      if (i > 0) {
        printf(" -> ");
      }

      printf("%d", order->array[i]);
    }

    printf("\n");
  }
  aug_graph->scc_order = *order;

  int* buffer_array = (int*)alloca(sizeof(int) * aug_graph->components.length);
  int buffer_count = 0;
  int count_consolidated_sccs = 0;

  SCC_COMPONENTS* consolidated_sccs =
      (SCC_COMPONENTS*)malloc(sizeof(SCC_COMPONENTS));
  VECTORALLOC(*consolidated_sccs, SCC_COMPONENT, aug_graph->components.length);

  BOOL* consolidated_sccs_cycles =
      (BOOL*)malloc(sizeof(BOOL) * aug_graph->components.length);
  memset(consolidated_sccs_cycles, false,
         sizeof(BOOL) * aug_graph->components.length);

  for (i = 0; i < order->length; i++) {
    int comp_index = order->array[i];
    SCC_COMPONENT* comp = &aug_graph->components.array[comp_index];
    if (comp->length == 1) {
      buffer_array[buffer_count++] = comp_index;
    }

    if (comp->length > 1 || i + 1 == order->length) {
      if (buffer_count > 0) {
        // Combine all SCC that are non-circular (i.e. have a size of 1)
        SCC_COMPONENT* merged_scc =
            (SCC_COMPONENT*)malloc(sizeof(SCC_COMPONENT));
        merged_scc->array = (int*)malloc(sizeof(int) * buffer_count);
        merged_scc->length = 0;

        for (j = 0; j < buffer_count; j++) {
          SCC_COMPONENT* current_comp =
              &aug_graph->components.array[buffer_array[j]];

          merged_scc->array[merged_scc->length++] = current_comp->array[0];
        }

        buffer_count = 0;
        consolidated_sccs_cycles[count_consolidated_sccs] = false;
        consolidated_sccs->array[count_consolidated_sccs] = *merged_scc;
        count_consolidated_sccs++;
      }

      if (comp->length > 1) {
        consolidated_sccs_cycles[count_consolidated_sccs] = true;
        consolidated_sccs->array[count_consolidated_sccs] = *comp;
        count_consolidated_sccs++;

        topological_sort_component(aug_graph, comp);
      }
    }
  }

  consolidated_sccs->length = count_consolidated_sccs;
  aug_graph->consolidated_ordered_scc = *consolidated_sccs;
  aug_graph->consolidated_ordered_scc_cycle = consolidated_sccs_cycles;

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf("Consolidated Ordered Components of %s\n",
           decl_name(aug_graph->syntax_decl));
    for (i = 0; i < consolidated_sccs->length; i++) {
      SCC_COMPONENT* comp = &consolidated_sccs->array[i];
      printf(" Component #%d\n", i);

      for (j = 0; j < comp->length; j++) {
        printf("   ");
        print_instance(&aug_graph->instances.array[comp->array[j]], stdout);
        printf("\n");
      }
    }
    printf("\n");
  }
}

/**
 * @param s state
 */
void state_scc(STATE* s) {
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
        SCC_COMPONENT* comp = &aug_graph->components.array[j];
        printf(" Component #%d\n", j);

        for (k = 0; k < comp->length; k++) {
          printf("   ");
          print_instance(&aug_graph->instances.array[comp->array[k]], stdout);
          printf("\n");
        }
      }
      printf("\n");
    }

    analyze_sccs(aug_graph);
  }
}
