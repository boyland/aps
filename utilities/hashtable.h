#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INT2VOIDP(i) (void*)(uintptr_t)(i)
#define VOIDP2INT(i) (int)(uintptr_t)(i)

typedef long (*Hash_Table_Hash)(void*);
typedef bool (*Hash_Table_Equal)(void*, void*);

typedef struct hash_table_entry {
  void* key;
  void* value;
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
void hash_table_initialize(HASH_TABLE* table,
                           unsigned int initial_capacity,
                           Hash_Table_Hash hashf,
                           Hash_Table_Equal equalf);

/**
 * Adds an entry to the hashtable if not exists or updates the value if entry
 * exists
 * @param key hash entry key
 * @param value hash entry value
 * @param table hash table
 */
void hash_table_add_or_update(HASH_TABLE* table, void* key, void* value);

/**
 * Get hash entry value if there is one otherwise returns NULL
 * @param key key to lookup item
 * @param table hash table
 * @return the value of the hash entry given the key or NULL
 */
void* hash_table_get(HASH_TABLE* table, void* key);

/**
 * Removes hash entry if there is one otherwise returns NULL
 * @param key key to lookup item
 * @param table hash table
 * @return boolean indicating if updating the entry's value was successful or
 * not
 */
bool hash_table_remove(HASH_TABLE* table, void* key);

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
bool hash_table_contains(HASH_TABLE* table, void* key);

/**
 * Generic function that maps void* ptr to hash value
 * @param v void* ptr
 * @return address of void* ptr used as hash value
 */
long ptr_hashf(void* v);

/**
 * Generic function that creates void* equality
 * @param v1 void* ptr1
 * @param v1 void* ptr2
 * @return boolean indicating whether two ptrs are equal
 */
bool ptr_equalf(void* v1, void* v2);

#endif
