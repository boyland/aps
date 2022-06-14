
/**
 * Topological Sort with linear time complexity using Stack and DFS traversal
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aps-ag.h"
#include "jbb-alloc.h"
#include "jbb.h"

// Color definition
#define WHITE 0  // never visited
#define GRAY 1   // itself is visited, but its neighbors are still under visit
#define BLACK 2  // both itself and all its neighbors are visited

// Insert a new vertex to a linked list
AdjacencyNode* insert_vertex(AdjacencyNode** adjacency, int vertex) {
  // The new vertex
  AdjacencyNode* new = (AdjacencyNode*)malloc(sizeof(AdjacencyNode));
  new->vertex = vertex;
  new->next = NULL;
  // Check if the linked list exist
  if (*adjacency != NULL) {
    // When the linked list exist, reach the end of it
    AdjacencyNode* cur = *adjacency;
    AdjacencyNode* prev = NULL;

    while (cur) {
      if (cur->vertex == vertex) {
        // Already added this child
        return *adjacency;
      }

      prev = cur;
      cur = cur->next;
    }
    // add the new vertex to the end
    prev->next = new;
  } else {
    // When the linked list does not exist, create a new one
    *adjacency = new;
  }
  // Return the modified linked list
  return *adjacency;
}

// DFS Visit
static void dfs(int v,
                int* colors,
                TopologicalSortGraph* graph,
                LinkedStack** stack) {
  colors[v] = GRAY;
  AdjacencyNode* curr = graph->adjacencies[v];
  while (curr != NULL) {
    int w = curr->vertex;
    if (colors[w] == GRAY) {
      // Found a loop
      fatal_error("Loop found! No topological order exist!");
      return;
    }
    if (colors[w] == WHITE) {
      dfs(w, colors, graph, stack);
    }
    curr = curr->next;
  }
  colors[v] = BLACK;
  stack_push(stack, v);
}

static void free_adjacency_node(AdjacencyNode* node) {
  if (node != NULL) {
    free_adjacency_node(node->next);
    free(node);
  }
}

TopologicalSortGraph* topological_sort_graph_destroy(
    TopologicalSortGraph* graph) {
  int i;
  for (i = 0; i < graph->num_vertices; i++) {
    AdjacencyNode* head = graph->adjacencies[i];
    free_adjacency_node(head);
  }

  free(graph);
}

// Topological Sort
TOPOLOGICAL_SORT_ORDER* topological_sort_order(TopologicalSortGraph* graph) {
  if (graph == NULL) {
    fatal_error("invalid graph provided to topological sort.");
    return NULL;
  }

  int i;
  int* colors = (int*)alloca(graph->num_vertices * sizeof(int));

  for (int i = 0; i < graph->num_vertices; i++) {
    colors[i] = WHITE;
  }

  LinkedStack* stack;
  stack_create(&stack);

  // DFS on the graph
  for (i = 0; i < graph->num_vertices; i++) {
    if (colors[i] == WHITE) {
      dfs(i, colors, graph, &stack);
    }
  }

  TOPOLOGICAL_SORT_ORDER* order =
      (TOPOLOGICAL_SORT_ORDER*)malloc(sizeof(TOPOLOGICAL_SORT_ORDER));
  VECTORALLOC(*order, int, graph->num_vertices);

  for (i = 0; i < graph->num_vertices; i++) {
    stack_pop(&stack, &order->array[i]);
  }

  return order;
}

void topological_sort_add_edge(TopologicalSortGraph* graph,
                               int source,
                               int sink) {
  graph->adjacencies[source] = insert_vertex(&graph->adjacencies[source], sink);
}

TopologicalSortGraph* topological_sort_graph_create(int num_vertices) {
  TopologicalSortGraph* graph =
      (TopologicalSortGraph*)malloc(sizeof(TopologicalSortGraph));

  graph->num_vertices = num_vertices;

  size_t adjacencies_size = num_vertices * sizeof(AdjacencyNode*);
  graph->adjacencies = (AdjacencyNode**)malloc(adjacencies_size);

  memset(graph->adjacencies, 0, adjacencies_size);

  return graph;
}
