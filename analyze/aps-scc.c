#include <jbb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

void set_phylum_graph_components(PHY_GRAPH* phy_graph) {
  int i, j, k;
  int n = phy_graph->instances.length;
  SccGraph* graph;
  scc_graph_initialize(graph, n);
  for (i = 0; i < n; i++) {
    INSTANCE* source = &phy_graph->instances.array[i];
    for (j = 0; j < n; j++) {
      if (phy_graph->mingraph[i * n + j]) {
        INSTANCE* sink = &phy_graph->instances.array[j];

        scc_graph_add_edge(graph, (void*)source, (void*)sink);
      }
    }
  }

  phy_graph->components = scc_graph_components(graph);
  phy_graph->component_cycle =
      (bool*)calloc(sizeof(bool), phy_graph->components->length);

  for (i = 0; i < phy_graph->components->length; i++) {
    SCC_COMPONENT* comp = phy_graph->components->array[i];

    for (j = 0; j < comp->length; j++) {
      INSTANCE* in = (INSTANCE*)comp->array[j];

      if (phy_graph->mingraph[in->index * n + in->index]) {
        phy_graph->component_cycle[i] = true;
      }
    }
  }

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf("Components of Phylum Graph: %s\n", phy_graph_name(phy_graph));
    for (j = 0; j < phy_graph->components->length; j++) {
      SCC_COMPONENT* comp = phy_graph->components->array[j];
      printf(" Component #%d [%s]\n", j,
             phy_graph->component_cycle[j] ? "circular" : "non-circular");

      for (k = 0; k < comp->length; k++) {
        printf("   ");
        print_instance(comp->array[k], stdout);
        printf("\n");
      }
    }
    printf("\n");
  }
}

void set_aug_graph_components(AUG_GRAPH* aug_graph) {
  int i, j, k;
  int n = aug_graph->instances.length;
  SccGraph* graph;
  scc_graph_initialize(graph, n);
  for (i = 0; i < n; i++) {
    INSTANCE* source = &aug_graph->instances.array[i];
    for (j = 0; j < n; j++) {
      INSTANCE* sink = &aug_graph->instances.array[j];
      if (edgeset_kind(aug_graph->graph[i * n + j])) {
        if (!MERGED_CONDITION_IS_IMPOSSIBLE(instance_condition(source),
                                            instance_condition(sink))) {
          scc_graph_add_edge(graph, (void*)source, (void*)sink);
        }
      }
    }
  }

  aug_graph->components = scc_graph_components(graph);
  aug_graph->component_cycle =
      (bool*)calloc(sizeof(bool), aug_graph->components->length);

  for (i = 0; i < aug_graph->components->length; i++) {
    SCC_COMPONENT* comp = aug_graph->components->array[i];

    for (j = 0; j < comp->length; j++) {
      INSTANCE* source = (INSTANCE*)comp->array[j];

      for (k = 0; k < comp->length; k++) {
        INSTANCE* sink = (INSTANCE*)comp->array[k];
        if (edgeset_kind(aug_graph->graph[source->index * n + sink->index])) {
          aug_graph->component_cycle[i] = true;
        }
      }
    }
  }

  if ((oag_debug & DEBUG_ORDER) && (oag_debug & DEBUG_ORDER_VERBOSE)) {
    printf("Components of Augmented Graph: %s\n", aug_graph_name(aug_graph));
    for (j = 0; j < aug_graph->components->length; j++) {
      SCC_COMPONENT* comp = aug_graph->components->array[j];
      printf(" Component #%d [%s]\n", j,
             aug_graph->component_cycle[j] ? "circular" : "non-circular");

      for (k = 0; k < comp->length; k++) {
        printf("   ");
        print_instance((INSTANCE*)comp->array[k], stdout);
        printf("\n");
      }
    }
    printf("\n");
  }
}
