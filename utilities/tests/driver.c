#include <stdio.h>
#include "hashcons_test.h"
#include "hashtable_test.h"
#include "stack_test.h"

int main() {
  test_hash_cons();
  test_hash_table();
  test_stack();

  printf("Finished running all tests\n");
}