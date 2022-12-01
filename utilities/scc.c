/**
 * Kosaraju's algorithm implementation which is a linear time algorithm
 * to find the strongly connected components of a directed graph.
 * https://en.wikipedia.org/wiki/kosaraju's_algorithm
 */

#include "scc.h"
#include <alloca.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "stack.h"

static int get_vertex_index(SccGraph* graph, uintptr_t v) {
  if (!hash_table_contains((const void*)v, graph->vertices_map)) {
    fatal_error("Failed to find vertex %d in the vertex map", v);
  }

  int index = VOIDP2INT(hash_table_get((const void*)v, graph->vertices_map));

  if (index < 0 || index >= graph->num_vertices) {
    fatal_error("Unexpected index %d was retrevied from the vertex map", index);
  }

  return index;
}

static long vertex_hash(const void* v) {
  return (long)v;
}

static bool vertex_equals(const void* v1, const void* v2) {
  return v1 == v2;
}

/**
 * @brief Create graph given number of vertices implemented using adjacency
 * @return pointer to allocated graph
 */
SccGraph* scc_graph_create(int num_vertices) {
  SccGraph* graph = (SccGraph*)malloc(sizeof(SccGraph));
  graph->num_vertices = num_vertices;
  size_t vertices_size = num_vertices * num_vertices * sizeof(bool);
  graph->adjacency = (bool*)malloc(vertices_size);
  memset(graph->adjacency, false, vertices_size);

  graph->vertices_map = (HASH_TABLE*)malloc(sizeof(HASH_TABLE));
  hash_table_initialize(num_vertices, vertex_hash, vertex_equals,
                        graph->vertices_map);

  return graph;
}

/**
 * @brief Add edge method of graph
 * @param graph pointer to graph
 * @param source index of source
 * @param sink index of sink
 */
static void scc_graph_add_edge_internal(SccGraph* graph, int source, int sink) {
  graph->adjacency[source * graph->num_vertices + sink] = true;
}

/**
 * @brief Add edge method of graph
 * @param graph pointer to graph
 * @param source index of source
 * @param sink index of sink
 */
void scc_graph_add_edge(SccGraph* graph, uintptr_t source, uintptr_t sink) {
  graph->adjacency[get_vertex_index(graph, source) * graph->num_vertices +
                   get_vertex_index(graph, sink)] = true;
}

/**
 * @brief Add vertex to the graph
 * @param graph pointer to graph
 * @param v pointer of vertex
 */
void scc_graph_add_vertex(SccGraph* graph, uintptr_t v) {
  hash_table_add_or_update((const void*)v, INT2VOIDP(graph->next_vertex_index),
                           graph->vertices_map);
  graph->next_vertex_index++;
}

/**
 * @brief Deallocate graph
 * @param graph pointer to graph
 */
void scc_graph_destroy(SccGraph* graph) {
  hash_table_clear(graph->vertices_map);

  free(graph->vertices_map);
  free(graph->adjacency);
  free(graph);
}

static bool contains_edge(SccGraph* graph, uintptr_t source, uintptr_t sink) {
  return graph->adjacency[source * graph->num_vertices + sink];
}

/**
 * @brief Returns the neighbors of given edge
 * @param graph pointer to graph
 */
static void get_neighbors(SccGraph* graph,
                          uintptr_t source,
                          int* neighbors,
                          int* count_neighbors) {
  int i;
  int n = graph->num_vertices;
  for (i = 0; i < n; i++) {
    if (contains_edge(graph, source, i)) {
      neighbors[*count_neighbors] = i;
      *count_neighbors = *count_neighbors + 1;
    }
  }
}

/**
 * @brief DFS traversal of graph
 * @param graph pointer to graph
 * @param stack pointer to stack
 * @param visited visited boolean array
 * @param v vertex
 */
static void dfs(SccGraph* graph,
                LinkedStack** stack,
                bool* visited,
                uintptr_t v) {
  int v_index = get_vertex_index(graph, v);
  visited[v_index] = true;
  int n = graph->num_vertices;
  int* neighbors = (int*)alloca(n * sizeof(int));
  int count_neighbors = 0;
  get_neighbors(graph, v, neighbors, &count_neighbors);

  int i;
  for (i = 0; i < count_neighbors; i++) {
    int neighbor = neighbors[i];
    if (!visited[neighbor]) {
      dfs(graph, stack, visited, neighbor);
    }
  }
  stack_push(stack, v);
}

/**
 * @brief Builds reverse of graph
 * @param graph pointer to graph
 * @return reversed graph
 */
static SccGraph* reverse(SccGraph* graph) {
  int n = graph->num_vertices;
  SccGraph* reversed_graph = scc_graph_create(n);

  int i, j;
  for (i = 0; i < graph->num_vertices; i++) {
    int* neighbors = (int*)alloca(n * sizeof(int));
    int count_neighbors = 0;
    get_neighbors(graph, i, neighbors, &count_neighbors);
    for (j = 0; j < count_neighbors; j++) {
      int neighbor = neighbors[j];
      scc_graph_add_edge(reversed_graph, neighbor, i);
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
  int n = graph->num_vertices;

  result_array[(*result_count)++] = v;
  visited[v] = true;
  deleted[v] = true;

  // the adjacent list of vertex v
  int* neighbors = (int*)alloca(n * sizeof(int));
  int count_neighbors = 0;
  get_neighbors(graph, v, neighbors, &count_neighbors);

  int i;
  for (i = 0; i < count_neighbors; i++) {
    int neighbor = neighbors[i];
    if (!visited[neighbor] && !deleted[neighbor]) {
      dfs_collect_scc(graph, visited, deleted, neighbor, result_array,
                      result_count);
    }
  }
}

/**
 * @brief Kosaraju logic
 * @param graph pointer to graph
 */
SCC_COMPONENTS scc_graph_components(SccGraph* graph) {
  if (graph == NULL || graph->num_vertices <= 0) {
    fatal_error("Graph parameter passed to Kosaraju method is not valid.");
  }

  int i, j, k;
  int n = graph->num_vertices;

  // Run transitive closure of the graph
  bool changed;
  do {
    changed = false;
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        for (k = 0; k < n; k++) {
          if (contains_edge(graph, i, j) && contains_edge(graph, j, k) &&
              !contains_edge(graph, i, k)) {
            scc_graph_add_edge(graph, i, k);
            changed = true;
            aps_warning(graph,
                        "Graph provided to SCC utility has not gone through "
                        "transitive closure");
          }
        }
      }
    }
  } while (changed);

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
    uintptr_t v;
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

  // Collect components as vector of integers
  SCC_COMPONENTS* result = (SCC_COMPONENTS*)malloc(sizeof(SCC_COMPONENTS));
  VECTORALLOC(*result, SCC_COMPONENT, num_components);

  for (i = 0; i < num_components; i++) {
    SCC_COMPONENT* comp = (SCC_COMPONENT*)malloc(sizeof(SCC_COMPONENT));
    VECTORALLOC(*comp, uintptr_t, components_count[i]);
    result->array[i] = *comp;
    for (j = 0; j < components_count[i]; j++) {
      comp->array[j] = components_array[i * n + j];
    }
  }

  // Free memory allocated via malloc
  scc_graph_destroy(reversed_graph);

  SCC_COMPONENTS re = *result;

  return re;
}