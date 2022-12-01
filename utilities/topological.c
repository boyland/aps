
/**
 * Topological Sort with linear time complexity using Stack and DFS traversal
 */

#include "topological.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "prime.h"
#include "stack.h"

// Color definition
#define WHITE 0  // never visited
#define GRAY 1   // itself is visited, but its neighbors are still under visit
#define BLACK 2  // both itself and all its neighbors are visited

static int get_vertex_index(TopologicalSortGraph* graph, uintptr_t v) {
  if (!hash_table_contains((const void*)v, graph->vertices_map)) {
    fatal_error("Failed to find vertex %d in the vertex map", v);
  }

  int index = VOIDP2INT(hash_table_get((const void*)v, graph->vertices_map));

  if (index < 0 || index >= graph->num_vertices) {
    fatal_error("Unexpected index %d was retrevied from the vertex map", index);
  }

  return index;
}

/**
 * Insert a new vertex to a linked list
 * @param adjacency reference to the head of adjacency linked list
 * @param vertex vertex to add to adjacency linked list
 * @return new head of adjacency linked list
 */
AdjacencyNode* insert_vertex(AdjacencyNode** adjacency, uintptr_t vertex) {
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
static void dfs(uintptr_t v,
                int* colors,
                TopologicalSortGraph* graph,
                LinkedStack** stack) {
  int v_index = get_vertex_index(graph, v);
  colors[v_index] = GRAY;
  AdjacencyNode* curr = graph->adjacencies[v_index];
  while (curr != NULL) {
    uintptr_t w = curr->vertex;
    int w_index = get_vertex_index(graph, curr->vertex);
    if (colors[w_index] == GRAY) {
      // Found a loop
      if (graph->ignore_cycles) {
        aps_warning(NULL,
                    "Cycle was expected! will continue to find the topological "
                    "order.\n");
      } else {
        fatal_error(
            "Did not expect cycle while topological sorting the graph.");
      }
    }
    if (colors[w_index] == WHITE) {
      dfs(w, colors, graph, stack);
    }
    curr = curr->next;
  }
  colors[v_index] = BLACK;
  stack_push(stack, v);
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

  hash_table_clear(graph->vertices_map);
  free(graph->vertices_map);

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
  VECTORALLOC(*order, uintptr_t, graph->num_vertices);

  for (i = 0; i < graph->num_vertices; i++) {
    stack_pop(&stack, &order->array[i]);
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
                               uintptr_t source,
                               uintptr_t sink) {
  graph->adjacencies[get_vertex_index(graph, source)] =
      insert_vertex(&graph->adjacencies[get_vertex_index(graph, source)], sink);
}

void topological_sort_add_vertex(TopologicalSortGraph* graph, uintptr_t v) {
  hash_table_add_or_update((const void*)v, INT2VOIDP(graph->next_vertex_index),
                           graph->vertices_map);

  graph->next_vertex_index++;
}

static long vertex_hash(const void* v) {
  return (long)v;
}

static bool vertex_equals(const void* v1, const void* v2) {
  return v1 == v2;
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

  graph->next_vertex_index = 0;
  graph->num_vertices = num_vertices;
  graph->ignore_cycles = ignore_cycles;

  size_t adjacencies_size = num_vertices * sizeof(AdjacencyNode*);
  graph->adjacencies = (AdjacencyNode**)malloc(adjacencies_size);

  graph->vertices_map = (HASH_TABLE*)malloc(sizeof(HASH_TABLE));
  hash_table_initialize(num_vertices, vertex_hash, vertex_equals,
                        graph->vertices_map);

  memset(graph->adjacencies, 0, adjacencies_size);

  return graph;
}
