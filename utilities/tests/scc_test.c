#include "../scc.h"

#include "common.h"

static void test_all_disjoints() {
  SccGraph* graph = scc_graph_create(TOTAL_COUNT);

  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_vertex(graph, INT2VOIDP(i));
  }

  SCC_COMPONENTS* components = scc_graph_components(graph);

  assert_true("should have many disjoint components",
              components->length == TOTAL_COUNT);

  for (i = 0; i < components->length; i++) {
    SCC_COMPONENT* component = components->array[i];

    assert_true("component should have one element in it", component->length);
  }
}

static void test_all_disjoints2() {
  SccGraph* graph = scc_graph_create(TOTAL_COUNT);

  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_vertex(graph, INT2VOIDP(i));
  }

  for (i = 1; i <= TOTAL_COUNT - 1; i++) {
    scc_graph_add_edge(graph, INT2VOIDP(i), INT2VOIDP(i + 1));
  }

  SCC_COMPONENTS* components = scc_graph_components(graph);

  assert_true("should have many disjoint components",
              components->length == TOTAL_COUNT);

  for (i = 0; i < components->length; i++) {
    SCC_COMPONENT* component = components->array[i];

    assert_true("component should have one element in it", component->length);
  }
}

static void test_all_connected() {
  SccGraph* graph = scc_graph_create(TOTAL_COUNT);

  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_vertex(graph, INT2VOIDP(i));
  }

  for (i = 0; i < TOTAL_COUNT - 1; i++) {
    scc_graph_add_edge(graph, INT2VOIDP(i), INT2VOIDP(i + 1));
  }

  scc_graph_add_edge(graph, INT2VOIDP(TOTAL_COUNT), INT2VOIDP(1));

  SCC_COMPONENTS* components = scc_graph_components(graph);

  assert_true("should have one large component", components->length == 1);

  SCC_COMPONENT* component = components->array[0];
  assert_true("component should have all elements in it",
              component->length == TOTAL_COUNT);
}

static void test_two_connected_components() {
  SccGraph* graph = scc_graph_create(TOTAL_COUNT);
  int half_count = TOTAL_COUNT / 2;

  int i;
  for (i = 0; i < TOTAL_COUNT; i++) {
    scc_graph_add_vertex(graph, INT2VOIDP(i + 1));
  }

  // Component 1
  for (i = 1; i <= half_count - 1; i++) {
    scc_graph_add_edge(graph, INT2VOIDP(i), INT2VOIDP(i + 1));
  }

  scc_graph_add_edge(graph, INT2VOIDP(half_count), INT2VOIDP(1));

  // Component 2
  for (i = half_count + 1; i <= TOTAL_COUNT; i++) {
    scc_graph_add_edge(graph, INT2VOIDP(i), INT2VOIDP(i + 1));
  }

  scc_graph_add_edge(graph, INT2VOIDP(TOTAL_COUNT), INT2VOIDP(half_count + 1));

  SCC_COMPONENTS* components = scc_graph_components(graph);

  assert_true("should have one large component", components->length == 2);

  SCC_COMPONENT* component1 = components->array[0];
  assert_true("component 1 should have half of elements in it",
              component1->length == half_count);

  SCC_COMPONENT* component2 = components->array[2];
  assert_true("component 2 should have other half of elements in it",
              component1->length == (TOTAL_COUNT - half_count));
}

void test_scc() {
  TEST tests[] = {
      {test_all_disjoints, "SCC all disjoints"},
      {test_all_disjoints2, "SCC all disjoints with edges"},
      {test_all_connected, "SCC all connected"},
      {test_two_connected_components, "SCC two connected component"}};

  run_tests("scc", tests, 4);
}