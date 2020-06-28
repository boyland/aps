#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aps-ag.h"
#include "jbb-alloc.h"

/**
 * http://www.cse.yorku.ca/~oz/hash.html
 */
int hash_string(unsigned char *str) {
  unsigned long hash = 5381;
  int c;

  while (c = *str++) {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

/**
 * Combine two hash values into one
 */
int hash_mix(int h1, int h2) {
  int hash = 17;
  hash = hash * 31 + h1;
  hash = hash * 31 + h2;
  return hash;
}

int canonical_type_hash(void *arg) {
  if (arg == NULL) return 0;

  struct canonicalTypeBase *canonical_type = (struct CanonicalType *)arg;

  switch (canonical_type->key) {
    case KEY_CANONICAL_USE: {
      struct Canonical_use *canonical_use_type = (struct Canonical_use *)arg;
      return hash_mix(canonical_use_type->key, hash_string(decl_name(canonical_use_type->decl)));
    }
    case KEY_CANONICAL_QUAL: {
      struct Canonical_qual_type *canonical_qual_type = (struct Canonical_qual_type *)arg;
      return hash_mix(canonical_qual_type->key,
                      hash_mix(hash_string(decl_name(canonical_qual_type->decl)),
                               canonical_type_hash(canonical_qual_type->source)));
    }
    case KEY_CANONICAL_FUNC: {
      struct Canonical_function_type *canonical_function_type =
          (struct Canonical_function_type *)arg;

      int index;
      int param_types_hash = 0;
      for (index = 0; index < canonical_function_type->num_formals; index++) {
        param_types_hash = hash_mix(
            param_types_hash, canonical_type_hash(&canonical_function_type->param_types[index]));
      }

      return hash_mix(
          canonical_function_type->key,
          hash_mix(canonical_function_type->num_formals,
                   hash_mix(param_types_hash,
                            canonical_type_hash(canonical_function_type->return_type))));
    }
    default:
      break;
  }
}

bool canonical_type_equal(void *a, void *b) {
  if (a == NULL || b == NULL) return false;

  struct canonicalTypeBase *canonical_type = (struct CanonicalType *)arg;

  switch (canonical_type->key) {
    case KEY_CANONICAL_USE: {
      struct Canonical_use *canonical_use_type = (struct Canonical_use *)arg;
      return hash_mix(canonical_use_type->key, hash_string(decl_name(canonical_use_type->decl)));
    }
    case KEY_CANONICAL_QUAL: {
      struct Canonical_qual_type *canonical_qual_type = (struct Canonical_qual_type *)arg;
      return hash_mix(canonical_qual_type->key,
                      hash_mix(hash_string(decl_name(canonical_qual_type->decl)),
                               canonical_type_hash(canonical_qual_type->source)));
    }
    case KEY_CANONICAL_FUNC: {
      struct Canonical_function_type *canonical_function_type =
          (struct Canonical_function_type *)arg;

      int index;
      int param_types_hash = 0;
      for (index = 0; index < canonical_function_type->num_formals; index++) {
        param_types_hash = hash_mix(
            param_types_hash, canonical_type_hash(&canonical_function_type->param_types[index]));
      }

      return hash_mix(
          canonical_function_type->key,
          hash_mix(canonical_function_type->num_formals,
                   hash_mix(param_types_hash,
                            canonical_type_hash(canonical_function_type->return_type))));
    }
    default:
      break;
  }
}

static struct hash_cons_table *canonical_type_table = {NULL, NULL, NULL, canonical_type_hash,
                                                       canonical_type_equal};

CanonicalType *from_type(Type t) {
  if (t == NULL) return NULL;

  switch (Type_KEY(t)) {
    case KEYtype_use: {
      Use use = type_use_use(t);
      switch (Use_KEY(use)) {
        case KEYuse: {
          struct Canonical_use ctype_use = {KEY_CANONICAL_USE, USE_DECL(type_use_use(t))};

          return hash_cons_get(&ctype_use, sizeof(ctype_use), canonical_type_table);
        }
        case KEYqual_use: {
          struct Canonical_qual_type cqual_use = {KEY_CANONICAL_QUAL, USE_DECL(type_use_use(t)),
                                                  from_type(qual_use_from(use))};

          return hash_cons_get(&cqual_use, sizeof(cqual_use), canonical_type_table);
        }
      }
    }
    case KEYfunction_type: {
      struct Canonical_function_type ctype_function = {KEY_CANONICAL_FUNC,
                                                       USE_DECL(type_use_use(t))};

      return hash_cons_get(&ctype_function, sizeof(ctype_function), canonical_type_table);
    }
    default: {
      aps_error(t, "Case is not implemented in from_type()");
      return NULL;
    }
  }
}
