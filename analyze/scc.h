#ifndef SCC_H
#define SCC_H

typedef VECTOR(int) COMPONENT;

typedef VECTOR(COMPONENT) COMPONENTS;

struct vertex {
  int value;
  struct vertex* next;
};

typedef struct vertex Vertex;

struct graph {
  int num_vertices;
  Vertex** neighbors;
};

typedef struct graph Graph;

/**
 * @brief Create graph given number of vertices implemented using adjacency
 * @return pointer to allocated graph
 */
Graph* graph_create(int num_vertices);

/**
 * @brief Deallocate graph
 * @param graph pointer to graph
 */
void graph_destroy(Graph* graph);

/**
 * @brief Add edge method of graph
 * @param graph pointer to graph
 * @param source index of source
 * @param sink index of sink
 */
void graph_add_edge(Graph* graph, int source, int sink);

/**
 * @brief Finds strongly connected components of a given graph
 * @param graph pointer to graph
 */
COMPONENTS graph_scc(Graph* graph);

#endif
