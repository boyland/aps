#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>
#include <stddef.h>

typedef long (*Hash_Table_Hash)(const void*);
typedef bool (*Hash_Table_Equal)(const void*, const void*);

typedef struct hash_table_entry {
  const void* key;
  const void* value;
} HASH_TABLE_ENTRY;

typedef struct hash_table {
  Hash_Table_Hash hashf;
  Hash_Table_Equal equalf;
  int size;
  int capacity;
  HASH_TABLE_ENTRY* table;
} HASH_TABLE;

/**
 * Initialize a new hashtable
 * @param initial_capacity hashtable initial capacity
 * @param hashf hash function to hash the key
 * @param equalf equality function
 * @return new hashcons set that includes the item
 */
void hash_table_initialize(unsigned int initial_capacity,
                           Hash_Table_Hash hashf,
                           Hash_Table_Equal equalf,
                           HASH_TABLE* table);

/**
 * Adds an entry to the hashtable if not exists or updates the value if entry
 * exists
 * @param key hash entry key
 * @param value hash entry value
 * @param table hash table
 */
void hash_table_add_or_update(const void* key,
                              const void* value,
                              HASH_TABLE* table);

/**
 * Get hash entry value if there is one otherwise returns NULL
 * @param key key to lookup item
 * @param table hash table
 * @return the value of the hash entry given the key or NULL
 */
const void* hash_table_get(const void* key, HASH_TABLE* table);

/**
 * Removes hash entry if there is one otherwise returns NULL
 * @param key key to lookup item
 * @param table hash table
 * @return boolean indicating if updating the entry's value was successful or
 * not
 */
bool hash_table_remove(const void* key, HASH_TABLE* table);

/**
 * Clears the hashtable and removes all the elements
 * @param key key to lookup item
 * @param table hash table
 */
void hash_table_clear(HASH_TABLE* table);

/**
 * Test whether hash entry with value exists or not in the table
 * @param key key to lookup item
 * @param table hash table
 * @return boolean indicating whether entry with the value exists or not
 */
bool hash_table_contains(const void* key, HASH_TABLE* table);

#endif
