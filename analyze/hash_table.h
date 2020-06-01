//
//	hash_table.h
//
//	Defines the internal structures for a hash table, including
//	the items that consist of the key/value pairs, and the
//	table itself.
//
//	We also need to define the methods to query, delete, and insert
//	items into the table.
//

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

typedef struct {
    char *key;
    void *value;
} ht_item;

typedef struct {
    int size;
    int count;
    int base_size;
    ht_item **items;
} ht_hash_table;

ht_hash_table *ht_new();
void ht_insert(ht_hash_table *ht, const char *key, const void *value);
void *ht_search(ht_hash_table *ht, const char *key);
void ht_delete(ht_hash_table *ht, const char *key);

#endif
