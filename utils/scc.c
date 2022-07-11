/**
 * Kosaraju's algorithm implementation which is a linear time algorithm
 * to find the strongly connected components of a directed graph.
 * https://en.wikipedia.org/wiki/kosaraju's_algorithm
 */

#include "scc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"
#include "stdbool.h"

/**
 * @brief Create graph given number of vertices implemented using adjacency
 * @return pointer to allocated graph
 */
SccGraph* scc_graph_create(int num_vertices) {
  SccGraph* graph = (SccGraph*)malloc(sizeof(SccGraph));
  graph->num_vertices = num_vertices;
  size_t vertices_size = num_vertices * sizeof(Vertex*);
  graph->neighbors = (Vertex**)malloc(vertices_size);
  memset(graph->neighbors, 0, vertices_size);
  return graph;
}

/**
 * @brief Add edge method of graph
 * @param graph pointer to graph
 * @param source index of source
 * @param sink index of sink
 */
void scc_graph_add_edge(SccGraph* graph, int source, int sink) {
  Vertex* item = (Vertex*)malloc(sizeof(Vertex));
  item->value = sink;
  item->next = graph->neighbors[source];
  graph->neighbors[source] = item;
}

/**
 * @brief Deallocate graph vertex
 * @param vertex vertex linked list node
 */
static void graph_destroy_vertex(Vertex* vertex) {
  if (vertex == NULL)
    return;

  if (vertex->next != NULL)
    graph_destroy_vertex(vertex->next);

  free(vertex);
}

/**
 * @brief Deallocate graph
 * @param graph pointer to graph
 */
void scc_graph_destroy(SccGraph* graph) {
  int i;
  for (i = 0; i < graph->num_vertices; i++) {
    graph_destroy_vertex(graph->neighbors[i]);
  }

  free(graph);
}

/**
 * @brief DFS traversal of graph
 * @param graph pointer to graph
 * @param stack pointer to stack
 * @param visited visited boolean array
 * @param v vertex
 */
static void dfs(SccGraph* graph, LinkedStack** stack, bool* visited, int v) {
  visited[v] = true;
  Vertex* neighbors = graph->neighbors[v];
  while (neighbors != NULL) {
    if (!visited[neighbors->value]) {
      dfs(graph, stack, visited, neighbors->value);
    }
    neighbors = neighbors->next;
  }
  stack_push(stack, v);
}

/**
 * @brief Builds reverse of graph
 * @param graph pointer to graph
 * @return reversed graph
 */
static SccGraph* reverse(SccGraph* graph) {
  SccGraph* reversed_graph = scc_graph_create(graph->num_vertices);

  int i;
  Vertex* neighbors;
  for (i = 0; i < graph->num_vertices; i++) {
    neighbors = graph->neighbors[i];
    while (neighbors != NULL) {
      scc_graph_add_edge(reversed_graph, neighbors->value, i);
      neighbors = neighbors->next;
    }
  }
  return reversed_graph;
}

/**
 * @brief Use dfs to list a set of vertices dfs_and_print from a vertex v in
 * reversed graph
 * @param graph pointer to graph
 * @param visited boolean array indicating whether index has been visited or not
 * @param deleted boolean array indicating whether index has been popped or not
 * @param v vertex
 * @param result_array result int array
 * @param result_count result counter
 */
void dfs_collect_scc(SccGraph* graph,
                     bool* visited,
                     bool* deleted,
                     int v,
                     int* result_array,
                     int* result_count) {
  result_array[(*result_count)++] = v;
  visited[v] = true;
  deleted[v] = true;
  Vertex* arcs = graph->neighbors[v];  // the adjacent list of vertex v
  while (arcs != NULL) {
    int u = arcs->value;
    if (!visited[u] && !deleted[u]) {
      dfs_collect_scc(graph, visited, deleted, u, result_array, result_count);
    }
    arcs = arcs->next;
  }
}

/**
 * @brief Kosaraju logic
 * @param graph pointer to graph
 */
SCC_COMPONENTS* scc_graph_components(SccGraph* graph) {
  if (graph == NULL || graph->num_vertices <= 0) {
    perror("Graph parameter passed to Kosaraju method is not valid.");
    exit(1);
  }

  int i, j;
  int n = graph->num_vertices;

  LinkedStack* stack;
  stack_create(&stack);

  size_t visited_size = n * sizeof(bool);
  bool* visited = (bool*)alloca(visited_size);
  memset(visited, false, visited_size);
  for (i = 0; i < n; i++) {
    if (!visited[i]) {
      dfs(graph, &stack, visited, i);
    }
  }

  SccGraph* reversed_graph = reverse(graph);

  bool* deleted = (bool*)alloca(n * sizeof(bool));
  memset(deleted, false, n * sizeof(bool));

  // Integer array to hold on to the size of each component indexed by component
  // index
  int* components_count = (int*)alloca(n * sizeof(int));
  memset(components_count, 0, n * sizeof(int));

  // Integer pointer array to hold on to items in each component
  int* components_array = (int*)alloca(n * n * sizeof(int*));
  // Number of all component
  int num_components = 0;

  while (!stack_is_empty(&stack)) {
    int v;
    bool any = stack_pop(&stack, &v);
    if (any && !deleted[v]) {
      memset(visited, false,
             n * sizeof(bool));  // mark all vertices of reverse as not visited

      dfs_collect_scc(reversed_graph, visited, deleted, v,
                      &components_array[num_components * n],
                      &components_count[num_components]);

      num_components++;
    }
  }

  // Collect components as vector of vector of integers
  SCC_COMPONENTS* result = (SCC_COMPONENTS*)malloc(sizeof(SCC_COMPONENTS));
  result->size = num_components;
  result->array =
      (SCC_COMPONENT**)malloc(num_components * sizeof(SCC_COMPONENT*));

  for (i = 0; i < num_components; i++) {
    SCC_COMPONENT* comp = (SCC_COMPONENT*)malloc(sizeof(SCC_COMPONENT));
    comp->size = components_count[i];
    result->array[i] = comp;
    for (j = 0; j < components_count[i]; j++) {
      comp->array[j] = components_array[i * n + j];
    }
  }

  // Free memory allocated via malloc
  scc_graph_destroy(reversed_graph);

  return result;
}
