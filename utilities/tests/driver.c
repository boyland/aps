#include <stdio.h>
#include "common.h"
#include "hashcons_test.h"
#include "hashtable_test.h"
#include "scc_test.h"
#include "stack_test.h"

int main() {
  printf("Test regression size: %d\n", TOTAL_COUNT);

  test_hash_cons();
  test_hash_table();
  test_stack();
  test_scc();

  printf("Finished running all tests successfully\n");
}