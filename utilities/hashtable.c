#include "hashtable.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imports.r"
#include "prime.h"

#define MAX_DENSITY 0.5
#define DOUBLE_SIZE(x) (((x) << 1) + 1)

/**
 * Finds the candidate index intended to get inserted or searched in hash table
 * @param item the item looking to be added or removed
 * @param table hash table
 * @return candidate index to insert or search for hash entry
 */
static int hash_table_candidate_index(void* key, HASH_TABLE* table) {
  long hash = table->hashf(key) & LONG_MAX;
  int index = hash % table->capacity;
  int step_size = hash % (table->capacity - 2) + 1;

  while (true) {
    if (table->table[index].key == NULL ||
        table->equalf(table->table[index].key, key)) {
      return index;
    }

    index = (index + step_size) % table->capacity;
  }
}

/**
 * Insert an item into table
 * @param key key to insert at given index
 * @param value value to insert at given index
 * @param index index to insert the hash entry
 * @param item the item intended to get inserted into the table
 */
static void hash_table_insert_at(void* key,
                                 void* value,
                                 const int index,
                                 HASH_TABLE* table) {
  table->table[index].key = key;
  table->table[index].value = value;
  table->size++;
}

/**
 * Searches for hash enrty index in the table
 * @param item the key of the hash entry
 * @return possible index of the hash entry
 */
static int hash_table_search(void* key, HASH_TABLE* table) {
  int index = hash_table_candidate_index(key, table);

  return index;
}

/**
 * Resizes the hash table given new larger capacity
 * @param capacity new capacity
 * @param table hash table
 */
static void hash_table_resize(const int capacity, HASH_TABLE* table) {
  HASH_TABLE_ENTRY* old_table = table->table;
  int old_capacity = table->capacity;
  hash_table_initialize(capacity, table->hashf, table->equalf, table);

  for (int i = 0; i < old_capacity; i++) {
    HASH_TABLE_ENTRY item = old_table[i];
    if (item.key != NULL) {
      hash_table_add_or_update(item.key, item.value, table);
    }
  }

  free(old_table);
}

/**
 * Get hash entry value if there is one otherwise returns NULL
 * @param key key to lookup item
 * @param table hash table
 * @return the value of the hash entry given the key or NULL
 */
void* hash_table_get(void* key, HASH_TABLE* table) {
  int candidate_index = hash_table_search(key, table);

  if (table->equalf(table->table[candidate_index].key, key)) {
    return table->table[candidate_index].value;
  }

  return NULL;
}

/**
 * Adds an entry to the hashtable if not exists or updates the value if entry
 * exists
 * @param key hash entry key
 * @param value hash entry value
 * @param table hash table
 */
void hash_table_add_or_update(void* key, void* value, HASH_TABLE* table) {
  if (key == NULL) {
    fprintf(stdout, "NULL key is not allowed in hashtable.\n");
    exit(1);
    return;
  }

  if (table->size + 1 > table->capacity * MAX_DENSITY) {
    const int new_capacity = next_twin_prime(DOUBLE_SIZE(table->capacity));
    hash_table_resize(new_capacity, table);
  }

  int candidate_index = hash_table_candidate_index(key, table);

  if (table->equalf(table->table[candidate_index].key, key)) {
    table->table[candidate_index].value = value;
    return;
  }

  hash_table_insert_at(key, value, candidate_index, table);
}

/**
 * Initialize a new hashtable
 * @param initial_capacity hashtable initial capacity
 * @param hashf hash function to hash the key
 * @param equalf equality function
 */
void hash_table_initialize(unsigned int initial_capacity,
                           Hash_Table_Hash hashf,
                           Hash_Table_Equal equalf,
                           HASH_TABLE* table) {
  table->capacity = next_twin_prime(initial_capacity);
  table->hashf = hashf;
  table->equalf = equalf;
  table->size = 0;
  table->table =
      (HASH_TABLE_ENTRY*)calloc(table->capacity, sizeof(HASH_TABLE_ENTRY));
}

/**
 * Removes hash entry if there is one otherwise returns NULL
 * @param key key to lookup item
 * @param table hash table
 * @return boolean indicating if updating the entry's value was successful or
 * not
 */
bool hash_table_remove(void* key, HASH_TABLE* table) {
  int candidate_index = hash_table_search(key, table);

  if (table->equalf(table->table[candidate_index].key, key)) {
    table->table[candidate_index].key = NULL;
    table->size--;
    return true;
  }

  return false;
}

/**
 * Clears the hashtable and removes all the elements
 * @param key key to lookup item
 * @param table hash table
 */
void hash_table_clear(HASH_TABLE* table) {
  table->capacity = 0;
  table->size = 0;
  free(table->table);
}

/**
 * Test whether hash entry with value exists or not in the table
 * @param key key to lookup item
 * @param table hash table
 * @return boolean indicating whether entry with the value exists or not
 */
bool hash_table_contains(void* key, HASH_TABLE* table) {
  int candidate_index = hash_table_search(key, table);

  if (table->equalf(table->table[candidate_index].key, key)) {
    return true;
  }

  return false;
}

/**
 * Generic function that maps void* ptr to hash value
 * @param v void* ptr
 * @return address of void* ptr used as hash value
 */
long ptr_hashf(void* v) {
  return (long)v;
}

/**
 * Generic function that creates void* equality
 * @param v1 void* ptr1
 * @param v1 void* ptr2
 * @return boolean indicating whether two ptrs are equal
 */
bool ptr_equalf(void* v1, void* v2) {
  return v1 == v2;
}
