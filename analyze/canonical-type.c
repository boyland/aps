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
 * Check if declaration is a Result of some module without using declaration name (e.g. decl_name() == "Result")
 * @param decl
 * @return boolean indicating if declaration is a result of some outer module 
 */
static bool is_some_result_decl(Declaration decl)
{
  void *current = decl;
  Declaration current_decl;
  while (current != NULL && (current = tnode_parent(current)) != NULL)
  {
    switch (ABSTRACT_APS_tnode_phylum(current))
    {
    case KEYDeclaration:
      current_decl = (Declaration)current;
      switch (Declaration_KEY(current_decl))
      {
      case KEYmodule_decl:
        return module_decl_result_type(current_decl) == decl;
      }
    }
  }

  return false;
}

/**
 * Canonical type given a use
 * @param use any use
 * @return canonical type use
 */
static CanonicalType *canonical_type_use(Use use)
{
  Declaration td = Use_info(use)->use_decl;

  // printf("td: %s (%d)\n", decl_name(td), Declaration_KEY(td));

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
    {
      // This will catch if we are going from D -> Result -> T
      // In that case we just want to result Result
      Declaration nested_use_decl = USE_DECL(type_use_use(some_type_decl_type(td)));
      // printf("nested_use_decl: %s %s\n", decl_name(nested_use_decl), is_some_result_decl(nested_use_decl) ? "true" : "false");
      if (is_some_result_decl(nested_use_decl))
      {
        return new_canonical_type_use(nested_use_decl);
      }

      return canonical_type(some_type_decl_type(td));
    }

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

/**
 * Returns actual Type given formal declaration, type declaration and module declaration
 * @param tdecl type declaration
 * @param mdecl module declaration
 * @param formal formal declaration
 * @return actual Type (or NULL if it does not find a match)
 */
static Type get_actual_given_formal(Declaration tdecl, Declaration mdecl, Declaration formal)
{
  Declaration f;
  Type actual;

  for (f = first_Declaration(module_decl_type_formals(mdecl)),
      actual = first_TypeActual(type_inst_type_actuals(type_decl_type(tdecl)));
       f != NULL; f = DECL_NEXT(f), actual = TYPE_NEXT(actual))
  {
    if (formal == f)
    {
      return actual;
    }
  }

  fatal_error("Not sure how to find the actual given formal");

  return NULL;
}

/**
 * Joins two canonical typers
 * @param ctypeOuter outer canonical type
 * @param ctypeInner inner canonical type
 * @return substituted resuling canonical type
 */
static CanonicalType *join_canonical_types(CanonicalType *ctypeOuter, CanonicalType *ctypeInner)
{
  if (ctypeOuter == NULL)
  {
    return ctypeInner;
  }

  Declaration mdecl = NULL;
  Declaration tdecl = NULL;

  // Tries to resolve mdecl and tdecl from ctypeOuter
  Declaration some_decl = canonical_type_decl(ctypeOuter);
  switch (Declaration_KEY(some_decl))
  {
  case KEYsome_type_decl:
  {
    tdecl = some_decl;
    mdecl = USE_DECL(module_use_use(type_inst_module(type_decl_type(tdecl))));
    break;
  }
  default:
    fatal_error("Not sure what type of canonical type it is");
  }

  switch (ctypeInner->key)
  {
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use *canonical_type_use = (struct Canonical_Use *)ctypeInner;
    Declaration decl = canonical_type_use->decl;

    // If decl is not inside the module then short-circuit
    if (!is_inside_module(mdecl, decl))
    {
      return new_canonical_type_use(decl);
    }

    // If decl is the Result of module then return type decl
    if (module_decl_result_type(mdecl) == decl)
    {
      return new_canonical_type_use(tdecl);
    }

    switch (Declaration_KEY(decl))
    {
    case KEYsome_type_formal:
      return canonical_type(get_actual_given_formal(tdecl, mdecl, decl));
    case KEYsome_type_decl:
    {
      Type t = some_type_decl_type(decl);
      switch (Type_KEY(t))
      {
      case KEYno_type:
      case KEYprivate_type:
      case KEYtype_inst:
        return new_canonical_type_qual(ctypeOuter, decl);
      case KEYtype_use:
        return join_canonical_types(ctypeOuter, canonical_type(t));
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
    struct Canonical_qual_type *canonical_type_qual = (struct Canonical_qual_type *)ctypeInner;
    Declaration decl = canonical_type_qual->decl;

    ctypeInner = join_canonical_types(ctypeInner, new_canonical_type_use(canonical_type_qual->decl));
    return join_canonical_types(ctypeOuter, ctypeInner);

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
          return ctypeInner;
        }
        else
        {
          ctypeInner = join_canonical_types(ctypeInner, new_canonical_type_use(module_decl_result_type(nested_mdecl)));

          return join_canonical_types(ctypeOuter, ctypeInner);
        }
      }
      case KEYtype_use:
        return join_canonical_types(ctypeOuter, canonical_type(tdecl_type));
      case KEYno_type:
      case KEYprivate_type:
        return ctypeInner;
      default:
        aps_error(tdecl_type, "Unexpected type %d in resolve_canonical_base_type()", (int)Type_KEY(tdecl_type));
        return decl;
      }
    }
    case KEYsome_type_formal:
      return canonical_type(get_actual_given_formal(tdecl, mdecl, decl));
    default:
      aps_error(decl, "Unexpected decl %d in resolve_canonical_base_type()", (int)Declaration_KEY(decl));
      return decl;
    }
  }
  case KEY_CANONICAL_FUNC:
  {
    struct Canonical_function_type *canonical_function_type = (struct Canonical_function_type *)ctypeInner;
    size_t my_size = sizeof(struct Canonical_function_type) + canonical_function_type->num_formals * (sizeof(CanonicalType *));

    struct Canonical_function_type *result = shallow_clone_canonical_function_types(canonical_function_type);

    int i;
    for (i = 0; i < canonical_function_type->num_formals; i++)
    {
      result->param_types[i] = join_canonical_types(ctypeOuter, canonical_function_type->param_types[i]);
    }

    result->return_type = join_canonical_types(ctypeOuter, canonical_function_type->return_type);

    void *memory = hash_cons_get(result, my_size, &canonical_type_table);

    return (CanonicalType *)memory;
  }
  default:
    break;
  }
}

/**
 * Canonical type given a qual use
 * @param use any use
 * @return canonical type qual use
 */
static CanonicalType *canonical_type_qual_use(Use use)
{
  CanonicalType *ctypeOuter = canonical_type(qual_use_from(use));
  CanonicalType *ctypeInner = canonical_type_use(use);

  return join_canonical_types(ctypeOuter, ctypeInner);
}

/**
 * Simple function that tries to recursively join two canonical base types
 * @param ctypeOuter outer canonical type
 * @param ctypeInner inner canonical type
 * @return resolved declaration
 */
static CanonicalType *join_canonical_base_types(CanonicalType *ctypeOuter, CanonicalType *ctypeInner)
{
  Declaration mdecl = NULL;
  Declaration tdecl = NULL;

  if (ctypeOuter != NULL)
  {
    // Tries to resolve mdecl and tdecl from ctypeOuter
    Declaration some_decl = canonical_type_decl(ctypeOuter);
    switch (Declaration_KEY(some_decl))
    {
    case KEYsome_type_decl:
    {
      tdecl = some_decl;
      mdecl = USE_DECL(module_use_use(type_inst_module(type_decl_type(tdecl))));
      break;
    }
    }
  }

  switch (ctypeInner->key)
  {
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use *canonical_type_use = (struct Canonical_Use *)ctypeInner;
    Declaration decl = canonical_type_use->decl;

    if (mdecl != NULL && module_decl_generating(mdecl) && decl == module_decl_result_type(mdecl))
    {
      return ctypeOuter;
    }

    switch (Declaration_KEY(decl))
    {
    case KEYsome_type_formal:
      return mdecl != NULL && tdecl != NULL ? canonical_type(get_actual_given_formal(tdecl, mdecl, decl)) : ctypeInner;
    case KEYsome_type_decl:
    {
      Type t = some_type_decl_type(decl);
      switch (Type_KEY(t))
      {
      case KEYno_type:
        return ctypeOuter != NULL ? new_canonical_type_qual(ctypeOuter, decl) : ctypeInner;
      case KEYprivate_type:
        return ctypeInner;
      case KEYtype_use:
        return join_canonical_base_types(ctypeOuter, canonical_type(t));
      case KEYtype_inst:
      {
        Use mu = module_use_use(type_inst_module(t));
        Declaration nested_mdecl = USE_DECL(mu);

        if (module_decl_generating(nested_mdecl))
        {
          return ctypeInner;
        }
        else
        {
          CanonicalType *temp = join_canonical_base_types(ctypeInner, new_canonical_type_use(module_decl_result_type(nested_mdecl)));

          return join_canonical_base_types(ctypeOuter, temp);
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
  break;
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *canonical_type_qual = (struct Canonical_qual_type *)ctypeInner;
    Declaration decl = canonical_type_qual->decl;

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
          return ctypeInner;
        }
        else
        {
          CanonicalType *first = join_canonical_base_types(new_canonical_type_use(decl), new_canonical_type_use(module_decl_result_type(nested_mdecl)));
          CanonicalType *second = join_canonical_base_types(canonical_type_qual->source, first);

          if (canonical_type_qual->source->key == KEY_CANONICAL_QUAL)
          {
            struct Canonical_qual_type *nested_canonical_type_qual = (struct Canonical_qual_type *)canonical_type_qual->source;

            second = join_canonical_base_types(nested_canonical_type_qual->source, second);
          }

          CanonicalType *third = join_canonical_base_types(ctypeOuter, second);

          return third;
        }
      }
      case KEYtype_use:
        return join_canonical_base_types(ctypeOuter, canonical_type(tdecl_type));
      case KEYno_type:
      case KEYprivate_type:
        return tdecl != NULL && is_some_result_decl(decl) ? canonical_type_base_type(new_canonical_type_use(tdecl)) : ctypeInner;
      default:
        aps_error(tdecl_type, "Unexpected type %d in join_canonical_base_types()", (int)Type_KEY(tdecl_type));
        return decl;
      }
    }
    case KEYsome_type_formal:
      return canonical_type(get_actual_given_formal(tdecl, mdecl, decl));
    default:
      aps_error(decl, "Unexpected decl %d in resolve_canonical_base_type()", (int)Declaration_KEY(decl));
      return decl;
    }
  }
  break;
  case KEY_CANONICAL_FUNC:
  {
    struct Canonical_function_type *canonical_type_function = (struct Canonical_function_type *)shallow_clone_canonical_function_types((struct Canonical_function_type *)ctypeInner);

    size_t my_size = sizeof(struct Canonical_function_type) + canonical_type_function->num_formals * (sizeof(CanonicalType *));

    canonical_type_function->return_type = join_canonical_base_types(ctypeOuter, canonical_type_function->return_type);
    int i;
    for (i = 0; i < canonical_type_function->param_types[i]; i++)
    {
      canonical_type_function->param_types[i] = join_canonical_base_types(ctypeOuter, canonical_type_function->param_types[i]);
    }

    void *memory = hash_cons_get(canonical_type_function, my_size, &canonical_type_table);

    return (CanonicalType *)memory;
  }
  default:
    break;
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
  return join_canonical_base_types(NULL, canonicalType);
}
