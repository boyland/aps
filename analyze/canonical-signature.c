#include <stdio.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

int hash_class(Class cl);

/**
 * Hashes list of CanonicalType
 * @param count
 * @param types
 * @return combined hash 
 */
int hash_canonical_types(int count, CanonicalType *types)
{
  int h = 0;
  int i;
  for (i = 0; i < count; i++)
  {
    h = hash_mix(h, canonical_type_hash(&types[i]));
  }

  return h;
}

/**
 * Hashes Class
 * @param cl
 * @return hash value
 */
int hash_class(Class cl)
{
  return (int)cl;
}

/**
 * Hashes InferredSignature
 * @param untyped InferredSignature
 * @return hash value
 */
int canonical_signature_hash(void *untyped)
{
  struct CanonicalSignature_type *inferred_sig = (struct CanonicalSignature_type *)untyped;

  return hash_mix((int)inferred_sig->is_input, hash_mix((int)inferred_sig->is_var, hash_mix(hash_class(inferred_sig->_class), hash_canonical_types(inferred_sig->num_actuals, inferred_sig->actuals))));
}

/**
 * Equality test for InferredSignature
 * @param untyped1 untyped CanonicalType
 * @param untyped2 untyped CanonicalType
 * @return boolean indicating the result of equality
 */
bool canonical_signature_equal(void *untyped1, void *untyped2)
{
  struct CanonicalSignature_type *inferred_sig1 = (struct CanonicalSignature_type *)untyped1;
  struct CanonicalSignature_type *inferred_sig2 = (struct CanonicalSignature_type *)untyped2;

  if (inferred_sig1->num_actuals != inferred_sig2->num_actuals)
  {
    return false;
  }

  bool result = true;
  int i;
  for (i = 0; i < inferred_sig1->num_actuals; i++)
  {
    result &= (&inferred_sig1->actuals[i] == &inferred_sig2->actuals[i]);
  }

  return inferred_sig1->is_input == inferred_sig2->is_input && inferred_sig1->is_var == inferred_sig2->is_var && inferred_sig1->_class == inferred_sig2->_class;
}

static struct hash_cons_table *canonical_signature_table = {canonical_signature_hash, canonical_signature_equal};

/**
 * TODO: actuals needs to be canonical base types
 * LIST of integer should signature of LIST of integer lattice
 */
CanonicalSignature *new_canonical_signature(bool is_input, bool is_var, Class class, int num_actuals, CanonicalType *actuals)
{
  int long_key =
      hash_mix((int)is_input,
               hash_mix((int)is_var,
                        hash_mix(hash_class(class),
                                 hash_mix(num_actuals, hash_canonical_types(num_actuals, actuals)))));

  size_t struct_size = sizeof(struct CanonicalSignature_type) + num_actuals * (sizeof(CanonicalType *));

  struct CanonicalSignature_type *result = (struct CanonicalSignature_type *)alloca(struct_size);

  result = (CanonicalSignature *)alloca(sizeof(struct CanonicalSignature_type));
  result->is_input = is_input;
  result->is_var = is_var;
  result->_class = class;
  result->num_actuals = num_actuals;

  int i;
  for (i = 0; i < num_actuals; i++)
  {
    *result->actuals[i] = (actuals[i]);
  }

  void *memory = hash_cons_get(&result, struct_size, &canonical_signature_table);
  return (CanonicalSignature *)memory;
}

/**
 * Returns a single canonical signature set
 * @param cSig canonical signature
 * @return canonical signature set 
 */
static CanonicalSignatureSet *new_canonical_signature_set(CanonicalSignature *cSig)
{
  size_t my_size = sizeof(struct canonicalSignatureSet_type) + 1 * (sizeof(CanonicalSignature *));

  struct canonicalSignatureSet_type *result = (struct canonicalSignatureSet_type *)malloc(my_size);

  result->size = 1;
  result->members[0] = cSig;

  return result;
}

// Total order that is only returns tree for things that are equal
static int canonical_signature_compare(CanonicalSignature *sig1, CanonicalSignature *sig2)
{
  int h1 = hash_canonical_signature(sig1);
  int h2 = hash_canonical_signature(sig2);

  return h1 == h2 ? 0 : (h1 < h2 ? -1 : 1);
}

/**
 * Function that merges two sorted canonical signature lists
 * @param set1 canonical signature list A
 * @param set1 canonical signature list B
 * @return resuling canonical signature result
 */
CanonicalSignatureSet *union_canonical_signature_set(CanonicalSignatureSet *set1, CanonicalSignatureSet *set2)
{
  if (set1 == EMPTY_CANONICAL_SIGNATURE_SET)
  {
    return set2;
  }
  else if (set2 == EMPTY_CANONICAL_SIGNATURE_SET)
  {
    return set1;
  }
  else
  {
    size_t my_size = sizeof(struct canonicalSignatureSet_type) + set1->size + set2->size * (sizeof(CanonicalSignature *));

    struct canonicalSignatureSet_type *result = (struct canonicalSignatureSet_type *)malloc(my_size);

    int i = 0,
        j = 0, k = 0;
    while (k++ < set1->size + set2->size)
    {
      switch (canonical_signature_compare(set1->members[i], set2->members[j]))
      {
      case 0:
        result->members[k++] = set1->members[i++, j++];
        break;
      case -1:
        result->members[k] = set1->members[i++];
        result->size++;
        break;
      case 1:
        result->members[k] = set1->members[j++];
        result->size++;
        break;
      }
    }

    return result;
  }
}

CanonicalSignatureSet *infer_canonical_signatures(CanonicalType *ctype)
{
}