#include "hashcons.h"

#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"

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

void assert_true(const char *message, bool b)
{
  if (!b)
  {
    fprintf(stderr, "Assertion failed: %s\n", message);
    exit(1);
  }
}

void test_integer_table()
{
  int *i3 = new_hash_cons_integer(3);
  assert_true("initial i3", *i3 == 3);
  int *i8 = new_hash_cons_integer(8);
  assert_true("initial i8", *i8 == 8);
  assert_true("later i3", *i3 == 3);

  for (int i = 0; i < 100; ++i)
  {
    char buffer[256];
    sprintf(buffer, "integer for %d", i);
    assert_true(buffer, *new_hash_cons_integer(i) == i);
  }
}