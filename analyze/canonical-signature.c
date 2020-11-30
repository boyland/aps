#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

#define KEY_RESTRICTIVE 0
#define KEY_ADDITIVE 1
#define ACCUMULATION_METHOD KEY_RESTRICTIVE

int hash_source_class(Declaration cl);

static CanonicalSignatureSet *from_sig(Signature sig);
static CanonicalSignatureSet *from_type(Type t);
static CanonicalSignatureSet *from_declaration(Declaration decl);
static CanonicalSignatureSet *union_canonical_signature_set(CanonicalSignatureSet *set1, CanonicalSignatureSet *set2);
static CanonicalSignatureSet *single_canonical_signature_set(CanonicalSignature *cSig);
static CanonicalSignatureSet *substitute_canonical_signature_set_actuals(CanonicalType *source, CanonicalSignatureSet *sig_set);

static CanonicalSignatureSet *EMPTY_CANONICAL_SIGNATURE_SET;
static Declaration module_TYPE;
static Declaration module_PHYLUM;
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
  int h = 7;
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

  bool actuals_equal = true;
  int i;
  for (i = 0; i < inferred_sig1->num_actuals && actuals_equal; i++)
  {
    actuals_equal &= (inferred_sig1->actuals[i] == inferred_sig2->actuals[i]);
  }

  return actuals_equal && inferred_sig1->is_input == inferred_sig2->is_input && inferred_sig1->is_var == inferred_sig2->is_var && inferred_sig1->source_class == inferred_sig2->source_class;
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
 * Constructor to create a new canonical signature set
 * @param size
 * @param members
 * @return canonical signature set
 */
CanonicalSignatureSet *new_canonical_signature_set(int size, CanonicalSignature **members)
{
  size_t my_size = sizeof(struct CanonicalSignatureSet_type) + size * (sizeof(CanonicalSignature *));

  struct CanonicalSignatureSet_type *result = (struct CanonicalSignatureSet *)alloca(my_size);
  result->size = size;

  int i;
  for (i = 0; i < size; i++)
  {
    result->members[i] = members[i];
  }

  void *memory = hash_cons_get(result, my_size, &canonical_signature_set_table);
  return (CanonicalSignatureSet *)memory;
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

static CanonicalSignatureSet *direct_parents_canonical_signature_set(Signature *sig, CanonicalSignatureSet *csig_set)
{
  switch (Signature_KEY(sig))
  {
  case KEYsig_inst:
  {
    Declaration source_decl = USE_DECL(class_use_use(sig_inst_class(sig)));
    CanonicalSignature **members = (struct CanonicalSignatureSet_type *)alloca(csig_set->size * (sizeof(CanonicalSignature *)));

    int i, size = 0;
    for (i = 0; i < csig_set->size; i++)
    {
      if (csig_set->members[i]->source_class == source_decl)
      {
        members[size++] = csig_set->members[i];
      }
    }

    return new_canonical_signature_set(size, members);
  }
  case KEYmult_sig:
    return union_canonical_signature_set(direct_parents_canonical_signature_set(mult_sig_sig1(sig), csig_set), direct_parents_canonical_signature_set(mult_sig_sig2(sig), csig_set));
  case KEYsig_use:
  case KEYno_sig:
  case KEYfixed_sig:
    return EMPTY_CANONICAL_SIGNATURE_SET;
  }
}

static bool is_in_canonical_signature_set(CanonicalSignature *thing, CanonicalSignatureSet *sig_set)
{
  int i;
  for (i = 0; i < sig_set->size; i++)
  {
    if (sig_set->members[i] == thing)
    {
      return true;
    }
  }

  return false;
}

static CanonicalSignatureSet *subtract_canonical_signature_set(CanonicalSignatureSet *csig_set1, CanonicalSignatureSet *csig_set2)
{
  if (csig_set1->size == 0 && csig_set2->size == 0)
  {
    return EMPTY_CANONICAL_SIGNATURE_SET;
  }
  else if (csig_set1->size == 0)
  {
    return EMPTY_CANONICAL_SIGNATURE_SET;
  }
  else if (csig_set2->size == 0)
  {
    return csig_set1;
  }
  else
  {
    size_t my_size = sizeof(struct CanonicalSignatureSet_type) + (csig_set1->size) * (sizeof(CanonicalSignature *));

    struct CanonicalSignatureSet_type *result = (struct CanonicalSignatureSet_type *)alloca(my_size);
    result->size = 0;

    int i;
    for (i = 0; i < csig_set1->size; i++)
    {
      if (!is_in_canonical_signature_set(csig_set1->members[i], csig_set2))
      {
        result->members[result->size++] = csig_set1->members[i];
      }
    }

    return new_canonical_signature_set(result->size, result->members);
  }
}

struct ForwardActualsResult_type
{
  CanonicalSignatureSet *changed_csig_set;
  CanonicalSignatureSet *substituted_csig_set;
};

typedef struct ForwardActualsResult_type ForwardActualsResult;

// Given a canonical signature and a canonical signature set
// 1) loops through each canonical signature member
// 2) replace the actual
// 3) return a tuple of affected subset and substituted canonical signatures
ForwardActualsResult forward_actuals(CanonicalSignature *source_csig, CanonicalSignatureSet *csig_set)
{
  // printf("  @@source_csig: ");
  // print_canonical_signature(source_csig, stdout);
  // printf("\n");
  Declaration mdecl = source_csig->source_class;
  Declaration rdecl = some_class_decl_result_type(mdecl);

  CanonicalSignatureSet *changed_subset = EMPTY_CANONICAL_SIGNATURE_SET;
  CanonicalSignatureSet *substituted_subset = EMPTY_CANONICAL_SIGNATURE_SET;

  int i, j, k;
  for (i = 0; i < csig_set->size; i++)
  {
    CanonicalSignature *canonical_sig = csig_set->members[i];
    CanonicalType **substituted_actuals = (CanonicalType **)alloca(canonical_sig->num_actuals);
    bool any_change = canonical_sig->num_actuals == 0;

    for (j = 0; j < canonical_sig->num_actuals; j++)
    {
      substituted_actuals[j] = canonical_sig->actuals[j];
      Declaration f1 = canonical_type_decl(canonical_sig->actuals[j]);

      k = 0;
      Declaration f2 = first_Declaration(some_class_decl_type_formals(mdecl));
      while (f2 != NULL)
      {
        if (f1 == f2)
        {
          substituted_actuals[j] = source_csig->actuals[k];
          any_change = true;
          // printf("f2[%s] <-- f1[%s] in ", decl_name(f2), decl_name(f1));
          // print_canonical_signature(canonical_sig, stdout);
          // printf(" thing: ");
          // print_canonical_type(source_csig->actuals[k], stdout);
          // printf("\n");
        }

        k++;
        f2 = DECL_NEXT(f2);
      }
    }

    if (any_change)
    {
      substituted_subset = union_canonical_signature_set(substituted_subset, single_canonical_signature_set(new_canonical_signature(canonical_sig->is_input, canonical_sig->is_var, canonical_sig->source_class, canonical_sig->num_actuals, substituted_actuals)));
      changed_subset = union_canonical_signature_set(changed_subset, single_canonical_signature_set(canonical_sig));
    }
  }

  // printf("   $$ substituted_subset: ");
  // print_canonical_signature_set(substituted_subset, stdout);
  // printf("\n   $$ changed_subset: ");
  // print_canonical_signature_set(changed_subset, stdout);
  // printf("\n");

  ForwardActualsResult result = {changed_subset, substituted_subset};

  return result;
}

// Collects parents and result canonical signatures
static CanonicalSignatureSet *from_mdecl(CanonicalSignature *csig)
{
  Declaration mdecl = csig->source_class;
  Signature parent_sig = some_class_decl_parent(mdecl);
  Declaration rdecl = some_class_decl_result_type(mdecl);

  if (ACCUMULATION_METHOD == KEY_ADDITIVE)
  {
    return union_canonical_signature_set(from_declaration(rdecl), from_sig(parent_sig));
  }

  return EMPTY_CANONICAL_SIGNATURE_SET;
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
  CanonicalType **cactuals = (CanonicalType **)alloca(my_size);

  int i = 0;
  Type atype = first_TypeActual(actuals);

  while (atype != NULL)
  {
    cactuals[i] = canonical_type_base_type(canonical_type(atype));
    atype = TYPE_NEXT(atype);
    i++;
  }

  CanonicalSignature *csig = new_canonical_signature(sig_inst_is_input(sig), sig_inst_is_var(sig), mdecl, num_actuals, cactuals);

  return union_canonical_signature_set(from_mdecl(csig), single_canonical_signature_set(csig));
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
static CanonicalSignatureSet *single_canonical_signature_set(CanonicalSignature *cSig)
{
  return new_canonical_signature_set(1, &cSig);
}

/**
 * Comparator for two canonical signature
 * @param sig1 first canonical signature
 * @param sig2 second canonical signature
 * @return integer value representing the comparaison
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

    struct CanonicalSignatureSet_type *result = (struct CanonicalSignatureSet_type *)alloca(my_size);
    result->size = set1->size + set2->size;

    int i = 0,
        j = 0, k = 0;
    while (i < set1->size && j < set2->size)
    {
      int comp = canonical_signature_compare(set1->members[i], set2->members[j]);
      if (comp == 0)
      {
        result->members[k] = set1->members[i];
        result->size--;
        i++;
        j++;
      }
      else if (comp < 0)
      {
        result->members[k] = set1->members[i];
        i++;
      }
      else
      {
        result->members[k] = set2->members[j];
        j++;
      }

      k++;
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
  switch (Type_KEY(t))
  {
  case KEYtype_formal:
    return EMPTY_CANONICAL_SIGNATURE_SET;
  case KEYtype_use:
    return infer_canonical_signatures(canonical_type(t));
  case KEYtype_inst:
  {
    Module m = type_inst_module(t);
    Declaration mdecl = USE_DECL(module_use_use(m));
    int num_actuals = count_actuals(type_inst_type_actuals(t));
    size_t my_size = num_actuals * (sizeof(CanonicalSignature *));
    CanonicalType **cactuals = (CanonicalType *)alloca(my_size);

    int i = 0;
    Type type = first_TypeActual(type_inst_type_actuals(t));
    while (type != NULL)
    {
      cactuals[i++] = canonical_type_base_type(canonical_type(type));
      // printf("  > actual: ");
      // print_canonical_type(cactuals[i - 1], stdout);
      // printf("\n");
      type = TYPE_NEXT(type);
    }

    CanonicalSignature *csig = new_canonical_signature(true, true, mdecl, num_actuals, cactuals);
    return union_canonical_signature_set(from_mdecl(csig), single_canonical_signature_set(csig));
  }
  case KEYno_type:
    // Either TYPE[] or PHYLUM[]
    // type_is_phylum(t) ? type_PHYLUM : module_PHYLUM
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
static CanonicalSignatureSet *from_declaration(Declaration decl)
{
  CanonicalSignatureSet *re;
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
      re = from_sig(sig);
    }

    switch (Type_KEY(some_type_decl_type(decl)))
    {
    case KEYtype_inst:
    {
      re = substitute_canonical_signature_set_actuals(new_canonical_type_use(decl), re);
      break;
    }
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

/**
 * Given a canonical signature set and source canonical type, it would go through each and substitute
 * @param source source canonical type
 * @param sig_set canonical signature set
 * @return substituted canonical signature set
 */
static CanonicalSignatureSet *substitute_canonical_signature_set_actuals(CanonicalType *source_ctype, CanonicalSignatureSet *sig_set)
{
  // printf("source_ctype: ");
  // print_canonical_type(source_ctype, stdout);
  // printf("\nsig_set: ");
  // print_canonical_signature_set(sig_set, stdout);
  // printf("\n");

  CanonicalSignatureSet *changed_subset = EMPTY_CANONICAL_SIGNATURE_SET;
  CanonicalSignatureSet *substituted_subset = EMPTY_CANONICAL_SIGNATURE_SET;
  CanonicalSignatureSet *unchanged_subset = sig_set;

  int i, j;
  bool any_change;
  Declaration source_decl = canonical_type_decl(source_ctype);
  switch (Declaration_KEY(source_decl))
  {
  case KEYsome_type_decl:
  {
    Type source_decl_type = some_type_decl_type(source_decl);
    switch (Type_KEY(source_decl_type))
    {
    case KEYtype_inst:
    {
      Declaration mdecl = USE_DECL(module_use_use(type_inst_module(source_decl_type)));

      for (i = 0; i < sig_set->size; i++)
      {
        CanonicalSignature *csig_member = sig_set->members[i];

        if (mdecl == csig_member->source_class)
        {
          CanonicalSignature *root_csig = csig_member;

          // No need to run the substitution on the root of current canonical signature set
          changed_subset = single_canonical_signature_set(root_csig);
          substituted_subset = single_canonical_signature_set(root_csig);
          unchanged_subset = subtract_canonical_signature_set(sig_set, changed_subset);
          // printf("  found root_csig! ctype: ");
          // print_canonical_type(source_ctype, stdout);
          // printf(" sig: ");
          // print_canonical_signature(root_csig, stdout);
          // printf("\n");

          break;
        }
      }
    }
    }
  }
  }

  CanonicalSignatureSet *unchanged_subset_clone = unchanged_subset;

  for (i = 0; i < unchanged_subset_clone->size; i++)
  {
    CanonicalSignature *canonical_sig = unchanged_subset_clone->members[i];

    int num_actuals = canonical_sig->num_actuals;
    size_t actuals_size = num_actuals * sizeof(CanonicalType *);
    CanonicalType **substituted_actuals = (CanonicalType **)alloca(actuals_size);
    any_change = num_actuals == 0;

    for (j = 0; j < num_actuals; j++)
    {
      CanonicalType *cactual = canonical_type_join(source_ctype, canonical_sig->actuals[j], true);

      if (cactual != canonical_sig->actuals[j])
      {
        any_change = true;
      }

      substituted_actuals[j] = cactual;
    }

    if (any_change)
    {
      substituted_subset = union_canonical_signature_set(substituted_subset, single_canonical_signature_set(new_canonical_signature(canonical_sig->is_input, canonical_sig->is_var, canonical_sig->source_class, canonical_sig->num_actuals, substituted_actuals)));
      changed_subset = union_canonical_signature_set(changed_subset, single_canonical_signature_set(canonical_sig));
      unchanged_subset = subtract_canonical_signature_set(sig_set, changed_subset);
    }
  }

  // printf("\n  ** substituted_subset: ");
  // print_canonical_signature_set(substituted_subset, stdout);
  // printf("\n  ** changed_subset: ");
  // print_canonical_signature_set(changed_subset, stdout);
  // printf("\n  ** unchanged_subset: ");
  // print_canonical_signature_set(unchanged_subset, stdout);
  // printf("\n");

  any_change = !(substituted_subset->size > 0 && unchanged_subset->size == 0);

  while (any_change && unchanged_subset->size > 0)
  {
    // printf("  >any_change: %s\tunchanged_subset count: %d\n", any_change ? "true" : "false", unchanged_subset->size);
    any_change = false;
    for (i = 0; i < substituted_subset->size; i++)
    {
      ForwardActualsResult partial_result = forward_actuals(substituted_subset->members[i], unchanged_subset);
      CanonicalSignatureSet *temp = subtract_canonical_signature_set(unchanged_subset, partial_result.changed_csig_set);

      any_change |= (temp->size != unchanged_subset->size);
      unchanged_subset = temp;

      substituted_subset = union_canonical_signature_set(substituted_subset, partial_result.substituted_csig_set);
      // printf("    >>any_change: %s\n", any_change ? "true" : "false");
    }
  }

  CanonicalSignatureSet *final = union_canonical_signature_set(substituted_subset, unchanged_subset);

  // printf("    final result: ");
  // print_canonical_signature_set(final, stdout);
  // printf("\n\n");

  return final;
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

  // printf("infer_canonical_signatures ctype before: ");
  // print_canonical_type(ctype, stdout);
  // printf("\n");

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

    // printf("infer_canonical_signatures ctype after: ");
    // print_canonical_type(ctype, stdout);
    // printf("\n");

  } while (flag);

  return new_canonical_signature_set(result->size, result->members);
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

  EMPTY_CANONICAL_SIGNATURE_SET = new_canonical_signature_set(0, NULL);

  initialized = true;
}

Declaration get_module_TYPE() { return module_TYPE; }
Declaration get_module_PHYLUM() { return module_PHYLUM; }
