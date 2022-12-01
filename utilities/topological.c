
/**
 * Topological Sort with linear time complexity using Stack and DFS traversal
 */

#include "topological.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

// Color definition
#define WHITE 0  // never visited
#define GRAY 1   // itself is visited, but its neighbors are still under visit
#define BLACK 2  // both itself and all its neighbors are visited

static int get_vertex_index_from_ptr(TopologicalSortGraph* graph, void* v) {
  if (!hash_table_contains(v, graph->vertices_ptr_to_index_map)) {
    fatal_error("Failed to find vertex ptr %d in the vertex map", v);
  }

  int index = VOIDP2INT(hash_table_get(v, graph->vertices_ptr_to_index_map));

  if (index < 0 || index >= graph->num_vertices) {
    fatal_error("Unexpected index %d was retrevied from the vertex map", index);
  }

  return index;
}

static void* get_vertex_ptr_from_int(TopologicalSortGraph* graph, int v) {
  if (!hash_table_contains(INT2VOIDP(v), graph->vertices_ptr_to_index_map)) {
    fatal_error("Failed to find ptr of vertex with index %d in the vertex map",
                v);
  }

  void* ptr = hash_table_get(INT2VOIDP(v), graph->vertices_ptr_to_index_map);

  return ptr;
}

/**
 * Insert a new vertex to a linked list
 * @param adjacency reference to the head of adjacency linked list
 * @param vertex vertex to add to adjacency linked list
 * @return new head of adjacency linked list
 */
AdjacencyNode* insert_vertex(AdjacencyNode** adjacency, int vertex) {
  // The new vertex
  AdjacencyNode* new = (AdjacencyNode*)malloc(sizeof(AdjacencyNode));
  new->vertex = vertex;
  new->next = NULL;
  // Check if the linked list exist
  if (*adjacency != NULL) {
    // When the linked list exist, reach the end of it
    AdjacencyNode* cur = *adjacency;
    AdjacencyNode* prev = NULL;

    while (cur != NULL) {
      if (cur->vertex == vertex) {
        // Already added this child
        return *adjacency;
      }

      prev = cur;
      cur = cur->next;
    }
    // add the new vertex to the end
    prev->next = new;
  } else {
    // When the linked list does not exist, create a new one
    *adjacency = new;
  }
  // Return the modified linked list
  return *adjacency;
}

/**
 * DFS Visit
 * @param v vertext currently being investigated
 * @param graph the graph currently being topological sorted
 * @param stack reference to the head of stack
 */
static void dfs(int v,
                int* colors,
                TopologicalSortGraph* graph,
                LinkedStack** stack) {
  colors[v] = GRAY;
  AdjacencyNode* curr = graph->adjacencies[v];
  while (curr != NULL) {
    int w = curr->vertex;
    if (colors[w] == GRAY) {
      // Found a loop
      if (graph->ignore_cycles) {
        aps_warning(NULL,
                    "Cycle was expected! will continue to find the topological "
                    "order.");
      } else {
        fatal_error(
            "Did not expect cycle while topological sorting the graph.");
      }
    }
    if (colors[w] == WHITE) {
      dfs(w, colors, graph, stack);
    }
    curr = curr->next;
  }
  colors[v] = BLACK;
  stack_push(stack, INT2VOIDP(v));
}

/**
 * De-allocate the adjacency node
 * @param adjacency reference to the head of adjacency linked list
 */
static void free_adjacency_node(AdjacencyNode* node) {
  if (node != NULL) {
    free_adjacency_node(node->next);
    free(node);
  }
}

/**
 * De-allocate topological sorted graph
 * @param graph the graph that was topological sorted
 */
void topological_sort_graph_destroy(TopologicalSortGraph* graph) {
  int i;
  for (i = 0; i < graph->num_vertices; i++) {
    AdjacencyNode* head = graph->adjacencies[i];
    free_adjacency_node(head);
  }

  hash_table_clear(graph->vertices_ptr_to_index_map);
  hash_table_clear(graph->vertices_index_to_ptr_map);

  free(graph->vertices_ptr_to_index_map);
  free(graph->vertices_index_to_ptr_map);

  free(graph);
}

/**
 * Finds the topological sorted order
 * @param graph the graph that is being topological sorted
 * @return vector of indices (integer vector)
 */
TOPOLOGICAL_SORT_ORDER* topological_sort_order(TopologicalSortGraph* graph) {
  if (graph == NULL) {
    fatal_error("invalid graph provided to topological sort.");
    return NULL;
  }

  int i;
  int* colors = (int*)alloca(graph->num_vertices * sizeof(int));

  for (int i = 0; i < graph->num_vertices; i++) {
    colors[i] = WHITE;
  }

  LinkedStack* stack;
  stack_create(&stack);

  // DFS on the graph
  for (i = 0; i < graph->num_vertices; i++) {
    if (colors[i] == WHITE) {
      dfs(i, colors, graph, &stack);
    }
  }

  TOPOLOGICAL_SORT_ORDER* order =
      (TOPOLOGICAL_SORT_ORDER*)malloc(sizeof(TOPOLOGICAL_SORT_ORDER));
  VECTORALLOC(*order, void*, graph->num_vertices);

  for (i = 0; i < graph->num_vertices; i++) {
    void* temp;
    stack_pop(&stack, &temp);
    order->array[i] = get_vertex_ptr_from_int(graph, VOIDP2INT(temp));
  }

  return order;
}

/**
 * Given topological sort graph it adds an edge between two indices
 * @param graph the graph that is being topological sorted
 * @param source source index of edge
 * @param sink sink index of edge
 */
void topological_sort_add_edge(TopologicalSortGraph* graph,
                               void* source,
                               void* sink) {
  int source_index = get_vertex_index_from_ptr(graph, source);
  int sink_index = get_vertex_index_from_ptr(graph, sink);

  graph->adjacencies[source_index] =
      insert_vertex(&graph->adjacencies[source_index], sink_index);
}

/**
 * Given topological sort graph it adds a vertex
 * @param graph the graph that is being topological sorted
 * @param v vertex
 */
void topological_sort_add_vertex(TopologicalSortGraph* graph, void* v) {
  if (graph->next_vertex_index >= graph->num_vertices) {
    fatal_error("Expected %d vertices to be added", graph->num_vertices);
    return;
  }

  // ptr -> index
  hash_table_add_or_update(v, INT2VOIDP(graph->next_vertex_index),
                           graph->vertices_ptr_to_index_map);

  // index -> ptr
  hash_table_add_or_update(INT2VOIDP(graph->next_vertex_index), v,
                           graph->vertices_index_to_ptr_map);

  graph->next_vertex_index++;
}

/**
 * Created the graph that will be used for topological sorting
 * @param num_vertices number of vertices of the graph
 * @param ignore_cycles true if topological sorting algorithm ignores the cycles
 * and returns the one of possibly many valid order, false if existence of cycle
 * should cause a fatal error
 * @return graph that will be used for topological sorting
 */
TopologicalSortGraph* topological_sort_graph_create(int num_vertices,
                                                    bool ignore_cycles) {
  TopologicalSortGraph* graph =
      (TopologicalSortGraph*)malloc(sizeof(TopologicalSortGraph));

  graph->num_vertices = num_vertices;
  graph->ignore_cycles = ignore_cycles;

  size_t adjacencies_size = num_vertices * sizeof(AdjacencyNode*);
  graph->adjacencies = (AdjacencyNode**)malloc(adjacencies_size);

  memset(graph->adjacencies, 0, adjacencies_size);

  // Create a map to lookup from ptr to index
  graph->vertices_ptr_to_index_map = (HASH_TABLE*)malloc(sizeof(HASH_TABLE));
  hash_table_initialize(num_vertices, ptr_hashf, ptr_equalf,
                        graph->vertices_ptr_to_index_map);

  // Create a map to lookup from index to ptr
  graph->vertices_index_to_ptr_map = (HASH_TABLE*)malloc(sizeof(HASH_TABLE));
  hash_table_initialize(num_vertices, ptr_hashf, ptr_equalf,
                        graph->vertices_index_to_ptr_map);

  graph->next_vertex_index = 0;

  return graph;
}
