#ifndef _TOPOLOGICAL_SORT_H_
#define _TOPOLOGICAL_SORT_H_

#include <stdbool.h>
#include <stdint.h>
#include "hashtable.h"
#include "imports.r"

// Define the basic component in adjacent list
struct adjacency_node {
  uintptr_t vertex;             // The vertex number
  struct adjacency_node* next;  // vertices of nodes connected to this node
};

typedef struct adjacency_node AdjacencyNode;

typedef VECTOR(uintptr_t) TOPOLOGICAL_SORT_ORDER;

struct topological_sort_graph {
  AdjacencyNode**
      adjacencies;     // Adjacency linked list (indexed by vertex number)
  int num_vertices;    // number of vertices in the graph
  bool ignore_cycles;  // true if topological sorting algorithm ignores the
                       // cycles and returns the one of possibly many valid
                       // order, false if existence of cycle should cause a
                       // fatal error
  HASH_TABLE* vertices_map;  // Map of uintptr_t to int index
  int next_vertex_index;     // Index of the next vertex
};

typedef struct topological_sort_graph TopologicalSortGraph;

/**
 * Created the graph that will be used for topological sorting
 * @param num_vertices number of vertices of the graph
 * @param ignore_cycles true if topological sorting algorithm ignores the cycles
 * and returns the one of possibly many valid order, false if existence of cycle
 * should cause a fatal error
 * @return graph that will be used for topological sorting
 */
TopologicalSortGraph* topological_sort_graph_create(int num_vertices,
                                                    bool ignore_cycles);

/**
 * Given topological sort graph it adds an edge between two indices
 * @param graph the graph that is being topological sorted
 * @param source source index of edge
 * @param sink sink index of edge
 */
void topological_sort_add_edge(TopologicalSortGraph* graph,
                               uintptr_t source,
                               uintptr_t sink);

/**
 * Finds the topological sorted order
 * @param graph the graph that is being topological sorted
 * @return vector of indices (integer vector)
 */
TOPOLOGICAL_SORT_ORDER* topological_sort_order(TopologicalSortGraph* graph);

#endif
