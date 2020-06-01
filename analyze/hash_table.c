//
//	hash_table.c
//
//	Defines the necessary methods to create and process
//	inputs/queries/etc. to a hash table.
//
//  This is a open-addressed double-hashed data structure
//

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hash_table.h"
#include <stdio.h>

#define HT_INITIAL_BASE_SIZE 50
#define INCREASE_TABLE_LIMIT 70
#define DECREASE_TABLE_LIMIT 10

int HT_PRIME_1 = 163;
int HT_PRIME_2 = 199;

static ht_item HT_DELETED_ITEM = {NULL, NULL};

/**
 * Creates a hash_table based on the default size
 */
static ht_hash_table *ht_new_sized(const int base_size) {
    ht_hash_table *ht = malloc(sizeof(ht_hash_table));
    ht->base_size = base_size;

    ht->size = next_prime(ht->base_size);
    ht->count = 0;
    ht->items = calloc((size_t)ht->size, sizeof(ht_item *));
    return ht;
}

/**
 * Resizes the table by creating a temporary hash table for values to go off of.
 */
static void ht_resize(ht_hash_table *ht, const int base_size) {
    if (base_size < HT_INITIAL_BASE_SIZE) {
        return;
    }
    ht_hash_table *new_ht = ht_new_sized(base_size);
    for (int i = 0; i < ht->size; i++) {
        ht_item *item = ht->items[i];
        if (item != NULL && item != &HT_DELETED_ITEM) {
            ht_insert(new_ht, item->key, item->value);
        }
    }

    //	Reassigns the base size and count of the current table
    //	to the new ht's pointer's values.
    ht->base_size = new_ht->base_size;
    ht->count = new_ht->count;

    //	To delete new_ht, we give it ht's size and items.
    const int tmp_size = ht->size;
    ht->size = new_ht->size;
    new_ht->size = tmp_size;

    ht_item **tmp_items = ht->items;
    ht->items = new_ht->items;
    new_ht->items = tmp_items;

    ht_del_hash_table(new_ht);
}

/**
 * Increases the table size based on the "base size" by a factor of 2 + 1
 */
static void ht_resize_up(ht_hash_table *ht) {
    const int new_size = (ht->base_size << 1) + 1;
    ht_resize(ht, new_size);
}

/**
 * Decreases the table size based on the "base size" by a factor of 2.
 */
static void ht_resize_down(ht_hash_table *ht) {
    const int new_size = ht->base_size >> 1;
    ht_resize(ht, new_size);
}

/**
 * Defines a new hashtable item to insert into the table
 */
static ht_item *ht_new_item(const char *k, const void *v) {
    ht_item *i = malloc(sizeof(ht_item));
    i->key = strdup(k);
    i->value = strdup(v);
    return i;
}

/**
 * Defines a new hashtable pointer object with a default size of 50.
 */
ht_hash_table *ht_new() { return ht_new_sized(HT_INITIAL_BASE_SIZE); }

/**
 *  Releases ht_item values from memory, then frees the pointer.
 */
static void ht_del_item(ht_item *i) {
    free(i->key);
    free(i->value);
    free(i);
}

/**
 * Iterates through the hashtable to find any non-null indices, deletes them, then frees the
 * hashtable pointer.
 */
void ht_del_hash_table(ht_hash_table *ht) {
    for (int i = 0; i < ht->size; i++) {
        ht_item *item = ht->items[i];
        if (item != NULL) {
            ht_del_item(item);
        }
    }

    free(ht->items);
    free(ht);
}

/*
 * Returns whether x is prime or not.
 * 1 if prime
 * 0 if not prime
 * -1 if undefined.
 */
int is_prime(const int x) {
    if (x < 2) {
        return -1;
    }
    if (x < 4) {
        return 1;
    }
    if ((x % 2) == 0) {
        return 0;
    }

    for (int i = 3; i <= floor(sqrt((double)x)); i += 2) {
        if ((x % i) == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * Returns next possible prime
 */
int next_prime(int x) {
    while (is_prime(x) != 1) {
        x++;
    }

    return x;
}

/**
 * Creates a [hopefully] uniformly-distributed hash value for our input item by converting the index
 * to a very large integer, then normalizing it to the size of our table.
 */
static int ht_hash(const char *s, const int prime, const int size) {
    long hash = 0;
    const int len_s = strlen(s);

    for (int i = 0; i < len_s; i++) {
        hash += pow(prime, (len_s - (i + 1))) * ((int)s[i]);
        hash = hash % size;
    }
    return hash;
}

/**
 * By using open addressing, we make the use of two hash functions to resolve any collisions that
 * may occur. If we don't run into any collisions, our attempt will remain 0, and the hash_a value
 * is used. If a collision occurs, we will modify the hash value by hash_b.
 *
 */
static int ht_get_hash(const char *s, const int num_buckets, const int attempt) {
    const int hash_a = ht_hash(s, HT_PRIME_1, num_buckets);
    const int hash_b = ht_hash(s, HT_PRIME_2, num_buckets);
    return (hash_a + (attempt * (hash_b + 1))) % num_buckets;
}

/**
 * Inserts a key/value pair into the hash table.
 */
void ht_insert(ht_hash_table *ht, const char *key, const void *value) {
    const int load = ht->count * 100 / ht->size;
    if (load > INCREASE_TABLE_LIMIT) {
        ht_resize_up(ht);
    }

    ht_item *item = ht_new_item(key, value);
    int index = ht_get_hash(item->key, ht->size, 0);
    ht_item *curItem = ht->items[index];

    int hash_attempt = 1;
    while (curItem != NULL) {
        if (curItem != &HT_DELETED_ITEM) {
            if (strcmp(curItem->key, key) == 0) {
                ht_del_item(curItem);
                ht->items[index] = item;
            }
        }

        index = ht_get_hash(item->key, ht->size, hash_attempt++);
        curItem = ht->items[index];
    }

    ht->items[index] = item;
    ht->count++;
}

/**
 * Searches through the hash table for the value of the corresponding key. If nothing is found, NULL
 * is returned.
 */
void *ht_search(ht_hash_table *ht, const char *key) {
    int index = ht_get_hash(key, ht->size, 0);
    ht_item *item = ht->items[index];

    int hash_attempt = 1;

    while (item != NULL) {
        if (item != &HT_DELETED_ITEM) {

            // If we find the item, we need to return it's value.
            if (strcmp(item->key, key) == 0) {
                return item->value;
            }
        }
        index = ht_get_hash(item->key, ht->size, hash_attempt++);
        item = ht->items[index];
    }

    return NULL;
}

/**
 * Deletes a key/value pair from the hash table. Because deletions from a hash table using open
 * addressing can cause an entire chain to corrupt, we simply denote the spot that has the key (if
 * it exists) with HT_DELETED_ITEM.
 */
void ht_delete(ht_hash_table *ht, const char *key) {
    const int load = ht->count * 100 / ht->size;
    if (load > DECREASE_TABLE_LIMIT) {
        ht_resize_up(ht);
    }

    int index = ht_get_hash(key, ht->size, 0);
    ht_item *item = ht->items[index];

    int hash_attempt = 1;

    while (item != NULL) {
        if (item != &HT_DELETED_ITEM) {
            if (strcmp(item->key, key) == 0) {
                ht_del_item(item);
                ht->items[index] = &HT_DELETED_ITEM;
            }
        }
        index = ht_get_hash(key, ht->size, hash_attempt++);
        item = ht->items[index];
    }
    ht->count--;
}