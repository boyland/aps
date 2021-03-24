#ifndef CANONICAL_SIGNATURE_H
#define CANONICAL_SIGNATURE_H

struct CanonicalSignature_type
{
  bool is_input;
  bool is_var;
  Declaration source_class;
  int num_actuals;
  CanonicalType *actuals[];
};

typedef struct CanonicalSignature_type CanonicalSignature;

typedef HASH_CONS_SET CanonicalSignatureSet;

/**
 * Should accumulate the signatures in a restrictive way not additive mannger
 * @param ctype canonical type
 * @return canonical signature set
 */
CanonicalSignatureSet infer_canonical_signatures(CanonicalType *ctype);

/**
 * Initializes the necessary stuff needed to find the canonical signatures
 * @param module_PHYLUM
 * @param type_PHYLUM
 */
void initialize_canonical_signature(Declaration module_PHYLUM, Declaration type_PHYLUM);

/**
 * Given a canonical signature, prints it to the file output
 * @param untyped untyped canonical type
 * @return f FILE output
 */
void print_canonical_signature(void *untyped, FILE *f);

/**
 * Given a canonical signature set, prints it to the file output
 * @param untyped untyped canonical type
 * @return f FILE output
 */
void print_canonical_signature_set(void *untyped, FILE *f);

#endif
