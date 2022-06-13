#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aps-ag.h"
#include "jbb-alloc.h"
#include "jbb.h"

// Color definition
// 0 -> NO COLOR
// 1 -> WHITE
// 2 -> GRAY
// 3 -> BALCK

#define NO_COLOR 0
#define WHITE 1
#define GRAY 2
#define BALCK 3

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
    while (cur->next) {
      cur = cur->next;
    }
    // add the new vertex to the end
    cur->next = new;
  } else {
    // When the linked list does not exist, create a new one
    *adjacency = new;
  }
  // Return the modified linked list
  return *adjacency;
}

// DFS Visit
static void dfs_visit(int i,
                      TopologicalSortGraph* graph,
                      int* color,
                      int* result) {
  // Keep track of the "time"
  static int cnt = 1;

  // Extract the linked list of vertex i
  AdjacencyNode* hnode = graph->adjacencies[i];

  // Update the color
  color[i] = WHITE;
  while (hnode) {
    // current operating vertex
    int cur = hnode->vertex;
    // Check if the edge is a back edge
    if (color[hnode->vertex] == WHITE) {
      fatal_error("This graph has a cycle!\n");
      return;
    }
    // Explore the white vertex
    if (color[hnode->vertex] == NO_COLOR) {
      // Recursion
      dfs_visit(hnode->vertex, graph, color, result);
    }
    // Proceed to the next node
    hnode = hnode->next;
  }
  // Update the color and the finish time
  color[i] = GRAY;
  result[cnt] = i;
  cnt += 1;
}

// DFS
static void dfs(TopologicalSortGraph* graph, int* result) {
  int i;
  int* color;
  // Allocate the menory
  color = (int*)alloca((graph->num_vertices + 1) * sizeof(int));
  // Initialize all the vertices
  for (i = 1; i < graph->num_vertices + 1; i++) {
    color[i] = NO_COLOR;
  }
  for (i = 1; i < graph->num_vertices + 1; i++) {
    // When the vertex is not explored
    if (color[i] == NO_COLOR) {
      // Visit this vertex
      dfs_visit(i, graph, color, result);
    }
  }
}

// Topological Sort
TOPOLOGICAL_SORT_ORDER* topological_sort_order(TopologicalSortGraph* graph) {
  int* result;
  if (graph == NULL) {
    fatal_error("invalid graph provided to topoligical sort.");
    return NULL;
  }
  // Allocate the memory
  result = (int*)alloca((graph->num_vertices + 1) * sizeof(int));
  // DFS on the graph
  dfs(graph, result);

  TOPOLOGICAL_SORT_ORDER* order =
      (TOPOLOGICAL_SORT_ORDER*)malloc(sizeof(TOPOLOGICAL_SORT_ORDER));
  VECTORALLOC(*order, int, graph->num_vertices);

  int i;
  for (i = graph->num_vertices; i >= 1; i--) {
    order->array[graph->num_vertices - i] = result[i] - 1;
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