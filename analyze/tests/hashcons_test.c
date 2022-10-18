#include "hashcons.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_COUNT 1e5
#define PRIME_MODULO 977

void assert_true(const char *message, bool b)
{
  if (!b)
  {
    fprintf(stderr, "Assertion failed: %s\n", message);
    exit(1);
  }
}

long hashcons_integer_hash(void *p)
{
  return *((long *)p);
}

bool hashcons_integer_equals(void *p1, void *p2)
{
  int *i1 = (int *)p1;
  int *i2 = (int *)p2;
  return *i1 == *i2;
}

static struct hash_cons_table integer_table = { &hashcons_integer_hash, &hashcons_integer_equals, 0, 0 };

int *new_hash_cons_integer(int n)
{
  return hash_cons_get(&n, sizeof(int), &integer_table);
}

/**
 * Validating consistency of hashcons by storing and retriving integers
 */
void test_integer_table_consistency()
{
  printf("Started <test_integer_table_consistency>\n");

  char buffer[256];
  int i, j;
  for (i = 0, j = 0; i < TOTAL_COUNT; ++i, j = (j + 1) % PRIME_MODULO)
  {
    sprintf(buffer, "integer for %d", i);
    assert_true(buffer, *new_hash_cons_integer(j) == j);
  }

  printf("Finished <test_integer_table_consistency>\n");
}

/**
 * Validating empty hashcons set is unique and has a size of zero
 */
void test_integer_set_empty_consistency()
{
  printf("Started <test_integer_set_empty_consistency>\n");

  HASH_CONS_SET set = get_hash_cons_empty_set();

  assert_true("empty set should have size of 0", set->num_elements == 0);
  assert_true("empty set should be unique", set == get_hash_cons_empty_set());

  printf("Finished <test_integer_set_empty_consistency>\n");  
}

/**
 * Validating add_hash_cons_set, making sure size is getting incremented and values are unique
 */
void test_integer_set_add_consistency()
{
  printf("Started <test_integer_set_add_consistency>\n");

  char buffer[256];
  HASH_CONS_SET set = get_hash_cons_empty_set();

  int i, j;
  for (i = 0, j = 0; i < TOTAL_COUNT; ++i, j = (j + 1) % PRIME_MODULO)
  {
    set = add_hash_cons_set((void *)j, set);

    sprintf(buffer, "size should never go pass %d but it is %d", PRIME_MODULO, set->num_elements);
    assert_true(buffer, set->num_elements <= PRIME_MODULO);

    sprintf(buffer, "size contain the element %d at correct index", j);
    assert_true(buffer, (int)set->elements[j] == j);
  }

  printf("Finished <test_integer_set_add_consistency>\n");  
}

/**
 * Validating union_hash_const_set, making sure size is correct and values are sorted without any duplicates
 */
void test_integer_set_union_consistency()
{
  printf("Started <test_integer_set_union_consistency>\n");

  int size = 1000;
  char buffer[256];
  HASH_CONS_SET set1 = get_hash_cons_empty_set();
  HASH_CONS_SET set2 = get_hash_cons_empty_set();

  int i;
  for (i = 0; i < (size >> 1); i++)
  {
    set1 = add_hash_cons_set((void *)i, set1);
  }

  for (i = 0; i < size; i++)
  {
    set2 = add_hash_cons_set((void *)i, set2);
  }

  HASH_CONS_SET result_set = union_hash_const_set(set1, set2);

  sprintf(buffer, "size contain the element %d at correct index", i);
  assert_true(buffer, result_set->num_elements == size);

  for (i = 0; i < PRIME_MODULO; i++)
  {
    sprintf(buffer, "size contain the element %d at correct index", i);
    assert_true(buffer, (int)result_set->elements[i] == i);
  }

  printf("Finished <test_integer_set_union_consistency>\n");  
}

/**
 * Validating new_hash_cons_set, making sure size is correct and values are sorted
 */
void test_integer_set_new_consistency()
{
  printf("Started <test_integer_set_new_consistency>\n");

  int size = 1000;
  char buffer[256];
  HASH_CONS_SET set1 = get_hash_cons_empty_set();
  HASH_CONS_SET set2 = get_hash_cons_empty_set();
  
  size_t struct_size = sizeof(struct hash_cons_set) + 2 * size * sizeof(void *); 
  HASH_CONS_SET set = (HASH_CONS_SET)alloca(struct_size);
  set->num_elements = size;

  int i;
  for (i = 0; i < size; i++)
  {
    set->elements[i] = (void *)size - i - 1;
  }

  HASH_CONS_SET sorted_set = new_hash_cons_set(set);
  assert_true("size should be correct", sorted_set->num_elements == size);

  for (i = 0; i < size; i++)
  {
    sprintf(buffer, "elements should be in sorted order %d != %d", (int)sorted_set->elements[i], i);
    assert_true(buffer, (int)sorted_set->elements[i] == i);
  }

  printf("Finished <test_integer_set_new_consistency>\n");  
}

typedef struct dummy {
  int key;
  char text[];
} *DUMMY;

long hashcons_dummy_hash(void *untyped)
{
  DUMMY item = (DUMMY) untyped;
  return hash_mix(item->key, hash_string(item->text));
}

bool hashcons_dummy_equals(void *untyped_a, void *untyped_b)
{
  if (untyped_a == NULL || untyped_b == NULL)
  {
    return false;
  }

  DUMMY item_a = (DUMMY) untyped_a;
  DUMMY item_b = (DUMMY) untyped_b;

  return item_a->key == item_b->key && !strcmp(item_a->text, item_b->text);
}

static struct hash_cons_table dummy_table = { &hashcons_dummy_hash, &hashcons_dummy_equals, 0, 0 };

DUMMY new_hash_cons_dummy(int key, char text[])
{
  size_t struct_size = sizeof(key) + (strlen(text) + 1) * sizeof(char);
  DUMMY dummy = alloca(struct_size);
  dummy->key = key;
  strcpy(dummy->text, text);

  return hash_cons_get(dummy, struct_size, &dummy_table);
}

/**
 * Validating hashcons works with a struct
 */
void test_dummy_table_consistency()
{
  printf("Started <test_dummy_table_consistency>\n");
 
  char buffer[256];
  int i, j;
  for (i = 0, j = 0; i < TOTAL_COUNT; ++i, j = (j + 1) % PRIME_MODULO)
  {
    sprintf(buffer, "dummy for %d", i);
    assert_true(buffer, new_hash_cons_dummy(j, buffer) == new_hash_cons_dummy(j, buffer));
  }

  printf("Finished <test_dummy_table_consistency>\n");
}

/**
 * Run tests in a sequence 
 */
void test_hash_cons()
{
  test_integer_table_consistency();
  test_dummy_table_consistency();

  test_integer_set_empty_consistency();
  test_integer_set_add_consistency();
  test_integer_set_union_consistency();
  test_integer_set_new_consistency();
}