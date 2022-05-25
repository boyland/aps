/**
 * Kosaraju's algorithm implementation which is a linear time algorithm
 * to find the strongly connected components of a directed graph.
 * https://en.wikipedia.org/wiki/kosaraju's_algorithm
 */

#include <jbb.h>
#include <stdio.h>
#include <string.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

struct stack {
  int value;
  struct stack* next;
};

typedef struct stack Stack;

/**
 * @brief Create stack using endogenous linked list
 * @param stack pointer to a stack pointer
 */
static void stack_create(Stack** stack) {
  *stack = NULL;
}

/**
 * @brief Push method of stack
 * @param stack pointer to a stack pointer
 * @param value value to push to stack
 */
static void stack_push(Stack** stack, int value) {
  Stack* item = malloc(sizeof(Stack));
  item->value = value;
  item->next = *stack;
  *stack = item;
}

/**
 * @brief Pop method of stack
 * @param stack pointer to a stack pointer
 * @param value that has just been popped from the stack
 * @return boolean indicating whether popping from the stack was successful or
 * not
 */
static bool stack_pop(Stack** stack, int* v) {
  Stack* old = *stack;
  if (old == NULL)
    return false;

  *v = old->value;
  *stack = old->next;
  free(old);
  return true;
}

/**
 * @brief Checks whether stack is empty or not
 * @param stack pointer to a stack pointer
 * @return boolean indicating whether stack is empty or not
 */
static bool stack_is_empty(Stack** stack) {
  return *stack == NULL;
}

/**
 * @brief Create graph given number of vertices implemented using adjacency
 * @return pointer to allocated graph
 */
Graph* graph_create(int num_vertices) {
  Graph* graph = malloc(sizeof(Graph));
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
void graph_add_edge(Graph* graph, int source, int sink) {
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
void graph_destroy(Graph* graph) {
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
static void dfs(Graph* graph, Stack** stack, bool* visited, int v) {
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
static Graph* reverse(Graph* graph) {
  Graph* reversed_graph = graph_create(graph->num_vertices);

  int i;
  Vertex* neighbors;
  for (i = 0; i < graph->num_vertices; i++) {
    neighbors = graph->neighbors[i];
    while (neighbors != NULL) {
      graph_add_edge(reversed_graph, neighbors->value, i);
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
void dfs_collect_scc(Graph* graph,
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
COMPONENTS graph_scc(Graph* graph) {
  if (graph == NULL || graph->num_vertices <= 0) {
    fatal_error("Graph parameter passed to Kosaraju method is not valid.");
  }

  int i, j;
  int n = graph->num_vertices;

  Stack* stack;
  stack_create(&stack);

  size_t visited_size = n * sizeof(bool);
  bool* visited = (bool*)alloca(visited_size);
  memset(visited, false, visited_size);
  for (i = 0; i < n; i++) {
    if (!visited[i]) {
      dfs(graph, &stack, visited, i);
    }
  }

  Graph* reversed_graph = reverse(graph);

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
  COMPONENTS result;
  VECTORALLOC(result, COMPONENT, num_components);

  for (i = 0; i < num_components; i++) {
    COMPONENT comp;
    VECTORALLOC(comp, int, components_count[i]);
    result.array[i] = comp;
    for (j = 0; j < components_count[i]; j++) {
      comp.array[j] = components_array[i * n + j];
    }
  }

  // Free memory allocated via malloc
  graph_destroy(reversed_graph);

  return result;
}
