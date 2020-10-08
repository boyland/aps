#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

/**
 * Hash CanonicalType
 * @param arg CanonicalType
 * @return hash value
 */
int canonical_type_hash(void *arg)
{
  if (arg == NULL)
  {
    return 0;
  }

  struct canonicalTypeBase *canonical_type = (struct CanonicalType *)arg;

  switch (canonical_type->key)
  {
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use *canonical_use_type = (struct Canonical_use *)arg;
    return hash_mix(canonical_use_type->key, (int)canonical_use_type->decl);
  }
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *canonical_qual_type = (struct Canonical_qual_type *)arg;
    return hash_mix(canonical_qual_type->key, hash_mix((int)canonical_qual_type->decl, canonical_type_hash(canonical_qual_type->source)));
  }
  case KEY_CANONICAL_FUNC:
  {
    struct Canonical_function_type *canonical_function_type = (struct Canonical_function_type *)arg;

    int index;
    int param_types_hash = 0;
    for (index = 0; index < canonical_function_type->num_formals; index++)
    {
      param_types_hash = hash_mix(param_types_hash, canonical_type_hash(canonical_function_type->param_types[index]));
    }

    return hash_mix(canonical_function_type->key, hash_mix(canonical_function_type->num_formals, hash_mix(param_types_hash, canonical_type_hash(canonical_function_type->return_type))));
  }
  default:
    return 0;
  }
}

/**
 * Equality test for CanonicalType
 * @param a untyped CanonicalType
 * @param b untyped CanonicalType
 * @return boolean indicating the result of equality
 */
bool canonical_type_equal(void *a, void *b)
{
  if (a == NULL || b == NULL)
  {
    return false;
  }

  struct canonicalTypeBase *canonical_type_a = (struct CanonicalType *)a;
  struct canonicalTypeBase *canonical_type_b = (struct CanonicalType *)b;

  if (canonical_type_a->key != canonical_type_b->key)
  {
    return false;
  }

  switch (canonical_type_a->key)
  {
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use *canonical_use_type_a = (struct Canonical_use *)a;
    struct Canonical_use *canonical_use_type_b = (struct Canonical_use *)b;

    return canonical_use_type_a->decl == canonical_use_type_b->decl;
  }
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *canonical_qual_type_a = (struct Canonical_qual_type *)a;
    struct Canonical_qual_type *canonical_qual_type_b = (struct Canonical_qual_type *)b;

    return (canonical_qual_type_a->decl == canonical_qual_type_b->decl) &&
           (canonical_qual_type_a->source == canonical_qual_type_b->source);
  }
  case KEY_CANONICAL_FUNC:
  {
    struct Canonical_function_type *canonical_function_type_a = (struct Canonical_function_type *)a;
    struct Canonical_function_type *canonical_function_type_b = (struct Canonical_function_type *)b;

    if (canonical_function_type_a->num_formals != canonical_function_type_b->num_formals)
    {
      return false;
    }

    bool params_equal = true;
    int index;

    for (index = 0; index < canonical_function_type_a->num_formals; index++)
    {
      params_equal &= canonical_type_equal(canonical_function_type_a->param_types[index], canonical_function_type_b->param_types[index]);
    }

    return params_equal && (canonical_function_type_a->return_type == canonical_function_type_b->return_type);
  }
  default:
    return false;
  }
}

/**
 * Prints canonical type 
 * @param untyped CanonicalType
 * @param f output stream
 */
void print_canonical_type(void *untyped, FILE *f)
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

  struct canonicalTypeBase *canonical_type = (struct CanonicalType *)untyped;

  switch (canonical_type->key)
  {
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use *canonical_use_type = (struct Canonical_use *)canonical_type;

    fprintf(f, "%s", decl_name(canonical_use_type->decl));
    break;
  }
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *canonical_qual_type = (struct Canonical_qual_type *)canonical_type;

    print_canonical_type(canonical_qual_type->source, f);

    fprintf(f, "$%s", decl_name(canonical_qual_type->decl));
    break;
  }
  case KEY_CANONICAL_FUNC:
  {
    int started = false;
    struct Canonical_function_type *canonical_func_type = (struct Canonical_function_type *)canonical_type;

    fputc('(', f);
    int i;
    for (i = 0; i < canonical_func_type->num_formals; i++)
    {
      if (started)
      {
        fputc(',', f);
      }
      else
      {
        started = true;
      }
      print_canonical_type(canonical_func_type->param_types[i], f);
    }
    fputc(')', f);
    fprintf(f, "=>");
    print_canonical_type(canonical_func_type->return_type, f);
    break;
  }
  default:
    break;
  }
}

// Hashcons table for CanonicalTypes
static struct hash_cons_table canonical_type_table = {canonical_type_hash, canonical_type_equal};

/**
 * Counts number of Declarations
 */
static int count_declarations(Declarations declarations)
{
  switch (Declarations_KEY(declarations))
  {
  default:
    fatal_error("count_type_actuals crashed");
  case KEYnil_Declarations:
    return 0;
  case KEYlist_Declarations:
    return 1;
  case KEYappend_Declarations:
    return count_declarations(append_Declarations_l1(declarations)) + count_declarations(append_Declarations_l2(declarations));
  }
}

/**
 * Creates an instance of Canonical_use
 * @param decl
 * @return Canonical_use
 */
static CanonicalType *new_canonical_type_use(Declaration decl)
{
  struct Canonical_use ctype_use = {KEY_CANONICAL_USE, decl};
  void *memory = hash_cons_get(&ctype_use, sizeof(ctype_use), &canonical_type_table);
  return (CanonicalType *)memory;
}

/**
 * Creates an instance of Canonical_qual_type
 * @param from
 * @param decl
 * @return Canonical_qual_type
 */
static CanonicalType *new_canonical_type_qual(CanonicalType *from, Declaration decl)
{
  struct Canonical_qual_type ctype_qual = {KEY_CANONICAL_QUAL, decl, from};
  void *memory = hash_cons_get(&ctype_qual, sizeof(ctype_qual), &canonical_type_table);
  return (CanonicalType *)memory;
}

/**
 * Check if node is inside a module
 * @param mdecl
 * @param node
 * @return boolean indicating whether node is inside module or not
 */
static bool is_inside_module(Declaration mdecl, void *node)
{
  void *thing = node;
  while ((thing = tnode_parent(thing)) != NULL)
  {
    if (ABSTRACT_APS_tnode_phylum(thing) == KEYDeclaration && Declaration_KEY((Declaration)thing) == KEYmodule_decl && (Declaration)thing == mdecl)
    {
      return true;
    }
  }

  return false;
}

/**
 * Clone canonical function type
 * @param canonical function type
 * @return cloned canonical function type 
 */
struct Canonical_function_type *shallow_clone_canonical_function_types(struct Canonical_function_type *canonical_function_type)
{
  size_t my_size = sizeof(struct Canonical_function_type) + canonical_function_type->num_formals * (sizeof(CanonicalType *));

  struct Canonical_function_type *result = (struct Canonical_function_type *)malloc(my_size);

  result->key = KEY_CANONICAL_FUNC;
  result->num_formals = canonical_function_type->num_formals;
  result->return_type = canonical_function_type->return_type;

  int i;
  for (i = 0; i < canonical_function_type->num_formals; i++)
  {
    result->param_types[i] = canonical_function_type->param_types[i];
  }

  return result;
}

/**
 * Returns the Declaration member of a CanonicalType
 * @param canonical_type
 */
static Declaration canonical_type_decl(CanonicalType *canonical_type)
{
  if (canonical_type == NULL)
  {
    return NULL;
  }

  switch (canonical_type->key)
  {
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use *canonical_use_type = (struct Canonical_use *)canonical_type;
    return canonical_use_type->decl;
  }
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *canonical_qual_use_type = (struct Canonical_qual_type *)canonical_type;
    return canonical_qual_use_type->decl;
  }
  default:
    aps_error(canonical_type, "Failed to find the decl for CanonicalType");
    return NULL;
  }
}

/**
 * Canonical type given a use
 * @param use any use
 * @return canonical type use
 */
static CanonicalType *canonical_type_use(Use use)
{
  Declaration td = Use_info(use)->use_decl;

  // printf("use: %s %d\n", decl_name(td), (int)Declaration_KEY(td));

  switch (Declaration_KEY(td))
  {
  case KEYsome_type_decl:
  {
    switch (Type_KEY(some_type_decl_type(td)))
    {
    case KEYno_type:
    case KEYtype_inst:
    case KEYprivate_type:
      return new_canonical_type_use(td);
    case KEYtype_use:
      return canonical_type(some_type_decl_type(td));
    case KEYfunction_type:
      return canonical_type(some_type_decl_type(td));
    default:
      fatal_error("Unknown type use_decl type key %d", (int)Type_KEY(some_type_decl_type(td)));
      return NULL;
    }
  }
  case KEYtype_replacement: // XXX need to rethink this
    return canonical_type(type_replacement_as(td));
  case KEYtype_renaming:
    return canonical_type(type_renaming_old(td));
  case KEYtype_formal:
    return new_canonical_type_use(td);
  default:
    aps_error(td, "Not sure how handle this decl type while finding canonical type use");
  }

  return NULL;
}

static Type get_actual_given_formal(Declaration type_inst_decl, Declaration mdecl, Declaration formal)
{
  Declaration f;
  Type actual;

  for (f = first_Declaration(module_decl_type_formals(mdecl)),
      actual = first_TypeActual(type_inst_type_actuals(type_decl_type(type_inst_decl)));
       f != NULL; f = DECL_NEXT(f), actual = TYPE_NEXT(actual))
  {
    // printf("formal: %s\nactual: ", decl_name(f));
    // print_Type(actual, stdout);
    // printf("\n");

    // printf("canonicalized actual: %s\n", decl_name(canonical_type_decl(canonical_type(actual))));

    if (formal == f)
    {
      return actual;
    }
  }

  return NULL;
}

/**
 * Subtutute formals and result types if type is inside a module and is being referenced from outside
 * @param thing current declaration
 * @param mdecl module declaration
 * @param type_inst_decl type_inst_declaration
 * @param fallback fallback canonical type in case
 * @return substituted canonical type
 */
static CanonicalType *canonical_type_helper(Declaration thing, Declarations mdecl, Declaration type_inst_decl, CanonicalType *fallback)
{
  if (thing == NULL)
  {
    return fallback;
  }

  Declaration rdecl = module_decl_result_type(mdecl);

  // When module is not generating and thing is of type of KEYtype_formal, it means we are actually referring to the result of module
  if (!module_decl_generating(mdecl) && canonical_type_decl(canonical_type(some_type_decl_type(rdecl))) == thing)
  {
    return new_canonical_type_use(type_inst_decl);
  }

  // type XXX := Result
  // Get result of module
  if (rdecl == thing)
  {
    return new_canonical_type_use(type_inst_decl);
  }

  // type XXX := formal;
  // Get actual matching position of the formal
  else if (Declaration_KEY(thing) == KEYtype_formal)
  {
    Type actual = get_actual_given_formal(type_inst_decl, mdecl, thing);

    if (actual != NULL)
    {
      return canonical_type(actual);
    }
  }

  return fallback;
}

/**
 * Canonical type given a qual use
 * @param use any use
 * @return canonical type qual use
 */
static CanonicalType *canonical_type_qual_use(Use use)
{
  CanonicalType *from_canonicalized = canonical_type(qual_use_from(use));
  CanonicalType *inside_canonicalized = canonical_type_use(use);

  Declaration type_inst_decl = canonical_type_decl(from_canonicalized);

  Module module = type_inst_module(type_decl_type(type_inst_decl));
  Declaration mdecl = USE_DECL(module_use_use(module));

  if (inside_canonicalized->key != KEY_CANONICAL_FUNC)
  {
    Declaration inside_decl = canonical_type_decl(inside_canonicalized);

    if (!is_inside_module(mdecl, inside_decl))
    {
      return new_canonical_type_use(inside_decl);
    }
    else
    {
      // Continue ...
    }
  }

  Declaration udecl = Use_info(use)->use_decl;
  Type udecl_type = some_type_decl_type(udecl);

  switch (Type_KEY(udecl_type))
  {
  case KEYno_type:
  case KEYtype_inst:
    return new_canonical_type_qual(from_canonicalized, udecl);
  case KEYtype_use:
  {
    Use use = type_use_use(udecl_type);
    CanonicalType *canonical_type_use_nested = canonical_type_use(use);
    Declaration thing = canonical_type_decl(canonical_type_use_nested);

    // call helper function
    return canonical_type_helper(thing, mdecl, type_inst_decl, canonical_type_use_nested);
  }
  case KEYfunction_type:
  {
    struct Canonical_function_type *canonical_function_type = (struct Canonical_function_type *)inside_canonicalized;
    size_t my_size = sizeof(struct Canonical_function_type) + canonical_function_type->num_formals * (sizeof(CanonicalType *));

    struct Canonical_function_type *result = shallow_clone_canonical_function_types(canonical_function_type);

    int i;
    for (i = 0; i < canonical_function_type->num_formals; i++)
    {
      Declaration thing = canonical_type_decl(canonical_function_type->param_types[i]);

      result->param_types[i] = canonical_type_helper(thing, mdecl, type_inst_decl, canonical_function_type->param_types[i]);
    }

    Declaration thing = canonical_type_decl(canonical_function_type->return_type);
    result->return_type = canonical_type_helper(thing, mdecl, type_inst_decl, canonical_function_type->return_type);

    void *memory = hash_cons_get(result, my_size, &canonical_type_table);

    return (CanonicalType *)memory;
  }
  }
}

static bool has_canonical_type_changed(CanonicalType *before, CanonicalType *after)
{
  if (before == NULL && after != NULL)
  {
    return true;
  }
  else if (before != NULL && after == NULL)
  {
    return false;
  }
  else
  {
    return before != after;
  }
}

static Declaration substitute_decl(Declaration decl, Declaration tdecl, Declaration mdecl)
{
  printf("mdecl: %s\n", mdecl == NULL ? "null" : decl_name(mdecl));
  printf("tdecl: %s\n", tdecl == NULL ? "null" : decl_name(tdecl));
  printf("decl: %s\n", decl == NULL ? "null" : decl_name(decl));
  printf("\n");

  switch (Declaration_KEY(decl))
  {
  case KEYsome_type_decl:
  {
    Type tdecl_type = some_type_decl_type(decl);
    switch (Type_KEY(tdecl_type))
    {
    case KEYtype_inst:
    {
      Declaration nested_mdecl = USE_DECL(module_use_use(type_inst_module(tdecl_type)));

      if (module_decl_generating(nested_mdecl))
      {
        return decl;
      }
      else
      {
        Declaration rdecl = module_decl_result_type(nested_mdecl);
        Declaration temp = substitute_decl(rdecl, decl, nested_mdecl);

        return decl == temp ? decl : substitute_decl(temp, tdecl, mdecl);
      }
    }
    case KEYtype_use:
    {
      return substitute_decl(canonical_type_decl(canonical_type(some_type_decl_type(decl))), tdecl, mdecl);
    }
    case KEYno_type:
    case KEYprivate_type:
      return decl;
      break;
    default:
      printf("default case x %d\n", (int)Type_KEY(some_type_decl_type(decl)));
      return decl;
    }
  }
  case KEYsome_type_formal:
    return canonical_type_decl(canonical_type(get_actual_given_formal(tdecl, mdecl, decl)));
  default:
    printf("default case y %d\n", (int)Declaration_KEY(decl));
    return decl;
  }
}

/**
 * Canonical type given a function type
 * @param use any use
 * @return canonical type qual use
 */
static CanonicalType *canonical_type_function(Type t)
{
  int num_formals = count_declarations(function_type_formals(t));
  CanonicalType *return_type = canonical_type(function_type_return_type(t));

  size_t my_size = sizeof(struct Canonical_function_type) + num_formals * (sizeof(CanonicalType *));

  struct Canonical_function_type *ctype_function = (struct Canonical_function_type *)alloca(my_size);

  ctype_function->key = KEY_CANONICAL_FUNC;
  ctype_function->num_formals = num_formals;
  ctype_function->return_type = return_type;

  int index = 0;
  Declaration f = first_Declaration(function_type_formals(t));

  while (f != NULL)
  {
    switch (Declaration_KEY(f))
    {
    case KEYseq_formal:
    {
      fatal_error("Not sure how to handle KEYseq_formal");
      ctype_function->param_types[index++] = canonical_type(seq_formal_type(f));
      break;
    }
    case KEYnormal_formal:
    {
      ctype_function->param_types[index++] = canonical_type(normal_formal_type(f));
      break;
    }
    default:
      fatal_error("Not sure to handle the formal while finding canonical function type");
      return NULL;
    }

    f = DECL_NEXT(f);
  }

  void *memory = hash_cons_get(ctype_function, my_size, &canonical_type_table);

  return (CanonicalType *)memory;
}

/**
 * Converts a type into a canonical type
 * @param t Type
 * @return CanonicalType
 */
CanonicalType *canonical_type(Type t)
{
  if (t == NULL)
  {
    return NULL;
  }

  switch (Type_KEY(t))
  {
  case KEYremote_type:
    return canonical_type(remote_type_nodetype(t));
  case KEYtype_inst:
    fatal_error("CanonicalType requested for type instance");
    return NULL;
  case KEYtype_use:
  {
    Use use = type_use_use(t);
    switch (Use_KEY(use))
    {
    case KEYuse:
      return canonical_type_use(use);
    case KEYqual_use:
      return canonical_type_qual_use(use);
    default:
      aps_error(t, "Case of type use %d is not implemented in canonical_type() for use", (int)Use_KEY(use));
      return NULL;
    }
  }
  case KEYfunction_type:
    return canonical_type_function(t);
  default:
    aps_error(t, "Case of type %d is not implemented in canonical_type()", (int)Type_KEY(t));
    return NULL;
  }
}

/**
 *  Returns the base type of a canonical type
 * @param canonicalType
 * @return base type of a canonicalType
 */
CanonicalType *canonical_type_base_type(CanonicalType *canonicalType)
{
  switch (canonicalType->key)
  {
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use *canonical_type_use = (struct Canonical_Use *)canonicalType;
    Declaration tdecl = canonical_type_use->decl;

    switch (Declaration_KEY(tdecl))
    {
    case KEYsome_type_formal:
      return canonicalType;
    case KEYtype_renaming:
      fatal_error("type_renaming should not show up in a canonical base type");
      return canonicalType;
    case KEYsome_type_decl:
    {
      Type t = some_type_decl_type(tdecl);
      switch (Type_KEY(t))
      {
      case KEYno_type:
      case KEYprivate_type:
        return canonicalType;
      case KEYtype_inst:
      {
        Use mu = module_use_use(type_inst_module(t));
        Declaration mdecl = USE_DECL(mu);
        Declaration rdecl = module_decl_result_type(mdecl);

        if (module_decl_generating(mdecl))
        {
          return canonicalType;
        }
        else
        {
          Declaration thing = canonical_type_decl(canonical_type(some_type_decl_type(rdecl))); // this needs to be corrected because it maybe formal and needs to be replaced by actual

          return canonical_type_base_type(canonical_type(get_actual_given_formal(tdecl, mdecl, thing)));
        }
      }
      default:
        aps_error(t, "Case of type use %d is not implemented in canonical_type_base_type() for type", (int)Type_KEY(t));
        return NULL;
      }
    }
    default:
      aps_error(tdecl, "Case of type use %d is not implemented in canonical_type_base_type() for type", (int)Declaration_KEY(tdecl));
      return NULL;
    }
  }
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *canonical_type_qual = (struct Canonical_qual_type *)canonicalType;

    CanonicalType *source_base = canonical_type_base_type(canonical_type_qual->source);

    printf("\nbefore\t");
    print_canonical_type(canonicalType, stdout);
    printf("\nafter\t");
    print_canonical_type(source_base, stdout);
    printf("\n");

    // Short-circut if there has been no change
    if (canonical_type_qual->source == source_base)
    {
      return canonicalType;
    }

    Declaration thing = canonical_type_qual->decl;
    Declaration tdecl = canonical_type_decl(canonical_type_qual->source);
    Declaration mdecl = USE_DECL(module_use_use(type_inst_module(some_type_decl_type(tdecl))));

    printf("before result: %s\n", decl_name(thing));
    Declaration result = substitute_decl(thing, tdecl, mdecl);
    printf("After result: %s\n", decl_name(result));

    if (!is_inside_module(mdecl, result))
    {
      return new_canonical_type_use(result);
    }
    else
    {
      return new_canonical_type_qual(canonical_type_qual->source, result);
    }
  }
  case KEY_CANONICAL_FUNC:
  {
    struct Canonical_function_type *canonical_type_function = (struct Canonical_function_type *)shallow_clone_canonical_function_types((struct Canonical_function_type *)canonicalType);

    size_t my_size = sizeof(struct Canonical_function_type) + canonical_type_function->num_formals * (sizeof(CanonicalType *));

    canonical_type_function->return_type = canonical_type_base_type(canonical_type_function->return_type);
    int i;
    for (i = 0; i < canonical_type_function->param_types[i]; i++)
    {
      canonical_type_function->param_types[i] = canonical_type_base_type(canonical_type_function->param_types[i]);
    }

    void *memory = hash_cons_get(canonical_type_function, my_size, &canonical_type_table);

    return (CanonicalType *)memory;
  }
  default:
    aps_error(canonicalType, "Not sure how to find the base type of this");
    return NULL;
  }
}