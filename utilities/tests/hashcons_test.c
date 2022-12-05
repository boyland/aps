#include "../hashcons.h"
#include "../hashtable.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "assert.h"

#define PRIME_MODULO 977

static struct hash_cons_table integer_table = {&ptr_hashf, &ptr_equalf, 0, 0};

static int* new_hash_cons_integer(int n) {
  return hash_cons_get(&n, sizeof(int), &integer_table);
}

/**
 * Validating consistency of hashcons by storing and retrieving integers
 */
static void test_integer_table_consistency() {
  char buffer[256];
  int i, j;
  for (i = 0, j = 0; i < TOTAL_COUNT; ++i, j = (j + 1) % PRIME_MODULO) {
    sprintf(buffer, "integer for %d", i);
    _ASSERT_EXPR(buffer, *new_hash_cons_integer(j) == j);
  }
}

/**
 * Validating empty hashcons set is unique and has a size of zero
 */
static void test_integer_set_empty_consistency() {
  HASH_CONS_SET set = get_hash_cons_empty_set();

  _ASSERT_EXPR("empty set should have size of 0", set->num_elements == 0);
  _ASSERT_EXPR("empty set should be unique", set == get_hash_cons_empty_set());
}

/**
 * Validating add_hash_cons_set, making sure size is getting incremented and
 * values are unique
 */
static void test_integer_set_add_consistency() {
  char buffer[256];
  HASH_CONS_SET set = get_hash_cons_empty_set();

  long i, j;
  for (i = 0, j = 0; i < TOTAL_COUNT; ++i, j = (j + 1) % PRIME_MODULO) {
    set = add_hash_cons_set((long*)j, set);

    sprintf(buffer, "size should never go pass %d but it is %d", PRIME_MODULO,
            set->num_elements);
    _ASSERT_EXPR(buffer, set->num_elements <= PRIME_MODULO);

    sprintf(buffer, "size contain the element %ld at correct index", j);
    _ASSERT_EXPR(buffer, (long)set->elements[j] == j);
  }
}

/**
 * Validating union_hash_const_set, making sure size is correct and values are
 * sorted without any duplicates
 */
static void test_integer_set_union_consistency() {
  int size = 1000;
  char buffer[256];
  HASH_CONS_SET set1 = get_hash_cons_empty_set();
  HASH_CONS_SET set2 = get_hash_cons_empty_set();

  int i;
  for (i = 0; i < (size >> 1); i++) {
    set1 = add_hash_cons_set(INT2VOIDP(i), set1);
  }

  for (i = 0; i < size; i++) {
    set2 = add_hash_cons_set(INT2VOIDP(i), set2);
  }

  HASH_CONS_SET result_set = union_hash_const_set(set1, set2);

  sprintf(buffer, "size contain the element %d at correct index", i);
  _ASSERT_EXPR(buffer, result_set->num_elements == size);

  for (i = 0; i < PRIME_MODULO; i++) {
    sprintf(buffer, "size contain the element %d at correct index", i);
    _ASSERT_EXPR(buffer, VOIDP2INT(result_set->elements[i]) == i);
  }
}

/**
 * Validating new_hash_cons_set, making sure size is correct and values are
 * sorted
 */
static void test_integer_set_new_consistency() {
  int size = 1000;
  char buffer[256];
  HASH_CONS_SET set1 = get_hash_cons_empty_set();
  HASH_CONS_SET set2 = get_hash_cons_empty_set();

  size_t struct_size = sizeof(struct hash_cons_set) + 2 * size * sizeof(void*);
  HASH_CONS_SET set = (HASH_CONS_SET)alloca(struct_size);
  set->num_elements = size;

  int i;
  for (i = 0; i < size; i++) {
    set->elements[i] = INT2VOIDP(size - i - 1);
  }

  HASH_CONS_SET sorted_set = new_hash_cons_set(set);
  _ASSERT_EXPR("size should be correct", sorted_set->num_elements == size);

  for (i = 0; i < size; i++) {
    sprintf(buffer, "elements should be in sorted order %d != %d",
            VOIDP2INT(sorted_set->elements[i]), i);
    _ASSERT_EXPR(buffer, VOIDP2INT(sorted_set->elements[i]) == i);
  }
}

typedef struct dummy {
  int key;
  char text[];
} * DUMMY;

static long hashcons_dummy_hash(void* untyped) {
  DUMMY item = (DUMMY)untyped;
  return hash_mix(item->key, hash_string(item->text));
}

static bool hashcons_dummy_equals(void* untyped_a, void* untyped_b) {
  if (untyped_a == NULL || untyped_b == NULL) {
    return false;
  }

  DUMMY item_a = (DUMMY)untyped_a;
  DUMMY item_b = (DUMMY)untyped_b;

  return item_a->key == item_b->key && !strcmp(item_a->text, item_b->text);
}

static struct hash_cons_table dummy_table = {&hashcons_dummy_hash,
                                             &hashcons_dummy_equals, 0, 0};

static DUMMY new_hash_cons_dummy(int key, char text[]) {
  size_t struct_size = sizeof(key) + (strlen(text) + 1) * sizeof(char);
  DUMMY dummy = alloca(struct_size);
  dummy->key = key;
  strcpy(dummy->text, text);

  return hash_cons_get(dummy, struct_size, &dummy_table);
}

/**
 * Validating hashcons works with a struct
 */
static void test_dummy_table_consistency() {
  char buffer[256];
  int i, j;
  for (i = 0, j = 0; i < TOTAL_COUNT; ++i, j = (j + 1) % PRIME_MODULO) {
    sprintf(buffer, "dummy for %d", i);
    _ASSERT_EXPR(buffer, new_hash_cons_dummy(j, buffer) ==
                            new_hash_cons_dummy(j, buffer));
  }
}

/**
 * Run tests in a sequence
 */
void test_hash_cons() {
  TEST tests[] = {
      {test_integer_set_new_consistency, "hashcons new"},
      {test_integer_table_consistency, "int hashcons table consistency"},
      {test_dummy_table_consistency, "dummy struct hashcons table consistency"},
      {test_integer_set_empty_consistency, "empty hashcons table consistency"},
      {test_integer_set_add_consistency, "hashcons add"},
      {test_integer_set_union_consistency, "hashcons union"}};

  run_tests("hashcons", tests, 6);
}