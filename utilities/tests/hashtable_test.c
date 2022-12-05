#include "../hashtable.h"

#include "common.h"
#include "assert.h"

/**
 * Validating consistency of hashtable by storing and retrieving integers
 */
void test_hash_table_consistency() {
  HASH_TABLE table;
  hash_table_initialize(&table, 10, ptr_hashf, ptr_equalf);

  // Ensure size is initially zero
  _ASSERT_EXPR("size of table should initially be zero", table.size == 0);

  // Add items to the hashtable and ensure they exist
  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should be first time seeing this key
    _ASSERT_EXPR("hashtable should not initially contain the number",
                !hash_table_contains(&table, INT2VOIDP(i)));

    // Add entry (i,i+1) to the hash table
    hash_table_add_or_update(&table, INT2VOIDP(i), INT2VOIDP(i + 1));

    // Ensure hash table now holds the (i,i+1)
    _ASSERT_EXPR("hashtable should contain the number after adding",
                hash_table_contains(&table, INT2VOIDP(i)));
    _ASSERT_EXPR("hashtable value should be as expected",
                VOIDP2INT(hash_table_get(&table, INT2VOIDP(i))) == i + 1);
  }

  // Size should be TOTAL_COUNT
  _ASSERT_EXPR("should table size should be > 0 and expected number",
              table.size == TOTAL_COUNT);

  // Update hash table value and make sure its updates
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should not be first time seeing this key
    _ASSERT_EXPR("hashtable should contain the number already added",
                hash_table_contains(&table, INT2VOIDP(i)));

    // Update entry (i,i+2) in the hash table
    hash_table_add_or_update(&table, INT2VOIDP(i), INT2VOIDP(i + 2));

    // Ensure hash table now holds the (i,i+2)
    _ASSERT_EXPR("after update entry given key should still be in table",
                hash_table_contains(&table, INT2VOIDP(i)));
    _ASSERT_EXPR("updated value should be in table",
                VOIDP2INT(hash_table_get(&table, INT2VOIDP(i))) == i + 2);
  }

  // Size should still be TOTAL_COUNT
  _ASSERT_EXPR("hashtable size should still be unchanged after update",
              table.size == TOTAL_COUNT);

  // Remove hash table entry and make sure its updates
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should not be first time seeing this key
    _ASSERT_EXPR(
        "contains should return true for element expected to be in the table",
        hash_table_contains(&table, INT2VOIDP(i)));

    // Remove entry with key i from the hash table
    _ASSERT_EXPR(
        "remove should return true because table should have contained this "
        "element",
        hash_table_remove(&table, INT2VOIDP(i)));

    // Remove -i key from the hash table
    _ASSERT_EXPR(
        "remove should return false because table should not have contained "
        "this "
        "element",
        !hash_table_remove(&table, INT2VOIDP(-i)));

    // Ensure hash table now holds the (i, i+1)
    _ASSERT_EXPR("hashtable should not contain element that was just removed",
                !hash_table_contains(&table, INT2VOIDP(i)));
    _ASSERT_EXPR("hashtable size should be correct after removing an element",
                table.size == TOTAL_COUNT - i);
  }

  // Size should be 0
  _ASSERT_EXPR(
      "hashtable size should be zero after removing elements one by one",
      table.size == 0);

  // Ensure other items do not exist in the hash table
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Ensure hash table doesn't hold -i key
    _ASSERT_EXPR("hashtable should not contain value that has not been added",
                !hash_table_contains(&table, INT2VOIDP(-i)));
  }

  hash_table_clear(&table);

  // Ensure size is cleared to zero
  _ASSERT_EXPR("hash table should be empty after clear", table.size == 0);
}

/**
 * Run tests in a sequence
 */
void test_hash_table() {
  TEST tests[] = {{test_hash_table_consistency, "hashtable consistency"}};

  run_tests("hashtable", tests, 1);
}
