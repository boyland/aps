#ifndef _TOPOLOGICAL_SORT_H_
#define _TOPOLOGICAL_SORT_H_

// Define the basic component in adjacent list
struct adjacency_node {
  int vertex;                   // The vertex number
  struct adjacency_node* next;  // Its child
};

typedef struct adjacency_node AdjacencyNode;

typedef VECTOR(int) TOPOLOGICAL_SORT_ORDER;

struct topological_sort_graph {
  AdjacencyNode** adjacencies;
  int num_vertices;
};

typedef struct topological_sort_graph TopologicalSortGraph;

TopologicalSortGraph* topological_sort_graph_create(int num_vertices);

void topological_sort_add_edge(TopologicalSortGraph* graph,
                               int source,
                               int sink);

TOPOLOGICAL_SORT_ORDER* topological_sort_order(TopologicalSortGraph* graph);

#endif
