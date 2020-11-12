#ifndef CANONICAL_TYPE_H
#define CANONICAL_TYPE_H

#define KEY_CANONICAL_FUNC 0
#define KEY_CANONICAL_USE 1
#define KEY_CANONICAL_QUAL 2

struct canonicalTypeBase
{
  int key;
};
typedef struct canonicalTypeBase CanonicalType;

// Type function_type(Declarations formals,Declarations return_values)
struct Canonical_function_type
{
  int key; /* KEY_CANONICAL_FUNC */
  int num_formals;
  CanonicalType *return_type;
  CanonicalType *param_types[];
};

struct Canonical_qual_type
{
  int key; /* KEY_CANONICAL_QUAL */
  Declaration decl;
  CanonicalType *source;
};

struct Canonical_use
{
  int key; /* KEY_CANONICAL_USE */
  Declaration decl;
};

// Immutable Set with total ordering property
struct CanonicalTypeSet_type
{
  int size;
  CanonicalType *members[];
};

typedef struct CanonicalTypeSet_type CanonicalTypeSet;

CanonicalType *canonical_type(Type ty);

CanonicalType *canonical_type_base_type(CanonicalType *canonicalType);

CanonicalType *canonical_type_join(CanonicalType *ctypeOuter, CanonicalType *ctypeInner, bool isBaseType);

#endif
