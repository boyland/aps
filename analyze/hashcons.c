#include "hashcons.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "prime.h"

#define HC_INITIAL_BASE_SIZE 61
#define MAX_DENSITY 0.5
#define DOUBLE_SIZE(x) (((x) << 1) + 1)

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
  int hash = hc->hashf(item) & INT_MAX;
  int index = hash % hc->capacity;
  int step_size = hash % (hc->capacity - 2) + 1;

  while (true)
  {
    if (hc->table[index] == NULL || hc->equalf(hc->table[index], item))
    {
      return index;
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

  int candidate_index = hc_search(hc, item);

  if (hc->table[candidate_index] != NULL)
  {
    return hc->table[candidate_index];
  }

  void *result = malloc(temp_size);
  memcpy(result, item, temp_size);

  hc_insert_at(hc, result, candidate_index);

  if (hc->size > hc->capacity * MAX_DENSITY)
  {
    const int new_capacity = next_twin_prime(DOUBLE_SIZE(hc->capacity));
    hc_resize(hc, new_capacity);
  }

  return result;
}

/**
 * Hashes hashcons set
 * @param untyped InferredSignature
 * @return hash integer value
 */
static int hashcons_set_hash(void *untyped)
{
  HASH_CONS_SET set = (HASH_CONS_SET)untyped;

  int i, hash = 17;
  for (i = 0; i < set->num_elements; i++) hash |= ((int)set->elements[i]);

  return hash;
}

/**
 * Equality test for hashcons set
 * @param untyped1 untyped hashcons set
 * @param untyped2 untyped hashcons set
 * @return boolean indicating the result of equality
 */
static bool hashcons_set_equal(void *untyped1, void *untyped2)
{
  HASH_CONS_SET set_a = (HASH_CONS_SET)untyped1;
  HASH_CONS_SET set_b = (HASH_CONS_SET)untyped2;

  if (set_a->num_elements != set_b->num_elements) return false;

  int i, hash = 0;
  for (i = 0; i < set_a->num_elements; i++)
  {
    if (set_a->elements[i] != set_b->elements[i]) return false;
  }

  return true;
}

/**
 * Used to hold hashconsed sets
 */
static struct hash_cons_table hashcons_set_table = { hashcons_set_hash, hashcons_set_equal };

/**
 * Take a temporary set and hash cons it, returning the set that results
 * NOTE: The elements array will be sorted by address to ensure a canonical representation.
 * @param set hashcons set
 * @return new hashcons set that includes the item
 */
HASH_CONS_SET new_hash_cons_set(HASH_CONS_SET set)
{
  size_t struct_size = sizeof(struct hash_cons_set) + set->num_elements * sizeof(void *);
  HASH_CONS_SET sorted_set = (HASH_CONS_SET)alloca(struct_size);
  sorted_set->num_elements = 0;

  int i, j;
  for (i = 0; i < set->num_elements; i++)
  {
    void* item = set->elements[i];
    int key = (int) item; 
    int j = i - 1;

    while (j >= 0 && (int)sorted_set->elements[j] > key)
    { 
      sorted_set->elements[j + 1] = sorted_set->elements[j]; 
      j--; 
    }

    sorted_set->elements[j + 1] = item;
    sorted_set->num_elements++;
  }

  void *memory = hash_cons_get(sorted_set, struct_size, &hashcons_set_table);
  return (HASH_CONS_SET)memory;
}

/**
 * Return the empty set
 * @return hashconsed empty set
 */
HASH_CONS_SET get_hash_cons_empty_set()
{
  struct hash_cons_set empty_set = (struct hash_cons_set) { 0 };

  void *memory = hash_cons_get(&empty_set, sizeof(empty_set), &hashcons_set_table);
  return (HASH_CONS_SET)memory;
}

/**
 * Adds an element to the hashcons set, returning the set that results
 * @param item item to be added to the set
 * @param set hashcons set
 * @return new hashcons set that includes the item
 */
HASH_CONS_SET add_hash_cons_set(void *item, HASH_CONS_SET set)
{
  size_t struct_size = sizeof(struct hash_cons_set) + 1 * sizeof(void *);
  HASH_CONS_SET single_element_set = (HASH_CONS_SET)alloca(struct_size);
  single_element_set->num_elements = 1;
  single_element_set->elements[0] = item;

  return union_hash_const_set(single_element_set, set);
}

/**
 * Unions two hashcons set, returning the set that results
 * @param set_a hashcons set A
 * @param set_b hashcons set B
 * @return new hashcons set that includes the item
 */
HASH_CONS_SET union_hash_const_set(HASH_CONS_SET set_a, HASH_CONS_SET set_b)
{
  int updated_count = set_a->num_elements + set_b->num_elements;
  size_t item_size = sizeof(void *);
  size_t struct_size = sizeof(struct hash_cons_set) + updated_count * item_size;
  HASH_CONS_SET sorted_set = (HASH_CONS_SET)alloca(struct_size);
  sorted_set->num_elements = updated_count;

  int i = 0, j = 0, k = 0;
  while (i < set_a->num_elements && j < set_b->num_elements)
  {
    if (set_a->elements[i] == set_b->elements[j])
    {
      sorted_set->elements[k] = set_a->elements[i];
      sorted_set->num_elements--;
      struct_size -= item_size;
      i++;
      j++;
    }
    else if (set_a->elements[i] < set_b->elements[j])
    {
      sorted_set->elements[k] = set_a->elements[i];
      i++;
    }
    else
    {
      sorted_set->elements[k] = set_b->elements[j];
      j++;
    }

    k++;
  }

  while (i < set_a->num_elements) sorted_set->elements[k++] = set_a->elements[i++];

  while (j < set_b->num_elements) sorted_set->elements[k++] = set_b->elements[j++];

  void *memory = hash_cons_get(sorted_set, struct_size, &hashcons_set_table);
  return (HASH_CONS_SET)memory;
}

/**
 * Hash string and returns a hash value
 * Source: http://www.cse.yorku.ca/~oz/hash.html
 * @param string
 * @return intger hash value
 */
int hash_string(char *str)
{
  int hash = 5381;
  int c;

  while ((c = *str++))
  {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

/**
 * Combine two hash values into one hash value
 * @param hash1
 * @param hash2
 * @return combined hash 
 */
int hash_mix(int h1, int h2)
{
  int hash = 17;
  hash = hash * 31 + h1;
  hash = hash * 31 + h2;
  return hash;
}
