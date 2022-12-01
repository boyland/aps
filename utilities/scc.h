#ifndef SCC_H
#define SCC_H

#include <stdbool.h>
#include <stdint.h>
#include "hashtable.h"
#include "imports.r"

typedef VECTOR(uintptr_t) SCC_COMPONENT;

typedef VECTOR(SCC_COMPONENT) SCC_COMPONENTS;

struct scc_graph {
  int num_vertices;
  bool* adjacency;
  HASH_TABLE* vertices_map;  // Map of uintptr_t to int index
  int next_vertex_index;     // Index of the next vertex
};

typedef struct scc_graph SccGraph;

/**
 * @brief Create graph given number of vertices implemented using adjacency
 * @return pointer to allocated graph
 */
SccGraph* scc_graph_create(int num_vertices);

/**
 * @brief Deallocate graph
 * @param graph pointer to graph
 */
void scc_graph_destroy(SccGraph* graph);

/**
 * @brief Add vertex to the graph
 * @param graph pointer to graph
 * @param v pointer of vertex
 */
void scc_graph_add_vertex(SccGraph* graph, uintptr_t v);

/**
 * @brief Add edge method of graph
 * @param graph pointer to graph
 * @param source pointer of source
 * @param sink pointer of sink
 */
void scc_graph_add_edge(SccGraph* graph, uintptr_t source, uintptr_t sink);

/**
 * @brief Finds strongly connected components of a given graph
 * @param graph pointer to graph
 */
SCC_COMPONENTS scc_graph_components(SccGraph* graph);

#endif
