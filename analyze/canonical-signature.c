#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

int hash_class(Class cl);

static CanonicalSignatureSet *from_sig(Signature sig);
static CanonicalSignatureSet *union_canonical_signature_set(CanonicalSignatureSet *set1, CanonicalSignatureSet *set2);
static CanonicalSignatureSet *new_canonical_signature_set(CanonicalSignature *cSig);

static struct CanonicalSignatureSet_type EMPTY_CANONICAL_SIGNATURE_SET = {0, NULL};

void print_canonical_signature(void *untyped, FILE *f)
{
  if (f == 0)
  {
    f = stdout;
  }
  if (untyped == 0)
  {
    fprintf(f, "<null>");
    return;
  }

  struct CanonicalSignature_type *canonical_signature = (struct CanonicalSignature_type *)untyped;

  print_Use(class_use_use(canonical_signature->_class), f);
  fputc('[', f);

  int started = false;
  int i;
  for (i = 0; i < canonical_signature->num_actuals; i++)
  {
    if (started)
    {
      fputc(',', f);
    }
    else
    {
      started = true;
    }
    print_canonical_type(canonical_signature->actuals[i], f);
  }

  fputc(']', f);
}

void print_canonical_signature_set(void *untyped, FILE *f)
{
  if (f == 0)
  {
    f = stdout;
  }
  if (untyped == 0)
  {
    fprintf(f, "[]");
    return;
  }

  struct CanonicalSignatureSet_type *canonical_signature_set = (struct CanonicalSignatureSet_type *)untyped;

  fputc('[', f);

  int started = false;
  int i;
  for (i = 0; i < canonical_signature_set->size; i++)
  {
    if (started)
    {
      fputc(',', f);
    }
    else
    {
      started = true;
    }
    print_canonical_signature(canonical_signature_set->members[i], f);
  }

  fputc(']', f);
}

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
  struct CanonicalSignature_type *inferred_sig = (struct CanonicalSignature *)untyped;

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
  struct CanonicalSignature_type *inferred_sig1 = (struct CanonicalSignature *)untyped1;
  struct CanonicalSignature_type *inferred_sig2 = (struct CanonicalSignature *)untyped2;

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

int canonical_signature_set_hash(void *c)
{
  struct CanonicalSignatureSet_type *c_set = (struct CanonicalSignatureSet *)c;
  int h = 0;
  int i;
  for (i = 0; i < c_set->size; i++)
  {
    h = hash_mix(h, canonical_signature_hash(c_set->members[i]));
  }

  return h;
}

bool canonical_signature_set_equal(void *c1, void *c2)
{
  struct CanonicalSignatureSet_type *c_set1 = (struct CanonicalSignatureSet_type *)c1;
  struct CanonicalSignatureSet_type *c_set2 = (struct CanonicalSignatureSet_type *)c2;

  if (c_set1->size != c_set2->size)
  {
    return false;
  }

  int i;
  for (i = 0; i < c_set1->size; i++)
  {
    if (c_set1->members[i] != c_set2->members[i])
    {
      return false;
    }
  }

  return true;
}

static struct hash_cons_table canonical_signature_table = {canonical_signature_hash, canonical_signature_equal};
static struct hash_cons_table canonical_signature_set_table = {canonical_signature_set_hash, canonical_signature_set_equal};

/**
 * TODO: actuals needs to be canonical base types
 * LIST of integer should signature of LIST of integer lattice
 */
CanonicalSignature *new_canonical_signature(bool is_input, bool is_var, Class class, int num_actuals, CanonicalType *actuals[])
{
  size_t struct_size = sizeof(struct CanonicalSignature_type) + num_actuals * (sizeof(CanonicalType *));

  struct CanonicalSignature_type *result = (struct CanonicalSignature_type *)alloca(struct_size);
  result->is_input = is_input;
  result->is_var = is_var;
  result->_class = class;
  result->num_actuals = num_actuals;

  int i;
  for (i = 0; i < num_actuals; i++)
  {
    result->actuals[i] = (actuals[i]);
  }

  void *memory = hash_cons_get(result, struct_size, &canonical_signature_table);
  return (CanonicalSignature *)memory;
}

/**
 * Counts number of Declarations
 */
static int count_actuals(TypeActuals type_actuals)
{
  switch (TypeActuals_KEY(type_actuals))
  {
  default:
    fatal_error("count_declarations crashed");
  case KEYnil_TypeActuals:
    return 0;
  case KEYlist_TypeActuals:
    return 1;
  case KEYappend_TypeActuals:
    return count_actuals(append_TypeActuals_l1(type_actuals)) + count_actuals(append_TypeActuals_l2(type_actuals));
  }
}

static CanonicalSignature *from_sig_inst(Signature sig)
{
  TypeActuals actuals = sig_inst_actuals(sig);
  int num_actuals = count_actuals(actuals);
  size_t my_size = num_actuals * sizeof(CanonicalType *);
  CanonicalType **result = (CanonicalType **)alloca(my_size);

  int i = 0;
  Type type = first_TypeActual(actuals);

  while (type != NULL)
  {
    result[i] = canonical_type_base_type(canonical_type(type));
    type = TYPE_NEXT(type);
    i++;
  }

  return new_canonical_signature(sig_inst_is_input(sig), sig_inst_is_var(sig), sig_inst_class(sig), num_actuals, result);
}

static CanonicalSignatureSet *from_mult_sig(Signature sig)
{
  return union_canonical_signature_set(from_sig(mult_sig_sig1(sig)), from_sig(mult_sig_sig2(sig)));
}

static CanonicalSignatureSet *from_sig(Signature sig)
{
  // printf("key %d\n", (int)Signature_KEY(sig));

  switch (Signature_KEY(sig))
  {
  case KEYsig_inst:
    return new_canonical_signature_set(from_sig_inst(sig));
  case KEYmult_sig:
    return from_mult_sig(sig);
  case KEYsig_use:
  case KEYno_sig:
  case KEYfixed_sig:
    return &EMPTY_CANONICAL_SIGNATURE_SET;
  }
}

/**
 * Returns a single canonical signature set
 * @param cSig canonical signature
 * @return canonical signature set 
 */
static CanonicalSignatureSet *new_canonical_signature_set(CanonicalSignature *cSig)
{
  size_t my_size = sizeof(struct CanonicalSignatureSet_type) + 1 * (sizeof(CanonicalSignature *));

  struct CanonicalSignatureSet_type *result = (struct CanonicalSignatureSet *)alloca(my_size);

  result->size = 1;
  result->members[0] = cSig;

  CanonicalSignatureSet *amir = hash_cons_get(result, my_size, &canonical_signature_set_table);

  return amir;
}

/**
* TODO: replace this implementation with a better approach that does not use hash function
* @param sig1 Canonical signature
* @param sig2 Canonical signature
* @return 0, -1, 1 indicating the result of comparison
*/
static int canonical_signature_compare(CanonicalSignature *sig1, CanonicalSignature *sig2)
{
  int h1 = canonical_signature_hash(sig1);
  int h2 = canonical_signature_hash(sig2);

  return h1 == h2 ? 0 : (h1 < h2 ? -1 : 1);
}

/**
 * Function that merges two sorted canonical signature lists
 * @param set1 canonical signature list A
 * @param set1 canonical signature list B
 * @return resuling canonical signature result
 */
static CanonicalSignatureSet *union_canonical_signature_set(CanonicalSignatureSet *set1, CanonicalSignatureSet *set2)
{
  if (set1->size == 0)
  {
    return set2;
  }
  else if (set2->size == 0)
  {
    return set1;
  }
  else if (set1->size == 0 && set2->size == 0)
  {
    return &EMPTY_CANONICAL_SIGNATURE_SET;
  }
  else
  {
    size_t my_size = sizeof(struct CanonicalSignatureSet_type) + (set1->size + set2->size) * (sizeof(CanonicalSignature *));

    // printf("\nA: ");
    // print_canonical_signature_set(set1, stdout);
    // printf("\nB: ");
    // print_canonical_signature_set(set2, stdout);
    // printf("\n");

    struct CanonicalSignatureSet_type *result = (struct CanonicalSignatureSet_type *)alloca(my_size);
    result->size = set1->size + set2->size;

    int i = 0,
        j = 0, k = 0;
    while (i < set1->size && j < set2->size)
    {
      int comp = canonical_signature_compare(set1->members[i], set2->members[j]);
      if (comp == 0)
      {
        result->members[k++] = set1->members[i++, j++];
        result->size--;
      }
      else if (comp < 0)
      {
        result->members[k++] = set1->members[i++];
      }
      else
      {
        result->members[k++] = set2->members[j++];
      }
    }

    while (i < set1->size)
    {
      result->members[k++] = set1->members[i++];
    }

    while (j < set2->size)
    {
      result->members[k++] = set2->members[j++];
    }

    CanonicalSignatureSet *amir = hash_cons_get(result, my_size, &canonical_signature_set_table);

    return amir;
  }
}

// Don't accumulate the fixed sigs
CanonicalSignatureSet *infer_canonical_signatures(CanonicalType *ctype)
{
  CanonicalSignatureSet *result = &EMPTY_CANONICAL_SIGNATURE_SET;
  bool flag = true;

  if (ctype == NULL)
  {
    return &EMPTY_CANONICAL_SIGNATURE_SET;
  }

  do
  {
    switch (ctype->key)
    {
    case KEY_CANONICAL_USE:
    {
      struct Canonical_use *canonical_use_type = (struct Canonical_use *)ctype;

      Declaration decl = canonical_use_type->decl;
      switch (Declaration_KEY(decl))
      {
      case KEYsome_type_decl:
      {
        Signature sig = some_type_decl_sig(decl);
        result = union_canonical_signature_set(result, from_sig(sig));
        break;
      }
      case KEYsome_type_formal:
      {
        Signature sig = some_type_formal_sig(decl);
        result = union_canonical_signature_set(result, from_sig(sig));
        break;
      }
      }

      break;
    }
    case KEY_CANONICAL_QUAL:
    {
      struct Canonical_qual_type *canonical_qual_type = (struct Canonical_qual_type *)ctype;

      result = union_canonical_signature_set(result, infer_canonical_signatures(canonical_qual_type->source));

      Declaration decl = canonical_qual_type->decl;
      switch (Declaration_KEY(decl))
      {
      case KEYsome_type_decl:
      {
        Signature sig = some_type_decl_sig(decl);
        result = union_canonical_signature_set(result, from_sig(sig));
        break;
      }
      case KEYsome_type_formal:
      {
        Signature sig = some_type_formal_sig(decl);
        result = union_canonical_signature_set(result, from_sig(sig));
        break;
      }
      }

      break;
    }
    case KEY_CANONICAL_FUNC:
    {
      struct Canonical_function_type *canonical_function_type = (struct Canonical_function_type *)ctype;
      flag = false;
      break;
    }
    }

    CanonicalType *base_type = canonical_type_base_type(ctype);
    flag &= !(base_type == NULL || base_type == ctype);

    ctype = base_type;

  } while (flag);

  size_t my_size = sizeof(struct CanonicalSignatureSet_type) + result->size * (sizeof(CanonicalSignature *));

  return hash_cons_get(result, my_size, &canonical_signature_set_table);
}