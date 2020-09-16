#include "hashcons.h"
#include <stdlib.h>
#include <string.h>
#include "prime.h"

#define HC_INITIAL_BASE_SIZE 61
#define MAX_DENSITY 0.5
#define DOUBLE_SIZE(x) ((x << 1) + 1)

/**
 * Initializes a table
 * @param hc table
 * @param capacity new capacity
 */
void hc_initialize(HASH_CONS_TABLE hc, const int capacity)
{
  hc->capacity = capacity;
  hc->table = calloc(hc->capacity, sizeof(void *));
  hc->size = 0;
}

/**
 * Finds the candidate index intended to get inserted or searched in table
 * @param hc table
 * @param item the item looking to be added or removed
 * @return
 */
static int hc_candidate_index(HASH_CONS_TABLE hc, void *item)
{
  int attempt = 0;
  int hash = hc->hashf(item);
  int index = hash % hc->capacity;
  int step_size = 0;

  while (true)
  {
    if (hc->table[index] == NULL || hc->equalf(hc->table[index], item))
    {
      return index;
    }

    if (attempt == 0)
    {
      step_size = hash % (hc->capacity - 2);
    }
    index = (index + step_size) % hc->capacity;
  }
}

/**
 * Insert an item into table
 * @param hc table
 * @param item the item intended to get inserted into the table
 */
static void hc_insert_at(HASH_CONS_TABLE hc, void *item, int index)
{
  hc->table[index] = item;
  hc->size++;
}

/**
 * Insert an item into table
 * @param hc table
 * @param item the item intended to get inserted into the table
 */
static void hc_insert(HASH_CONS_TABLE hc, void *item)
{
  int index = hc_candidate_index(hc, item);

  hc_insert_at(hc, item, index);
}

/**
 * Search an item in table
 * @param hc table
 * @param item the item intended to get searched in the table
 * @return possible index of the item
 */
static int hc_search(HASH_CONS_TABLE hc, void *item)
{
  int index = hc_candidate_index(hc, item);

  return index;
}

/**
 * Resizes the table given new capacity
 * @param hc table
 * @param capacity new capacity
 */
static void hc_resize(HASH_CONS_TABLE hc, const int capacity)
{
  void **old_table = hc->table;
  int old_capacity = hc->capacity;
  hc_initialize(hc, capacity);

  for (int i = 0; i < old_capacity; i++)
  {
    void *item = old_table[i];
    if (item != NULL)
    {
      hc_insert(hc, item);
    }
  }

  free(old_table);
}

/**
 * Insert an item into table if item is not already in table or just returns the existing item
 * @param item the item
 * @param temp_size item size
 * @param hc table
 * @return item just got inserted into the table or existing item
 */
void *hash_cons_get(void *item, size_t temp_size, HASH_CONS_TABLE hc)
{
  if (hc->table == NULL)
  {
    hc_initialize(hc, HC_INITIAL_BASE_SIZE);
  }

  if (hc->size > hc->capacity * MAX_DENSITY)
  {
    const int new_capacity = next_twin_prime(DOUBLE_SIZE(hc->capacity));
    hc_resize(hc, new_capacity);
  }

  int candidate_index = hc_search(hc, item);

  if (hc->table[candidate_index] != NULL)
  {
    return hc->table[candidate_index];
  }
  else
  {
    void *result = malloc(temp_size);
    memcpy(result, item, temp_size);

    hc_insert_at(hc, result, candidate_index);

    return result;
  }
}

int hash_string(char *str)
{
  int hash = 5381;
  int c;

  while (c = *str++)
  {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

int hash_mix(int h1, int h2)
{
  int hash = 17;
  hash = hash * 31 + h1;
  hash = hash * 31 + h2;
  return hash < 0 ? -hash : hash;
}