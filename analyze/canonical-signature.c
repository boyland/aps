#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

static CanonicalSignatureSet from_sig(Signature sig);
static CanonicalSignatureSet from_type(Type t);
static CanonicalSignatureSet from_declaration(Declaration decl);
static CanonicalSignatureSet substitute_canonical_signature_set_actuals(CanonicalType* source, CanonicalSignatureSet sig_set);
static int canonical_signature_compare(CanonicalSignature *sig1, CanonicalSignature *sig2);

static bool initialized = false;

/**
 * Prints canonical signature 
 * @param untyped CanonicalSignature
 * @param f output stream
 */
void print_canonical_signature(void *untyped, FILE *f)
{
  if (f == NULL)
  {
    f = stdout;
  }
  if (untyped == NULL)
  {
    fprintf(f, "<null>");
    return;
  }

  CanonicalSignature *canonical_signature = (CanonicalSignature *)untyped;

  fprintf(f, "%s", decl_name(canonical_signature->source_class));
  fputc('[', f);

  bool started = false;
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
  if (f == NULL)
  {
    f = stdout;
  }
  if (untyped == NULL)
  {
    fprintf(f, "{}");
    return;
  }

  CanonicalSignatureSet set = (CanonicalSignatureSet)untyped;

  fprintf(f, "{");

  if (set->num_elements > 0)
  {
    size_t struct_size = sizeof(struct hash_cons_set) + set->num_elements * sizeof(void *);
    HASH_CONS_SET sorted_set = (HASH_CONS_SET)alloca(struct_size);
    sorted_set->num_elements = 0;

    int i, j;
    for (i = 0; i < set->num_elements; i++)
    {
      CanonicalSignature * item = (CanonicalSignature *)set->elements[i];
      int j = i - 1;

      while (j >= 0 && canonical_signature_compare((CanonicalSignature *)sorted_set->elements[j], item) > 0)
      { 
        sorted_set->elements[j + 1] = sorted_set->elements[j]; 
        j--; 
      }

      sorted_set->elements[j + 1] = item;
      sorted_set->num_elements++;
    }

    int started = false;
    for (i = 0; i < sorted_set->num_elements; i++)
    {
      if (started)
      {
        fprintf(f, ",");
      }
      else
      {
        started = true;
      }
      print_canonical_signature(sorted_set->elements[i], f);
    }
  }

  fprintf(f, "}");
}

/**
 * Hashes list of CanonicalType
 * @param count
 * @param types
 * @return combined hash 
 */
long hash_canonical_types(int count, CanonicalType **types)
{
  long h = 17;
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
long hash_source_class(Declaration cl)
{
  return (long)cl;
}

/**
 * Hashes CanonicalSignature
 * @param untyped InferredSignature
 * @return hash integer value
 */
long canonical_signature_hash(void *untyped)
{
  CanonicalSignature *inferred_sig = (CanonicalSignature *)untyped;

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
  CanonicalSignature *inferred_sig1 = (CanonicalSignature *)untyped1;
  CanonicalSignature *inferred_sig2 = (CanonicalSignature *)untyped2;

  if (inferred_sig1->num_actuals != inferred_sig2->num_actuals)
  {
    return false;
  }

  bool actuals_equal = true;
  int i;
  for (i = 0; i < inferred_sig1->num_actuals && actuals_equal; i++)
  {
    actuals_equal &= (inferred_sig1->actuals[i] == inferred_sig2->actuals[i]);
  }

  return actuals_equal && inferred_sig1->is_input == inferred_sig2->is_input && inferred_sig1->is_var == inferred_sig2->is_var && inferred_sig1->source_class == inferred_sig2->source_class;
}

static struct hash_cons_table canonical_signature_table = {canonical_signature_hash, canonical_signature_equal};

/**
 * Constructor to create a new canonical signature
 * @param is_input
 * @param is_var
 * @param source_class
 * @param num_actuals
 * @param actuals
 * @return canonical signature
 */
CanonicalSignature* new_canonical_signature(bool is_input, bool is_var, Declaration source_class, int num_actuals, CanonicalType **actuals)
{
  size_t struct_size = sizeof(CanonicalSignature) + num_actuals * (sizeof(CanonicalType *));

  CanonicalSignature *result = (CanonicalSignature *)alloca(struct_size);
  result->is_input = is_input;
  result->is_var = is_var;
  result->source_class = source_class;
  result->num_actuals = num_actuals;

  int i;
  for (i = 0; i < num_actuals; i++)
  {
    result->actuals[i] = actuals[i];
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

// Collects parents and result canonical signatures
static CanonicalSignatureSet from_ctype(CanonicalSignature* csig)
{
  Declaration mdecl = csig->source_class;
  Signature parent_sig = some_class_decl_parent(mdecl);
  Declaration rdecl = some_class_decl_result_type(mdecl);

  return from_declaration(rdecl);
}

/**
 * Tries to resolve canonical signature set from a Signature inst
 * @param sig Signature AST
 * @return Canonical signature set 
 */
static CanonicalSignatureSet from_sig_inst(Signature sig)
{
  Declaration mdecl = USE_DECL(class_use_use(sig_inst_class(sig)));

  TypeActuals actuals = sig_inst_actuals(sig);
  int num_actuals = count_actuals(actuals);
  size_t my_size = num_actuals * sizeof(CanonicalType *);
  CanonicalType **cactuals = (CanonicalType **)alloca(my_size);

  int i = 0;
  Type atype = first_TypeActual(actuals);

  while (atype != NULL)
  {
    cactuals[i] = canonical_type_base_type(canonical_type(atype));
    atype = TYPE_NEXT(atype);
    i++;
  }

  CanonicalSignature* csig = new_canonical_signature(sig_inst_is_input(sig), sig_inst_is_var(sig), mdecl, num_actuals, cactuals);

  return add_hash_cons_set(csig, from_ctype(csig));
}

/**
 * Tries to resolve canonical signature set from a Signature mult
 * @param sig Signature AST
 * @return Canonical signature set 
 */
static CanonicalSignatureSet from_mult_sig(Signature sig)
{
  return union_hash_const_set(from_sig(mult_sig_sig1(sig)), from_sig(mult_sig_sig2(sig)));
}

/**
 * Tries to resolve canonical signature set from a Signature use
 * @param sig Signature AST
 * @return Canonical signature set 
 */
static CanonicalSignatureSet from_sig_use(Signature sig)
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
static CanonicalSignatureSet from_sig(Signature sig)
{
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
    return get_hash_cons_empty_set();
  }
}

/**
 * Returns a single canonical signature set
 * @param csig canonical signature
 * @return canonical signature set 
 */
static CanonicalSignatureSet single_canonical_signature_set(CanonicalSignature* csig)
{
  size_t sig_set_size = sizeof(struct hash_cons_set) + 1 * sizeof(CanonicalSignature *);
  CanonicalSignatureSet sig_set = (CanonicalSignatureSet)alloca(sig_set_size);
  sig_set->num_elements = 1;
  sig_set->elements[0] = csig;

  return new_hash_cons_set(sig_set);
}

/**
 * Comparator for two canonical signature
 * @param sig1 first canonical signature
 * @param sig2 second canonical signature
 * @return integer value representing the comparison
 */
static int canonical_signature_compare(CanonicalSignature *sig1, CanonicalSignature *sig2)
{
  if (sig1->is_input != sig2->is_input)
  {
    return sig1->is_input - sig2->is_input;
  }

  if (sig1->is_var != sig2->is_var)
  {
    return sig1->is_var - sig2->is_var;
  }

  if (tnode_line_number(sig1->source_class) != tnode_line_number(sig2->source_class))
  {
    return tnode_line_number(sig1->source_class) - tnode_line_number(sig2->source_class);
  }

  if (sig1->num_actuals != sig2->num_actuals)
  {
    return sig1->num_actuals - sig2->num_actuals;
  }

  int i;
  for (i = 0; i < sig1->num_actuals; i++)
  {
    int actual_comp = canonical_type_compare(sig1->actuals[i], sig2->actuals[i]);
    if (actual_comp != 0)
    {
      return actual_comp;
    }
  }

  return 0;
}

/**
 * Canonical signature set from Type AST
 * @param t Type AST
 * @return canonical signature set 
 */
static CanonicalSignatureSet from_type(Type t)
{
  switch (Type_KEY(t))
  {
  case KEYtype_use:
    return infer_canonical_signatures(canonical_type(t));
  case KEYtype_inst:
  {
    Module m = type_inst_module(t);
    Declaration mdecl = USE_DECL(module_use_use(m));
    int num_actuals = count_actuals(type_inst_type_actuals(t));
    size_t my_size = num_actuals * (sizeof(CanonicalSignature *));
    CanonicalType **cactuals = (CanonicalType **)alloca(my_size);

    int i = 0;
    Type type = first_TypeActual(type_inst_type_actuals(t));
    while (type != NULL)
    {
      cactuals[i++] = canonical_type_base_type(canonical_type(type));
      type = TYPE_NEXT(type);
    }

    CanonicalSignature* csig = new_canonical_signature(true, true, mdecl, num_actuals, cactuals);
    return add_hash_cons_set(csig, from_ctype(csig));
  }
  case KEYprivate_type:
  case KEYno_type:
    return single_canonical_signature_set(new_canonical_signature(true, true, type_is_phylum(t) ? module_PHYLUM : module_TYPE, 0, NULL));
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
static CanonicalSignatureSet from_declaration(Declaration decl)
{
  CanonicalSignatureSet re;
  switch (Declaration_KEY(decl))
  {
  case KEYsome_type_decl:
  {
    Signature sig = some_type_decl_sig(decl);
    if (Signature_KEY(sig) == KEYno_sig)
    {
      Type t = some_type_decl_type(decl);
      re = from_type(t);
    }
    else
    {
      re = union_hash_const_set(from_sig(sig), from_type(some_type_decl_type(decl)));
    }

    switch (Type_KEY(some_type_decl_type(decl)))
    {
    case KEYtype_inst:
    {
      re = substitute_canonical_signature_set_actuals(new_canonical_type_use(decl), re);
      break;
    }
    default:
      break;
    }
    break;
  }
  case KEYsome_type_formal:
  {
    Signature sig = some_type_formal_sig(decl);
    re = from_sig(sig);
    break;
  }
  default:
    aps_error(decl, "Not sure how to find the canonical signature set given Declaration with Declaration_KEY of %d", (int)Declaration_KEY(decl));
    return NULL;
  }

  return re;
}

static CanonicalSignature* join_canonical_signature_actuals(CanonicalType *source_ctype, CanonicalSignature *canonical_sig)
{
  switch (source_ctype->key)
  {
  case KEY_CANONICAL_FUNC:
  {
    aps_warning(source_ctype, "Not sure how to run substitution of actuals given function types");
    return canonical_sig;
  }
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *ctype_qual = (struct Canonical_qual_type *)source_ctype;
    return join_canonical_signature_actuals(ctype_qual->source, join_canonical_signature_actuals(new_canonical_type_use(ctype_qual->decl), canonical_sig));
  }
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use_type *ctype_use = (struct Canonical_use_type *)source_ctype;

    Declaration tdecl = ctype_use->decl;
    Declaration mdecl = USE_DECL(module_use_use(type_inst_module(some_type_decl_type(tdecl))));

    CanonicalType **substituted_actuals = (CanonicalType **)alloca(canonical_sig->num_actuals);

    int i, j, k;
    for (i = 0; i < canonical_sig->num_actuals; i++)
    {
      substituted_actuals[i] = canonical_type_join(source_ctype, canonical_sig->actuals[i], false);

      Declaration f1 = canonical_type_decl(canonical_sig->actuals[i]);

      j = 0;
      Declaration f2 = first_Declaration(some_class_decl_type_formals(mdecl));
      while (f2 != NULL)
      {
        if (f1 == f2)
        {
          k = 0;
          Type ta = first_TypeActual(type_inst_type_actuals(some_type_decl_type(tdecl)));
          while (ta != NULL)
          {
            if (j == k)
            {
              substituted_actuals[i] = canonical_type_base_type(canonical_type(ta));
            }

            k++;
            ta = TYPE_NEXT(ta);
          }
        }

        j++;
        f2 = DECL_NEXT(f2);
      }
    }

    return new_canonical_signature(canonical_sig->is_input, canonical_sig->is_var, canonical_sig->source_class, canonical_sig->num_actuals, substituted_actuals);
  }
  default:
    fatal_error("Unexpected source canonical type with key of %d", source_ctype->key);
    return NULL;
  }
}

static CanonicalSignatureSet join_canonical_signature_set_actuals(CanonicalType* source_ctype, CanonicalSignatureSet sig_set)
{
  CanonicalSignatureSet result = get_hash_cons_empty_set();

  int i;
  for (i = 0; i < sig_set->num_elements; i++)
  {
    result = add_hash_cons_set(join_canonical_signature_actuals(source_ctype, sig_set->elements[i]), result);
  }

  return result;
}

static CanonicalSignature *substitute_canonical_signature_actuals(CanonicalType *source_ctype, CanonicalSignature *canonical_sig)
{
  switch (source_ctype->key)
  {
  case KEY_CANONICAL_FUNC:
  {
    aps_warning(source_ctype, "Not sure how to run substitution of actuals given a function type");
    return canonical_sig;
  }
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *ctype_qual = (struct Canonical_qual_type *)source_ctype;
    return join_canonical_signature_actuals(ctype_qual->source, substitute_canonical_signature_actuals(new_canonical_type_use(ctype_qual->decl), canonical_sig));
  }
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use_type *ctype_use = (struct Canonical_use_type *)source_ctype;

    Declaration tdecl = ctype_use->decl;
    Declaration mdecl = USE_DECL(module_use_use(type_inst_module(some_type_decl_type(tdecl))));

    CanonicalType **substituted_actuals = (CanonicalType **)alloca(canonical_sig->num_actuals);

    int i, j, k;
    for (i = 0; i < canonical_sig->num_actuals; i++)
    {
      substituted_actuals[i] = canonical_sig->actuals[i];

      Declaration f1 = canonical_type_decl(canonical_sig->actuals[i]);

      j = 0;
      Declaration f2 = first_Declaration(some_class_decl_type_formals(mdecl));
      while (f2 != NULL)
      {
        if (f1 == f2)
        {
          k = 0;
          Type ta = first_TypeActual(type_inst_type_actuals(some_type_decl_type(tdecl)));
          while (ta != NULL)
          {
            if (j == k)
            {
              substituted_actuals[i] = canonical_type_base_type(canonical_type(ta));
            }

            k++;
            ta = TYPE_NEXT(ta);
          }
        }

        j++;
        f2 = DECL_NEXT(f2);
      }
    }

    return new_canonical_signature(canonical_sig->is_input, canonical_sig->is_var, canonical_sig->source_class, canonical_sig->num_actuals, substituted_actuals);
  }
  default:
    fatal_error("Unexpected source canonical type with key of %d", source_ctype->key);
    return NULL;
  }
}

/**
 * Given a canonical signature set and source canonical type, it would go through each and substitute
 * @param source source canonical type
 * @param sig_set canonical signature set
 * @return substituted canonical signature set
 */
static CanonicalSignatureSet substitute_canonical_signature_set_actuals(CanonicalType *source_ctype, CanonicalSignatureSet sig_set)
{
  CanonicalSignatureSet result = get_hash_cons_empty_set();

  int i;
  for (i = 0; i < sig_set->num_elements; i++)
  {
    result = add_hash_cons_set(substitute_canonical_signature_actuals(source_ctype, sig_set->elements[i]), result);
  }

  return result;
}

/**
 * Should accumulate the signatures in a restrictive way not additive manner
 * @param ctype canonical type
 * @return canonical signature set
 */
CanonicalSignatureSet infer_canonical_signatures(CanonicalType *ctype)
{
  if (!initialized)
  {
    fatal_error("canonical signature set is not initialized");
  }

  CanonicalSignatureSet result = get_hash_cons_empty_set();
  bool flag = true;

  if (ctype == NULL)
  {
    return result;
  }

  do
  {
    switch (ctype->key)
    {
    case KEY_CANONICAL_USE:
    {
      struct Canonical_use_type *canonical_use_type = (struct Canonical_use_type *)ctype;

      Declaration decl = canonical_use_type->decl;
      result = union_hash_const_set(result, from_declaration(decl));
      break;
    }
    case KEY_CANONICAL_QUAL:
    {
      struct Canonical_qual_type *canonical_qual_type = (struct Canonical_qual_type *)ctype;

      Declaration decl = canonical_qual_type->decl;
      result = union_hash_const_set(result, join_canonical_signature_set_actuals(canonical_qual_type->source, from_declaration(decl)));
      break;
    }
    case KEY_CANONICAL_FUNC:
    {
      struct Canonical_function_type *canonical_function_type = (struct Canonical_function_type *)ctype;
      flag = false;
      break;
    }
    default:
      break;
    }

    // TODO: lets revisit this 12/14/2020
    CanonicalType *base_type = canonical_type_base_type(ctype);
    flag &= !(base_type == NULL || base_type == ctype);
    ctype = base_type;

  } while (flag);

  return result;
}

/**
 * Initializes the necessary stuff needed to find the canonical signatures
 * @param module_TYPE_decl module decl for PHYLUM[]
 * @param module_PHYLUM_decl type decl for TYPE[]
 */
void initialize_canonical_signature(Declaration module_TYPE_decl, Declaration module_PHYLUM_decl)
{
  module_TYPE = module_TYPE_decl;
  module_PHYLUM = module_PHYLUM_decl;

  initialized = true;
}
