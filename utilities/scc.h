#ifndef SCC_H
#define SCC_H

#include <stdbool.h>
#include <stdint.h>
#include "hashtable.h"

typedef struct scc_component {
  void** array;
  int length;
} SCC_COMPONENT;

typedef struct scc_components {
  SCC_COMPONENT** array;
  int length;
} SCC_COMPONENTS;

// Vertex linked list node
struct vertex {
  int value;
  struct vertex* next;
};

typedef struct vertex Vertex;

struct scc_graph {
  int num_vertices;        // Number of vertices in the graph
  bool* adjacency_matrix;  // adjacency list of the edges

  HASH_TABLE* vertices_ptr_to_index_map;  // Map of void* to int index
  void** vertices_index_to_ptr_map;       // Map of int index to void*

  int next_vertex_index;  // Index of the next vertex
  Vertex** neighbors;     // O(1) way of getting neighbors
};

typedef struct scc_graph SccGraph;

/**
 * @brief Create graph given number of vertices implemented using adjacency
 * @return ptr to allocated graph
 */
void scc_graph_initialize(SccGraph* graph, int num_vertices);

/**
 * @brief Deallocate graph
 * @param graph ptr to graph
 */
void scc_graph_destroy(SccGraph* graph);

/**
 * @brief Add vertex to the graph
 * @param graph ptr to SCC graph
 * @param v vertex to be added to the graph
 */
void scc_graph_add_vertex(SccGraph* graph, void* v);

/**
 * @brief Add edge method of graph
 * @param graph ptr to SCC graph
 * @param source source of the edge
 * @param sink sink of the edge
 */
void scc_graph_add_edge(SccGraph* graph, void* source, void* sink);

/**
 * @brief Finds strongly connected components of a given graph
 * @param graph ptr to SCC graph
 */
SCC_COMPONENTS* scc_graph_components(SccGraph* graph);

#endif
