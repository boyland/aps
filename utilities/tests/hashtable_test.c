#include "../utilities.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_COUNT 1e5

/**
 * Validating consistency of hashtable by storing and retriving integers
 */
void test_hash_table_consistency() {
  printf("Started <test_hash_table_consistency>\n");

  HASH_TABLE table;
  hash_table_initialize(10, ptr_hashf, ptr_equalf, &table);

  // Ensure size is initially zero
  assert(table.size == 0);

  // Add items to the hashtable and ensure they exist
  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should be first time seeing this key
    assert(!hash_table_contains(INT2VOIDP(i), &table));

    // Add entry (i,i+1) to the hash table
    hash_table_add_or_update(INT2VOIDP(i), INT2VOIDP(i + 1), &table);

    // Ensure hash table now holds the (i,i+1)
    assert(hash_table_contains(INT2VOIDP(i), &table));
    assert(VOIDP2INT(hash_table_get(INT2VOIDP(i), &table)) == i + 1);
  }

  // Size should be TOTAL_COUNT
  assert(table.size == TOTAL_COUNT);

  // Update hash table value and make sure its updates
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should not be first time seeing this key
    assert(hash_table_contains(INT2VOIDP(i), &table));

    // Update entry (i,i+2) in the hash table
    hash_table_add_or_update(INT2VOIDP(i), INT2VOIDP(i + 2), &table);

    // Ensure hash table now holds the (i,i+2)
    assert(hash_table_contains(INT2VOIDP(i), &table));
    assert(VOIDP2INT(hash_table_get(INT2VOIDP(i), &table)) == i + 2);
  }

  // Size should still be TOTAL_COUNT
  assert(table.size == TOTAL_COUNT);

  // Remove hash table entry and make sure its updates
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should not be first time seeing this key
    assert(hash_table_contains(INT2VOIDP(i), &table));

    // Remove entry with key i from the hash table
    assert(hash_table_remove(INT2VOIDP(i), &table));

    // Ensure hash table now holds the (i, i+1)
    assert(!hash_table_contains(INT2VOIDP(i), &table));
    assert(table.size == TOTAL_COUNT - i);
  }

  // Size should be 0
  assert(table.size == 0);

  // Ensure other items do not exist in the hash table
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Ensure hash table doesn't hold -i key
    assert(!hash_table_contains(INT2VOIDP(-i), &table));

    // Add entry (i,i+3) to the hash table
    hash_table_add_or_update(INT2VOIDP(i), INT2VOIDP(i + 3), &table);
  }

  hash_table_clear(&table);

  // Ensure size is cleared to zero
  assert(table.size == 0);

  printf("Finished <test_hash_table_consistency>\n");
}

/**
 * Run tests in a sequence
 */
void test_hash_table() {
  test_hash_table_consistency();
}