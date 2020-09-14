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
    return 0;

  struct canonicalTypeBase *canonical_type = (struct CanonicalType *)arg;

  switch (canonical_type->key)
  {
  case KEY_CANONICAL_USE:
  {
    struct Canonical_use *canonical_use_type = (struct Canonical_use *)arg;
    return hash_mix(canonical_use_type->key, hash_string(decl_name(canonical_use_type->decl)));
  }
  case KEY_CANONICAL_QUAL:
  {
    struct Canonical_qual_type *canonical_qual_type = (struct Canonical_qual_type *)arg;
    return hash_mix(canonical_qual_type->key, hash_mix(hash_string(decl_name(canonical_qual_type->decl)), canonical_type_hash(canonical_qual_type->source)));
  }
  case KEY_CANONICAL_FUNC:
  {
    struct Canonical_function_type *canonical_function_type = (struct Canonical_function_type *)arg;

    int index;
    int param_types_hash = 0;
    for (index = 0; index < canonical_function_type->num_formals; index++)
    {
      param_types_hash = hash_mix(param_types_hash, canonical_type_hash(&canonical_function_type->param_types[index]));
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
    return false;

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

    return (canonical_qual_type_a->decl == canonical_qual_type_b->decl) && (canonical_qual_type_a->source && canonical_qual_type_b->source);
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

    return params_equal && (canonical_function_type_a->param_types == canonical_function_type_b->param_types) && (canonical_function_type_a->return_type == canonical_function_type_b->return_type);
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
    f = stdout;
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
        started = TRUE;
      }
      print_canonical_type(canonical_func_type->param_types[i], f);
    }
    fputc(')', f);
    fprintf("=>", f);
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
  bool is_inside_module = false;
  void *thing = node;
  while ((thing = tnode_parent(thing)) != NULL)
  {
    if (ABSTRACT_APS_tnode_phylum(thing) == KEYDeclaration && Declaration_KEY((Declaration)thing) == KEYmodule_decl && !strcmp(decl_name((Declaration)thing), decl_name(mdecl)))
    {
      is_inside_module = true;
    }
  }

  return is_inside_module;
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
    aps_error(canonical_type, "Failed to find the module for type use");
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
    default:
      fatal_error("Unknown type use_decl type key");
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
 * Canonical type given a qual use
 * @param use any use
 * @return canonical type qual use
 */
static CanonicalType *canonical_type_qual_use(Use use)
{
  CanonicalType *from_canonicalized = canonical_type(qual_use_from(use));
  CanonicalType *inside_canonicalized = canonical_type_use(use);

  Declaration type_inst_decl = canonical_type_decl(from_canonicalized);
  Declaration inside_decl = canonical_type_decl(inside_canonicalized);

  Module module = type_inst_module(type_decl_type(type_inst_decl));
  Declaration mdecl = USE_DECL(module_use_use(module));

  if (tnode_info(inside_decl) != NULL && !is_inside_module(mdecl, inside_decl))
  {
    return new_canonical_type_use(inside_decl);
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

    // type XXX := Result
    // Get result of module
    if (module_decl_result_type(mdecl) == thing || module_decl_result_type(mdecl) == USE_DECL(use))
    {
      if (module_decl_generating(mdecl))
      {
        return new_canonical_type_use(type_inst_decl);
      }
      else
      {
        Type tdecl_type = base_type(type_decl_type(type_inst_decl));

        return new_canonical_type_use(USE_DECL(type_use_use(tdecl_type)));
      }
    }

    // type XXX := formal;
    // Get actual matching position of the formal
    if (Declaration_KEY(thing) == KEYtype_formal)
    {
      int index = 0;
      Declaration formal = first_Declaration(module_decl_type_formals(mdecl));
      while (formal != NULL)
      {
        if (strcmp(decl_name(formal), symbol_name(use_name(type_use_use(type_decl_type(udecl))))) == 0)
        {
          break;
        }

        index++;
        formal = DECL_NEXT(formal);
      }

      Type actual = first_TypeActual(type_inst_type_actuals(type_decl_type(type_inst_decl)));
      while (actual != NULL)
      {
        if (index == 0)
        {
          break;
        }

        actual = TYPE_NEXT(actual);
      }

      return new_canonical_type_use(USE_DECL(type_use_use(actual)));
    }

    return canonical_type_use_nested;
  }
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
  CanonicalType *return_type = canonical_type(t);

  size_t my_size = sizeof(struct Canonical_function_type) + num_formals * (sizeof(CanonicalType *));

  struct Canonical_function_type *ctype_function = (struct Canonical_function_type *)alloca(my_size);

  ctype_function->key = KEY_CANONICAL_FUNC;
  ctype_function->num_formals = num_formals;
  ctype_function->return_type = return_type;

  Declaration temp = first_Declaration(function_type_formals(t));
  int index = 0;

  while ((temp != DECL_NEXT(temp)) != NULL)
  {
    switch (Declaration_KEY(temp))
    {
    case KEYseq_formal:
    {
      fatal_error("Not sure how to handle KEYseq_formal");
      ctype_function->param_types[index++] = seq_formal_type(temp);
      break;
    }
    case KEYnormal_formal:
    {
      ctype_function->param_types[index++] = canonical_type(normal_formal_type(temp));
      break;
    }
    default:
      fatal_error("Not sure to handle the formal while finding canonical function type");
      return NULL;
    }
  }

  void *memory = hash_cons_get(ctype_function, sizeof(&ctype_function), &canonical_type_table);

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
      aps_error(t, "Case of type use %d is not implemented in canonical_type()", (int)Use_KEY(use));
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
