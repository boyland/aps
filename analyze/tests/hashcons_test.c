#include "hashcons.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void assert_true(const char *message, bool b)
{
  if (!b)
  {
    fprintf(stderr, "Assertion failed: %s\n", message);
    exit(1);
  }
}

/*
 * Basic Consistency Test
 * 
 * Making sure everything is getting correctly stored in the hashcons
 * table by validating the values that are retrieved are correct
 */

int hashcons_integer_hash(void *p)
{
  return *((int *)p);
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

void test_integer_table_consistency()
{
  for (int i = 0; i < 100; ++i)
  {
    char buffer[256];
    sprintf(buffer, "integer for %d", i);
    assert_true(buffer, *new_hash_cons_integer(i) == i);
  }
}

/*
 * Struct Consistency Test
 * 
 * Making sure everything is getting correctly stored in the hashcons
 * table by validating the values that are retrieved are correct
 */
typedef struct dummy {
  int key;
  char text[];
} *DUMMY;

int hashcons_dummy_hash(void *untyped)
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

DUMMY new_hash_cons_dummy(int key, const char* text)
{
  DUMMY dummy = alloca(sizeof(dummy));
  dummy->key = key;
  strcpy(dummy->text, text);
  size_t struct_size = sizeof(key) + sizeof(text) / sizeof(char);

  return hash_cons_get(dummy, struct_size, &dummy_table);
}

void test_dummy_table_consistency()
{
  for (int i = 0; i < 100; ++i)
  {
    char buffer[256];
    sprintf(buffer, "dummy for %d", i);
    assert_true(buffer, new_hash_cons_dummy(i, buffer) == new_hash_cons_dummy(i, buffer));
  }
}

/**
 * Run tests in a sequence 
 */
void test_hash_cons()
{
  test_integer_table_consistency();
  test_dummy_table_consistency();
}