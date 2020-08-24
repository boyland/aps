#ifndef CANONICAL_SIGNATURE_H
#define CANONICAL_SIGNATURE_H

typedef struct InferredSignature_t
{
  bool is_input;
  bool is_var;
  Class _class;
  int num_actuals;
  CanonicalType actuals[];
} * InferredSignature;

InferredSignature inferred_sig(bool is_input, bool is_var, Class _class, int num_actuals, CanonicalType *actuals);

#endif
