#ifndef CANONICAL_SIGNATURE_H
#define CANONICAL_SIGNATURE_H

typedef struct InferredSignature_t {
    bool is_input;
    bool is_var;
    Class _class;
    CanonicalType *actuals;
    int num_actuals;
} * InferredSignature;

InferredSignature new_inferred_sig(bool is_input, bool is_var, Class _class, int num_actuals, Type *actuals);

#endif
