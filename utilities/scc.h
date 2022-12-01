#ifndef SCC_H
#define SCC_H

#include <stdbool.h>
#include <stdint.h>
#include "hashtable.h"
#include "imports.r"

typedef VECTOR(void*) SCC_COMPONENT;

typedef VECTOR(SCC_COMPONENT) SCC_COMPONENTS;

// Vertex linked list node
struct vertex {
  int value;
  struct vertex* next;
};

typedef struct vertex Vertex;

struct scc_graph {
  int num_vertices;       // Number of vertices in the graph
  Vertex** neighbors;     // adjacency list of the edges

  HASH_TABLE* vertices_ptr_to_index_map;  // Map of void* to int index
  HASH_TABLE* vertices_index_to_ptr_map;  // Map of int index to void*

  int next_vertex_index;  // Index of the next vertex
};

typedef struct scc_graph SccGraph;

/**
 * @brief Create graph given number of vertices implemented using adjacency
 * @return ptr to allocated graph
 */
SccGraph* scc_graph_create(int num_vertices);

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
SCC_COMPONENTS scc_graph_components(SccGraph* graph);

#endif
