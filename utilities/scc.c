/**
 * Kosaraju's algorithm implementation which is a linear time algorithm
 * to find the strongly connected components of a directed graph.
 * https://en.wikipedia.org/wiki/kosaraju's_algorithm
 */

#include "scc.h"
#include <alloca.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "stack.h"

/**
 * Give a graph and vertex it returns the internal index
 * @param graph SCC graph
 * @param v vertex to lookup
 * @return int corresponding internal index of vertex
 */
static int get_vertex_index_from_ptr(SccGraph* graph, void* v) {
  if (!hash_table_contains(v, graph->vertices_ptr_to_index_map)) {
    fatal_error("Failed to find vertex ptr %d in the vertex map", v);
  }

  int index = VOIDP2INT(hash_table_get(v, graph->vertices_ptr_to_index_map));

  if (index < 0 || index >= graph->num_vertices) {
    fatal_error("Unexpected index %d was retrevied from the vertex map", index);
  }

  return index;
}

/**
 * Give a graph and internal index it returns the vertex
 * @param graph SCC graph
 * @param index internal index
 * @return int corresponding vertex
 */
static void* get_vertex_ptr_from_int(SccGraph* graph, int index) {
  if (!hash_table_contains(INT2VOIDP(index),
                           graph->vertices_index_to_ptr_map)) {
    fatal_error("Failed to find ptr of vertex with index %d in the vertex map",
                index);
  }

  void* ptr =
      hash_table_get(INT2VOIDP(index), graph->vertices_index_to_ptr_map);

  return ptr;
}

/**
 * @brief Create graph given number of vertices implemented using adjacency
 * @return pointer to allocated graph
 */
SccGraph* scc_graph_create(int num_vertices) {
  SccGraph* graph = malloc(sizeof(SccGraph));
  graph->num_vertices = num_vertices;
  size_t vertices_size = num_vertices * sizeof(Vertex*);
  graph->neighbors = (Vertex**)malloc(vertices_size);
  memset(graph->neighbors, 0, vertices_size);

  // Create a map to lookup from ptr to index
  graph->vertices_ptr_to_index_map = (HASH_TABLE*)malloc(sizeof(HASH_TABLE));
  hash_table_initialize(num_vertices, ptr_hashf, ptr_equalf,
                        graph->vertices_ptr_to_index_map);

  // Create a map to lookup from index to ptr
  graph->vertices_index_to_ptr_map = (HASH_TABLE*)malloc(sizeof(HASH_TABLE));
  hash_table_initialize(num_vertices, ptr_hashf, ptr_equalf,
                        graph->vertices_index_to_ptr_map);

  // Index of vertex starts from 0
  graph->next_vertex_index = 0;

  return graph;
}

/**
 * @brief Add edge method of graph to be used internally
 * @param graph pointer to graph
 * @param source index of source
 * @param sink index of sink
 */
static void scc_graph_add_edge_internal(SccGraph* graph, int source, int sink) {
  Vertex* item = (Vertex*)malloc(sizeof(Vertex));
  item->value = sink;
  item->next = graph->neighbors[source];
  graph->neighbors[source] = item;
}

/**
 * @brief Add edge method of graph
 * @param graph pointer to graph
 * @param source source ptr
 * @param sink sink ptr
 */
void scc_graph_add_edge(SccGraph* graph, void* source, void* sink) {
  int source_index = get_vertex_index_from_ptr(graph, source);
  int sink_index = get_vertex_index_from_ptr(graph, sink);

  scc_graph_add_edge_internal(graph, source_index, sink_index);
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

  hash_table_clear(graph->vertices_ptr_to_index_map);
  hash_table_clear(graph->vertices_index_to_ptr_map);

  free(graph->vertices_ptr_to_index_map);
  free(graph->vertices_index_to_ptr_map);

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
  stack_push(stack, INT2VOIDP(v));
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
      scc_graph_add_edge_internal(reversed_graph, neighbors->value, i);
      neighbors = neighbors->next;
    }
  }
  return reversed_graph;
}

/**
 * @brief Add vertex to the graph
 * @param graph pointer to graph
 * @param v pointer of vertex
 */
void scc_graph_add_vertex(SccGraph* graph, void* v) {
  if (graph->next_vertex_index >= graph->num_vertices) {
    fatal_error("Expected %d vertices to be added", graph->num_vertices);
    return;
  }

  hash_table_add_or_update(v, INT2VOIDP(graph->next_vertex_index),
                           graph->vertices_ptr_to_index_map);

  hash_table_add_or_update(INT2VOIDP(graph->next_vertex_index), v,
                           graph->vertices_index_to_ptr_map);

  graph->next_vertex_index++;
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
 * @brief Internal utility function that checks whether edge exists
 * @param graph pointer to graph
 * @param source index of source
 * @param sink index of sink
 * @return boolean indicating the edge between source and sink
 */
static bool contains_edge(SccGraph* graph, int source, int sink) {
  Vertex* sinks = graph->neighbors[source];
  while (sinks != NULL) {
    if (sinks->value == sink) {
      return true;
    }

    sinks = sinks->next;
  }

  return false;
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
            scc_graph_add_edge_internal(graph, i, k);
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
    void* temp;
    bool any = stack_pop(&stack, &temp);
    int v = VOIDP2INT(temp);
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
  VECTORALLOC(*result, SCC_COMPONENT, num_components);

  for (i = 0; i < num_components; i++) {
    SCC_COMPONENT* comp = (SCC_COMPONENT*)malloc(sizeof(SCC_COMPONENT));
    VECTORALLOC(*comp, void*, components_count[i]);
    result->array[i] = *comp;
    for (j = 0; j < components_count[i]; j++) {
      comp->array[j] =
          get_vertex_ptr_from_int(graph, components_array[i * n + j]);
    }
  }

  // Free memory allocated via malloc
  scc_graph_destroy(reversed_graph);

  SCC_COMPONENTS re = *result;

  return re;
}
