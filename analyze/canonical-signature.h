#ifndef CANONICAL_SIGNATURE_H
#define CANONICAL_SIGNATURE_H

struct CanonicalSignature_type
{
  bool is_input;
  bool is_var;
  Class _class;
  int num_actuals;
  CanonicalType *actuals[];
};

typedef struct CanonicalSignature_type CanonicalSignature;

struct CanonicalSignatureSet_type
{
  int size;
  CanonicalSignature *members[]; /* MUST be sorted because otherwise it won't be unique*/
};

typedef struct CanonicalSignatureSet_type CanonicalSignatureSet;

/**
 * Should accumulate the signatures in a restrictive way not additive mannger
 * @param ctype canonical type
 * @return canonical signature set
 */
CanonicalSignatureSet *infer_canonical_signatures(CanonicalType *ctype);

#endif
