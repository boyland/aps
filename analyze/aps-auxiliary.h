#ifndef APS_AUXILIARY_H
#define APS_AUXILIARY_H


/**
 * Mimicking sig_inst
 */
typedef struct InferredSignature_t {
    struct InferredSignature_t *next;
    Boolean is_input;
    Boolean is_var;
    Class _class;
    Type *actuals;
    int num_actuals;
} * InferredSignature;

InferredSignature create_inferred_sig(Boolean is_input, Boolean is_var, Class _class,
                                      int num_actuals, Type *actuals);

#endif
