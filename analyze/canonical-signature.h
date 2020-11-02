#ifndef CANONICAL_SIGNATURE_H
#define CANONICAL_SIGNATURE_H

#define EMPTY_CANONICAL_SIGNATURE_SET NULL

struct CanonicalSignature_type
{
  bool is_input;
  bool is_var;
  Class _class;
  int num_actuals;
  CanonicalType *actuals[];
};

typedef struct CanonicalSignature_type CanonicalSignature;

// Immutable Set with total ordering property
struct canonicalSignatureSet_type
{
  int size;
  CanonicalSignature *members[]; /* MUST be sorted because otherwise it won't be unique*/
};

typedef struct canonicalSignatureSet_type CanonicalSignatureSet;

CanonicalSignatureSet *infer_canonical_signatures(CanonicalType *ctype);

#endif
