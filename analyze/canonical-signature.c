#include <stdio.h>
#include "aps-ag.h"
#include "jbb-alloc.h"
#include "canonical-type.c"

int hash_string(unsigned char *str);
int hash_mix(int h1, int h2);
int hash_use(Use u);
int hash_class(Class cl);

/**
 * Hash string
 * @param string
 * @return intger hash value
 */
int hash_string(unsigned char *str)
{
  unsigned long hash = 5381;
  int c;

  while (c = *str++)
  {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

/**
 * Combine two hash values into one
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

/**
 * Hashes list of CanonicalType
 * @param count
 * @param types
 * @return combined hash 
 */
int hash_canonical_types(int count, CanonicalType **types)
{
  int h = 0;
  int i;
  for (i = 0; i < count; i++)
  {
    h = hash_mix(h, canonical_type_hash(types[i]));
  }

  return h;
}

int hash_type(Type t)
{
  if (t == 0)
  {
    return 0;
  }
  switch (Type_KEY(t))
  {
  case KEYtype_use:
    return hash_use(type_use_use(t));
  case KEYprivate_type:
    return hash_mix(hash_string("private"), hash_type(private_type_rep(t)));
  case KEYremote_type:
    return hash_mix(hash_string("remote"), hash_type(remote_type_nodetype(t)));
  case KEYfunction_type:
  {
    unsigned long h = hash_string("function");

    Declarations fs = function_type_formals(t);
    Declarations rs = function_type_return_values(t);
    Declaration a, r;
    int started = FALSE;
    for (a = first_Declaration(fs); a; a = DECL_NEXT(a))
    {
      h = hash_mix(h, hash_string(decl_name(a)));
    }

    for (r = first_Declaration(rs); r; r = DECL_NEXT(r))
    {
      h = hash_mix(h, hash_string(decl_name(r)));
    }

    return h;
  }
  case KEYtype_inst:
  {
    unsigned long h = hash_use(module_use_use(type_inst_module(t)));

    TypeActuals as = type_inst_type_actuals(t);
    Type a;
    for (a = first_TypeActual(as); a; a = TYPE_NEXT(a))
    {
      h = hash_mix(h, hash_type(a));
    }
    return h;
  }
  case KEYno_type:
    return hash_string("<notype>");
    break;
  default:
    return 0x1ffffffffu;
  }
}

/**
 * Hashes Use
 * @param u
 * @return hash value
 */
int hash_use(Use u)
{
  if (u == 0)
  {
    return 0;
  }
  switch (Use_KEY(u))
  {
  case KEYuse:
    return hash_string(symbol_name(use_name(u)));
  case KEYqual_use:
    return hash_mix(hash_type(qual_use_from(u)), hash_string(symbol_name(qual_use_name(u))));
  default:
    return 0x1ffffffffu;
  }
}

/**
 * Hashes Class
 * @param cl
 * @return hash value
 */
int hash_class(Class cl)
{
  if (cl == 0)
  {
    return 0;
  }
  switch (Class_KEY(cl))
  {
  case KEYclass_use:
    return hash_use(class_use_use(cl));
  default:
    return 0x1ffffffffu;
  }
}

/**
 * Hashes InferredSignature
 * @param untyped InferredSignature
 * @return hash value
 */
int inferred_signature_hash(void *untyped)
{
  struct InferredSignature_t *inferred_sig = (struct InferredSignature_t *)untyped;

  return hash_mix((int)inferred_sig->is_input, hash_mix((int)inferred_sig->is_var, hash_mix(hash_class(inferred_sig->_class), hash_canonical_types(inferred_sig->num_actuals, inferred_sig->actuals))));
}

/**
 * Equality test for InferredSignature
 * @param untyped1 untyped CanonicalType
 * @param untyped2 untyped CanonicalType
 * @return boolean indicating the result of equality
 */
bool inferred_signature_equal(void *untyped1, void *untyped2)
{
  struct InferredSignature_t *inferred_sig1 = (struct InferredSignature_t *)untyped1;
  struct InferredSignature_t *inferred_sig2 = (struct InferredSignature_t *)untyped2;

  if (inferred_sig1->num_actuals != inferred_sig2->num_actuals)
  {
    return false;
  }

  bool result = true;
  int i;
  for (i = 0; index < inferred_sig1->num_actuals; i++)
  {
    result &= canonical_type_equal(inferred_sig1->actuals[i], inferred_sig2->actuals[i]);
  }

  return inferred_sig1->is_input == inferred_sig2->is_input && inferred_sig1->is_var == inferred_sig2->is_var && inferred_sig1->_class == inferred_sig2->_class;
}

static struct hash_cons_table *canonical_signature_table = {inferred_signature_hash, inferred_signature_equal};

InferredSignature inferred_sig(bool is_input, bool is_var, Class class, int num_actuals, CanonicalType *actuals)
{

  long long_key =
      hash_mix((int)is_input,
               hash_mix((int)is_var,
                        hash_mix(hash_class(class),
                                 hash_mix(num_actuals, hash_canonical_types(num_actuals, actuals)))));

  size_t my_size = sizeof(struct InferredSignature_t) + num_actuals * (sizeof(CanonicalType *));

  struct InferredSignature_t *result = (struct InferredSignature_t *)alloca(my_size);

  result = (InferredSignature)HALLOC(sizeof(struct InferredSignature_t));
  result->is_input = is_input;
  result->is_var = is_var;
  result->_class = class;
  result->num_actuals = num_actuals;

  int i;
  for (i = 0; i < num_actuals; i++)
  {
    result->actuals[i] = actuals[i];
  }

  void *memory = hash_cons_get(&result, sizeof(result), &canonical_signature_table);
  memcpy(memory, &result, sizeof(result));
  return (InferredSignature *)memory;
}
