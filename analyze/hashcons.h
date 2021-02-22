#ifndef HASHCONS_H
#define HASHCONS_H

#include <stdbool.h>
#include <stddef.h>

typedef int (*Hash_Cons_Hash)(void *);
typedef bool (*Hash_Cons_Equal)(void *, void *);

typedef struct hash_cons_table {
  Hash_Cons_Hash hashf;
  Hash_Cons_Equal equalf;
  int size;
  int capacity;
  void **table;
} * HASH_CONS_TABLE;

typedef struct hash_cons_set {
  int num_elements;
  void *elements[];
} * HASH_CONS_SET;

/**
 * Return the empty set
 * @return hashconsed empty set
 */
HASH_CONS_SET get_hash_cons_empty_set();

/**
 * Take a temporary set and hash cons it, returning the set that results
 * NOTE: The elements array will be sorted
 * by address to ensure a canonical representation.
 * @param set hashcons set
 * @return new hashcons set that includes the item
 */
HASH_CONS_SET new_hash_cons_set(HASH_CONS_SET set);

/**
 * Adds an element to the hashcons set, returning the set that results
 * @param item item to be added to the set
 * @param set hashcons set
 * @return new hashcons set that includes the item
 */
HASH_CONS_SET add_hash_cons_set(void *item, HASH_CONS_SET set);

/**
 * Unions two hashcons set, returning the set that results
 * @param set_a hashcons set A
 * @param set_b hashcons set B
 * @return new hashcons set that includes the item
 */
HASH_CONS_SET union_hash_const_set(HASH_CONS_SET set_a, HASH_CONS_SET set_b);

/**
 * Get item if there is one otherwise create one
 * @param temp_item it is a temporary or perhaps stack allocated creation of item
 * @param temp_size how many bytes it is
 * @param hashcons table
 */
void *hash_cons_get(void *temp_item, size_t temp_size, HASH_CONS_TABLE table);

/**
 * Hash string
 * @param string
 * @return intger hash value
 */
int hash_string(char *str);

/**
 * Combine two hash values into one
 * @param hash1
 * @param hash2
 * @return combined hash
 */
int hash_mix(int h1, int h2);

#endif