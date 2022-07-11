#ifndef SCC_H
#define SCC_H

struct scc_component {
  int size;
  int* array;
};

typedef struct scc_component SCC_COMPONENT;

struct scc_components {
  int size;
  SCC_COMPONENT* array;
};

typedef struct scc_components SCC_COMPONENTS;

struct vertex {
  int value;
  struct vertex* next;
};

typedef struct vertex Vertex;

struct scc_graph {
  int num_vertices;
  Vertex** neighbors;
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
 * @brief Add edge method of graph
 * @param graph pointer to graph
 * @param source index of source
 * @param sink index of sink
 */
void scc_graph_add_edge(SccGraph* graph, int source, int sink);

/**
 * @brief Finds strongly connected components of a given graph
 * @param graph pointer to graph
 */
SCC_COMPONENTS* scc_graph_components(SccGraph* graph);

#endif
