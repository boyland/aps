#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

int hash_source_class(Declaration cl);

static CanonicalSignatureSet *from_sig(Signature sig);
static CanonicalSignatureSet *from_type(Type t);
static CanonicalSignatureSet *from_declaration(Declaration decl);
static CanonicalSignatureSet *union_canonical_signature_set(CanonicalSignatureSet *set1, CanonicalSignatureSet *set2);
static CanonicalSignatureSet *new_canonical_signature_set(CanonicalSignature *cSig);

static CanonicalSignatureSet *EMPTY_CANONICAL_SIGNATURE_SET;
static Declaration module_PHYLUM;
static Declaration type_PHYLUM;
static bool initialized = false;

/**
 * Prints canonical signature 
 * @param untyped CanonicalSignature
 * @param f output stream
 */
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

  fprintf(f, "%s", decl_name(canonical_signature->source_class));
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

/**
 * Prints canonical signature set 
 * @param untyped CanonicalSignatureSet
 * @param f output stream
 */
void print_canonical_signature_set(void *untyped, FILE *f)
{
  if (f == 0)
  {
    f = stdout;
  }
  if (untyped == 0)
  {
    fprintf(f, "{}");
    return;
  }

  struct CanonicalSignatureSet_type *canonical_signature_set = (struct CanonicalSignatureSet_type *)untyped;

  fprintf(f, "{");

  int started = false;
  int i;
  for (i = 0; i < canonical_signature_set->size; i++)
  {
    if (started)
    {
      fprintf(f, ",");
    }
    else
    {
      started = true;
    }
    print_canonical_signature(canonical_signature_set->members[i], f);
  }

  fprintf(f, "}");
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

/**
 * Hashes Class declaration
 * @param cl Declaration
 * @return hash integer value
 */
int hash_source_class(Declaration cl)
{
  return (int)cl;
}

/**
 * Hashes CanonicalSignature
 * @param untyped InferredSignature
 * @return hash integer value
 */
int canonical_signature_hash(void *untyped)
{
  struct CanonicalSignature_type *inferred_sig = (struct CanonicalSignature *)untyped;

  return hash_mix((int)inferred_sig->is_input, hash_mix((int)inferred_sig->is_var, hash_mix(hash_source_class(inferred_sig->source_class), hash_canonical_types(inferred_sig->num_actuals, inferred_sig->actuals))));
}

/**
 * Equality test for CanonicalSignature
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
    result &= (inferred_sig1->actuals[i] == inferred_sig2->actuals[i]);
  }

  return inferred_sig1->is_input == inferred_sig2->is_input && inferred_sig1->is_var == inferred_sig2->is_var && inferred_sig1->source_class == inferred_sig2->source_class;
}

/**
 * Hashes CanonicalSignatureSet
 * @param untyped CanonicalSignatureSet
 * @return hash integer value
 */
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

/**
 * Equality test for CanonicalSignatureSet
 * @param untyped1 untyped CanonicalSignatureSet
 * @param untyped2 untyped CanonicalSignatureSet
 * @return boolean indicating the result of equality
 */
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
 * Constructor to create a new canonical signature
 * @param is_input
 * @param is_var
 * @param source_class
 * @param num_actuals
 * @param actuals
 * @return canonical signature
 */
CanonicalSignature *new_canonical_signature(bool is_input, bool is_var, Declaration source_class, int num_actuals, CanonicalType **actuals)
{
  size_t struct_size = sizeof(struct CanonicalSignature_type) + num_actuals * (sizeof(CanonicalType *));

  struct CanonicalSignature_type *result = (struct CanonicalSignature_type *)alloca(struct_size);
  result->is_input = is_input;
  result->is_var = is_var;
  result->source_class = source_class;
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
 * @param type_actuals
 * @return count of actuals in TypeActuals
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

/**
 * Tries to resolve canonical signature set from a Signature inst
 * @param sig Signature AST
 * @return Canonical signature set 
 */
static CanonicalSignatureSet *from_sig_inst(Signature sig)
{
  Declaration mdecl = USE_DECL(class_use_use(sig_inst_class(sig)));
  TypeActuals actuals = sig_inst_actuals(sig);
  int num_actuals = count_actuals(actuals);
  size_t my_size = num_actuals * sizeof(CanonicalType *);
  CanonicalType **result = (CanonicalType **)alloca(my_size);

  // printf("result: %s\nhere: ", decl_name(mdecl));
  // print_Signature(class_decl_parent(mdecl), stdout);

  int i = 0;
  Type type = first_TypeActual(actuals);

  while (type != NULL)
  {
    result[i] = canonical_type_base_type(canonical_type(type));
    type = TYPE_NEXT(type);
    i++;
  }

  return new_canonical_signature_set(new_canonical_signature(sig_inst_is_input(sig), sig_inst_is_var(sig), mdecl, num_actuals, result));
}

/**
 * Tries to resolve canonical signature set from a Signature mult
 * @param sig Signature AST
 * @return Canonical signature set 
 */
static CanonicalSignatureSet *from_mult_sig(Signature sig)
{
  return union_canonical_signature_set(from_sig(mult_sig_sig1(sig)), from_sig(mult_sig_sig2(sig)));
}

/**
 * Tries to resolve canonical signature set from a Signature use
 * @param sig Signature AST
 * @return Canonical signature set 
 */
static CanonicalSignatureSet *from_sig_use(Signature sig)
{
  Use use = sig_use_use(sig);
  switch (Use_KEY(use))
  {
  case KEYuse:
    return from_declaration(USE_DECL(use));
  case KEYqual_use:
    return from_type(qual_use_from(use));
  }
}

/**
 * Create canonical signature set from a Signature
 * @param sig Signature AST node
 * @return canonical signature set 
 */
static CanonicalSignatureSet *from_sig(Signature sig)
{
  // print_Signature(sig, stdout);
  // printf("\n");

  switch (Signature_KEY(sig))
  {
  case KEYsig_inst:
    return from_sig_inst(sig);
  case KEYmult_sig:
    return from_mult_sig(sig);
  case KEYsig_use:
    return from_sig_use(sig);
  case KEYno_sig:
  case KEYfixed_sig:
    return EMPTY_CANONICAL_SIGNATURE_SET;
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

  return hash_cons_get(result, my_size, &canonical_signature_set_table);
}

/**
* Compares two canonical signatures using line number of source class
* @param sig1 Canonical signature
* @param sig2 Canonical signature
* @return 0, -1, 1 indicating the result of comparison
*/
static int canonical_signature_compare(CanonicalSignature *sig1, CanonicalSignature *sig2)
{
  int v1 = tnode_line_number(sig1->source_class);
  int v2 = tnode_line_number(sig2->source_class);

  return v1 == v2 ? 0 : (v1 < v2 ? -1 : 1);
}

/**
 * Function that merges two sorted canonical signature lists
 * @param set1 canonical signature set A
 * @param set1 canonical signature set B
 * @return resuling canonical signature result
 */
static CanonicalSignatureSet *union_canonical_signature_set(CanonicalSignatureSet *set1, CanonicalSignatureSet *set2)
{
  if (set1->size == 0 && set2->size == 0)
  {
    return EMPTY_CANONICAL_SIGNATURE_SET;
  }
  else if (set1->size == 0)
  {
    return set2;
  }
  else if (set2->size == 0)
  {
    return set1;
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

    return hash_cons_get(result, my_size, &canonical_signature_set_table);
  }
}

/**
 * Canonical signature set from Type AST
 * @param t Type AST
 * @return canonical signature set 
 */
static CanonicalSignatureSet *from_type(Type t)
{
  // printf("Type\n");
  // print_Type(t, stdout);
  // printf("\n");

  switch (Type_KEY(t))
  {
  case KEYtype_inst:
  {
    Module m = type_inst_module(t);
    Declaration mdecl = USE_DECL(module_use_use(m));
    int num_actuals = count_actuals(type_inst_type_actuals(t));
    size_t my_size = num_actuals * (sizeof(CanonicalSignature *));
    CanonicalType **result = (CanonicalType *)alloca(my_size);

    // Declaration rdecl = module_decl_result_type(mdecl);
    // switch (Declaration_KEY(rdecl))
    // {
    // case KEYsome_type_decl:
    //   {
    //     Type nested = some_type_decl_type(rdecl);
    //     if (Type_KEY(nested) != KEYno_type) {
    //       return from_type(some_type_decl_type(rdecl));
    //     }
    //   }
    // }

    int i = 0;
    Type type = first_TypeActual(type_inst_type_actuals(t));
    while (type != NULL)
    {
      result[i++] = canonical_type_base_type(canonical_type(type));
      type = TYPE_NEXT(type);
    }

    return new_canonical_signature_set(new_canonical_signature(true, true, mdecl, num_actuals, result));
  }
  case KEYno_type:
    // Either TYPE[] or PHYLUM[]
    // type_is_phylum(t) ? type_PHYLUM : module_PHYLUM
    return new_canonical_signature_set(new_canonical_signature(true, true, type_PHYLUM, 0, NULL));
  default:
    aps_error(t, "Not sure how to find the canonical signature set given Type with Type_KEY of %d", (int)Type_KEY(t));
    return NULL;
  }
}

/**
 * Resolve a canonical signature from a Declaration
 * @param decl Declaration
 * @return Canonical signature set 
 */
static CanonicalSignatureSet *from_declaration(Declaration decl)
{
  // printf("decl: %s\n", decl_name(decl));

  switch (Declaration_KEY(decl))
  {
  case KEYsome_type_decl:
  {
    Signature sig = some_type_decl_sig(decl);
    if (Signature_KEY(sig) == KEYno_sig)
    {
      Type t = some_type_decl_type(decl);
      return from_type(t);
    }
    else
    {
      return from_sig(sig);
    }
  }
  case KEYsome_type_formal:
  {
    Signature sig = some_type_formal_sig(decl);
    return from_sig(sig);
  }
  default:
    aps_error(decl, "Not sure how to find the canonical signature set given Declaration with Declaration_KEY of %d", (int)Declaration_KEY(decl));
    return NULL;
  }
}

/**
 * Given a canonical signature set and source canonical type, it would go through each and substitute
 * @param source source canonical type
 * @param sig_set canonical signature set
 * @return substituted canonical signature set
 */
static CanonicalSignatureSet *substitute_canonical_signature_set_actuals(CanonicalType *source, CanonicalSignatureSet *sig_set)
{
  size_t my_size = sizeof(struct CanonicalSignatureSet_type) + sig_set->size * (sizeof(CanonicalSignature *));
  struct CanonicalSignatureSet_type *result = (struct CanonicalSignatureSet_type *)alloca(my_size);
  result->size = sig_set->size;

  int i;
  for (i = 0; i < sig_set->size; i++)
  {
    CanonicalSignature *canonical_sig = sig_set->members[i];

    int num_actuals = canonical_sig->num_actuals;
    size_t actuals_size = num_actuals * sizeof(CanonicalType *);
    CanonicalType **joined_actuals = (CanonicalType **)alloca(actuals_size);
    int j;
    for (j = 0; j < num_actuals; j++)
    {
      joined_actuals[j] = canonical_type_join(source, canonical_sig->actuals[j], true);
    }

    result->members[i] = new_canonical_signature(canonical_sig->is_input, canonical_sig->is_var, canonical_sig->source_class, canonical_sig->num_actuals, joined_actuals);
  }

  return hash_cons_get(result, my_size, &canonical_signature_set_table);
}

/**
 * Should accumulate the signatures in a restrictive way not additive mannger
 * @param ctype canonical type
 * @return canonical signature set
 */
CanonicalSignatureSet *infer_canonical_signatures(CanonicalType *ctype)
{
  if (!initialized)
  {
    fatal_error("canonical signature set is not initialized");
  }

  CanonicalSignatureSet *result = EMPTY_CANONICAL_SIGNATURE_SET;
  bool flag = true;

  if (ctype == NULL)
  {
    return EMPTY_CANONICAL_SIGNATURE_SET;
  }

  // print_canonical_type(ctype, stdout);
  // printf("done\n");

  do
  {
    switch (ctype->key)
    {
    case KEY_CANONICAL_USE:
    {
      struct Canonical_use *canonical_use_type = (struct Canonical_use *)ctype;

      Declaration decl = canonical_use_type->decl;
      result = union_canonical_signature_set(result, from_declaration(decl));
      break;
    }
    case KEY_CANONICAL_QUAL:
    {
      struct Canonical_qual_type *canonical_qual_type = (struct Canonical_qual_type *)ctype;

      Declaration decl = canonical_qual_type->decl;
      result = union_canonical_signature_set(result, substitute_canonical_signature_set_actuals(canonical_qual_type->source, from_declaration(decl)));
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

/**
 * Initializes the necessary stuff needed to find the canonical signatures
 * @param module_PHYLUM module decl for PHYLUM[]
 * @param type_PHYLUM type decl for TYPE[]
 */
void initialize_canonical_signature(Declaration module_PHYLUM_decl, Declaration type_PHYLUM_decl)
{
  module_PHYLUM = module_PHYLUM_decl;
  type_PHYLUM = type_PHYLUM_decl;

  size_t empty_set_size = sizeof(struct CanonicalSignature_type);
  CanonicalSignatureSet *empty_set = alloca(empty_set_size);
  empty_set->size = 0;
  EMPTY_CANONICAL_SIGNATURE_SET = hash_cons_get(empty_set, empty_set_size, &canonical_signature_set_table);

  initialized = true;
}