#include "../stack.h"
#include <stdio.h>
#include "../hashtable.h"
#include "assert.h"

static void test_empty() {
  printf("Started <test_empty>\n");

  LinkedStack* stack;
  stack_create(&stack);

  void* temp;
  assert(("Stack should be empty", !stack_pop(&stack, &temp)));

  printf("Started <test_empty>\n");
}

static void test_push_pop() {
  printf("Started <test_push_pop>\n");

  LinkedStack* stack;
  stack_create(&stack);

  int n = 1000;
  int i;
  for (i = 0; i < n; i++) {
    void* temp;
    stack_push(&stack, INT2VOIDP(n - i - 1));
  }

  for (i = 0; i < n; i++) {
    void* temp;
    assert(("Stack should be empty", stack_pop(&stack, &temp)));
    assert(("Should contain the right valud", VOIDP2INT(temp) == i));
  }

  printf("Started <test_push_pop>\n");
}

static void test_clear() {
  printf("Started <test_clear>\n");

  LinkedStack* stack;
  stack_create(&stack);

  int n = 1000;
  int i;
  for (i = 0; i < n; i++) {
    void* temp;
    stack_push(&stack, INT2VOIDP(n - i - 1));
  }

  stack_destroy(&stack);
  void* temp;
  assert(("Stack should be empty", !stack_pop(&stack, &temp)));

  printf("Started <test_clear>\n");
}

void test_stack() {
  test_empty();
  test_push_pop();
  test_clear();
}
