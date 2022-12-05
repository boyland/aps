#include "../scc.h"

#include <math.h>
#include <stdio.h>
#include "common.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static void test_all_disjoints() {
  SccGraph graph;
  scc_graph_initialize(&graph, TOTAL_COUNT);

  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_vertex(&graph, INT2VOIDP(i));
  }

  SCC_COMPONENTS* components = scc_graph_components(&graph);

  assert_true("should have many disjoint components",
              components->length == TOTAL_COUNT);

  for (i = 0; i < components->length; i++) {
    SCC_COMPONENT* component = components->array[i];

    assert_true("component should have one element in it", component->length);
  }

  scc_graph_destroy(&graph);
}

static void test_all_disjoints2() {
  SccGraph graph;
  scc_graph_initialize(&graph, TOTAL_COUNT);

  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_vertex(&graph, INT2VOIDP(i));
  }

  for (i = 1; i <= TOTAL_COUNT - 1; i++) {
    scc_graph_add_edge(&graph, INT2VOIDP(i), INT2VOIDP(i + 1));
  }

  SCC_COMPONENTS* components = scc_graph_components(&graph);

  assert_true("should have many disjoint components",
              components->length == TOTAL_COUNT);

  for (i = 0; i < components->length; i++) {
    SCC_COMPONENT* component = components->array[i];

    assert_true("component should have one element in it", component->length);
  }

  scc_graph_destroy(&graph);
}

static void test_all_connected() {
  SccGraph graph;
  scc_graph_initialize(&graph, TOTAL_COUNT);

  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_vertex(&graph, INT2VOIDP(i));
  }

  for (i = 1; i <= TOTAL_COUNT - 1; i++) {
    scc_graph_add_edge(&graph, INT2VOIDP(i), INT2VOIDP(i + 1));
  }

  scc_graph_add_edge(&graph, INT2VOIDP(TOTAL_COUNT), INT2VOIDP(1));

  SCC_COMPONENTS* components = scc_graph_components(&graph);

  assert_true("should have one large component", components->length == 1);

  SCC_COMPONENT* component = components->array[0];
  assert_true("component should have all elements in it",
              component->length == TOTAL_COUNT);

  scc_graph_destroy(&graph);
}

static void test_two_connected_components() {
  SccGraph graph;
  scc_graph_initialize(&graph, TOTAL_COUNT);
  int half_count = TOTAL_COUNT / 2;

  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_vertex(&graph, INT2VOIDP(i));
  }

  // Component 1
  for (i = 1; i <= half_count - 1; i++) {
    scc_graph_add_edge(&graph, INT2VOIDP(i), INT2VOIDP(i + 1));
  }

  scc_graph_add_edge(&graph, INT2VOIDP(half_count), INT2VOIDP(1));

  // Component 2
  for (i = half_count + 1; i <= TOTAL_COUNT - 1; i++) {
    scc_graph_add_edge(&graph, INT2VOIDP(i), INT2VOIDP(i + 1));
  }

  scc_graph_add_edge(&graph, INT2VOIDP(TOTAL_COUNT), INT2VOIDP(half_count + 1));

  SCC_COMPONENTS* components = scc_graph_components(&graph);

  assert_true("should have one large component", components->length == 2);

  SCC_COMPONENT* component1 = components->array[0];
  assert_true("component 1 should have half of elements in it",
              component1->length == half_count);

  SCC_COMPONENT* component2 = components->array[2];
  assert_true("component 2 should have other half of elements in it",
              component1->length == (TOTAL_COUNT - half_count));

  scc_graph_destroy(&graph);
}

static void test_n_connected_components() {
  SccGraph graph;
  scc_graph_initialize(&graph, TOTAL_COUNT);

  int i, j;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_vertex(&graph, INT2VOIDP(i));
  }

  int count_chunks = 7;
  int chunk_size = TOTAL_COUNT / count_chunks;

  int start = 0;
  int end;
  j = 0;
  do {
    end = MIN(TOTAL_COUNT, start + chunk_size);

    // Component j
    for (i = start; i <= end - 1; i++) {
      scc_graph_add_edge(&graph, INT2VOIDP(i), INT2VOIDP(i + 1));
    }

    scc_graph_add_edge(&graph, INT2VOIDP(end), INT2VOIDP(start));

    j++;
    start = end + 1;
  } while (end < TOTAL_COUNT);

  SCC_COMPONENTS* components = scc_graph_components(&graph);

  assert_true("should have n large component", components->length == j);

  for (j = 0; j < count_chunks; j++) {
    SCC_COMPONENT* component = components->array[j];
    assert_true(
        "component j should have at most CHUNK_SIZE number of elements in it",
        component->length <= ceil((1.0 * TOTAL_COUNT / count_chunks)));
  }

  scc_graph_destroy(&graph);
}

static void test_small_logical() {
  SccGraph graph;
  scc_graph_initialize(&graph, 5);

  scc_graph_add_vertex(&graph, INT2VOIDP(1));
  scc_graph_add_vertex(&graph, INT2VOIDP(2));
  scc_graph_add_vertex(&graph, INT2VOIDP(3));
  scc_graph_add_vertex(&graph, INT2VOIDP(4));
  scc_graph_add_vertex(&graph, INT2VOIDP(5));

  // Component 1
  scc_graph_add_edge(&graph, INT2VOIDP(1), INT2VOIDP(2));
  scc_graph_add_edge(&graph, INT2VOIDP(2), INT2VOIDP(1));

  // Component 2
  scc_graph_add_edge(&graph, INT2VOIDP(3), INT2VOIDP(4));
  scc_graph_add_edge(&graph, INT2VOIDP(4), INT2VOIDP(3));

  SCC_COMPONENTS* components = scc_graph_components(&graph);

  assert_true("should have 3 component", components->length == 3);

  int i;
  for (i = 0; i < components->length; i++) {
    SCC_COMPONENT* component = components->array[i];

    if (component->length > 1) {
      assert_true("Component should contain 2 vertices",
                  component->length == 2);
    } else {
      assert_true("Component should have vertex 5",
                  VOIDP2INT(component->array[0]) == 5);
    }
  }

  scc_graph_destroy(&graph);
}

void test_scc() {
  TEST tests[] = {
      {test_small_logical, "SCC with 2 connected components"},
      {test_all_disjoints, "SCC all disjoints"},
      {test_all_disjoints2, "SCC all disjoints with edges"},
      {test_all_connected, "SCC all connected"},
      {test_two_connected_components, "SCC two connected component"},
      {test_n_connected_components, "SCC n connected component"}};

  run_tests("scc", tests, 6);
}