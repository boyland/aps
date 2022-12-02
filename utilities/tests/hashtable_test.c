#include "../hashtable.h"

#include "common.h"

/**
 * Validating consistency of hashtable by storing and retrieving integers
 */
void test_hash_table_consistency() {
  HASH_TABLE table;
  hash_table_initialize(10, ptr_hashf, ptr_equalf, &table);

  // Ensure size is initially zero
  assert_true("size of table should initially be zero", table.size == 0);

  // Add items to the hashtable and ensure they exist
  int i;
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should be first time seeing this key
    assert_true("hashtable should not initially contain the number",
                !hash_table_contains(INT2VOIDP(i), &table));

    // Add entry (i,i+1) to the hash table
    hash_table_add_or_update(INT2VOIDP(i), INT2VOIDP(i + 1), &table);

    // Ensure hash table now holds the (i,i+1)
    assert_true("hashtable should contain the number after adding",
                hash_table_contains(INT2VOIDP(i), &table));
    assert_true("hashtable value should be as expected",
                VOIDP2INT(hash_table_get(INT2VOIDP(i), &table)) == i + 1);
  }

  // Size should be TOTAL_COUNT
  assert_true("should table size should be > 0 and expected number",
              table.size == TOTAL_COUNT);

  // Update hash table value and make sure its updates
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should not be first time seeing this key
    assert_true("hashtable should contain the number already added",
                hash_table_contains(INT2VOIDP(i), &table));

    // Update entry (i,i+2) in the hash table
    hash_table_add_or_update(INT2VOIDP(i), INT2VOIDP(i + 2), &table);

    // Ensure hash table now holds the (i,i+2)
    assert_true("after update entry given key should still be in table",
                hash_table_contains(INT2VOIDP(i), &table));
    assert_true("updated value should be in table",
                VOIDP2INT(hash_table_get(INT2VOIDP(i), &table)) == i + 2);
  }

  // Size should still be TOTAL_COUNT
  assert_true("hashtable size should still be unchanged after update",
              table.size == TOTAL_COUNT);

  // Remove hash table entry and make sure its updates
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Should not be first time seeing this key
    assert_true(
        "contains should return true for element expected to be in the table",
        hash_table_contains(INT2VOIDP(i), &table));

    // Remove entry with key i from the hash table
    assert_true(
        "remove should return true because table should have contained this "
        "element",
        hash_table_remove(INT2VOIDP(i), &table));

    // Remove -i key from the hash table
    assert_true(
        "remove should return false because table should not have contained "
        "this "
        "element",
        !hash_table_remove(INT2VOIDP(-i), &table));

    // Ensure hash table now holds the (i, i+1)
    assert_true("hashtable should not contain element that was just removed",
                !hash_table_contains(INT2VOIDP(i), &table));
    assert_true("hashtable size should be correct after removing an element",
                table.size == TOTAL_COUNT - i);
  }

  // Size should be 0
  assert_true(
      "hashtable size should be zero after removing elements one by one",
      table.size == 0);

  // Ensure other items do not exist in the hash table
  for (i = 1; i <= TOTAL_COUNT; i++) {
    // Ensure hash table doesn't hold -i key
    assert_true("hashtable should not contain value that has not been added",
                !hash_table_contains(INT2VOIDP(-i), &table));
  }

  hash_table_clear(&table);

  // Ensure size is cleared to zero
  assert_true("hash table should be empty after clear", table.size == 0);
}

/**
 * Run tests in a sequence
 */
void test_hash_table() {
  TEST tests[] = {{test_hash_table_consistency, "hashtable consistency"}};

  run_tests("hashtable", tests, 1);
}
