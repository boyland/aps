#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aps-ag.h"
#include "jbb-alloc.h"

/**
 * Hash string
 * @param string
 * @return intger hash value
 */
static int hash_string(unsigned char *str)
{
  unsigned long hash = 5381;
  int c;

  while (c = *str++)
  {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

/**
 * Combine two hash values into one
 * @param hash1
 * @param hash2
 * @return combined hash 
 */
static int hash_mix(int h1, int h2)
{
  int hash = 17;
  hash = hash * 31 + h1;
  hash = hash * 31 + h2;
  return hash;
}

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
static CanonicalType *canonical_type_use(Declaration decl)
{
  struct Canonical_use ctype_use = {KEY_CANONICAL_USE, decl};
  void *memory = hash_cons_get(&ctype_use, sizeof(ctype_use), &canonical_type_table);
  memcpy(memory, &ctype_use, sizeof(ctype_use));
  return (CanonicalType *)memory;
}

/**
 * Creates an instance of Canonical_qual_type
 * @param from
 * @param decl
 * @return Canonical_qual_type
 */
static CanonicalType *canonical_type_qual(CanonicalType *from, Declaration decl)
{
  struct Canonical_qual_type ctype_qual = {KEY_CANONICAL_QUAL, decl, from};
  void *memory = hash_cons_get(&ctype_qual, sizeof(ctype_qual), &canonical_type_table);
  memcpy(memory, &ctype_qual, sizeof(ctype_qual));
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
    fatal_error("Canonical requested for type instance");
    return NULL;
  case KEYtype_use:
  {
    Use use = type_use_use(t);
    switch (Use_KEY(use))
    {
    case KEYuse:
    {
      Declaration td = Use_info(use)->use_decl;

      switch (Declaration_KEY(td))
      {
      case KEYsome_type_decl:
      {
        switch (Type_KEY(some_type_decl_type(td)))
        {
        case KEYtype_inst:
          return canonical_type_use(td);
        case KEYprivate_type:
          return canonical_type_use(td);
        case KEYtype_use:
          return canonical_type(some_type_decl_type(td));
        }
      }
      case KEYtype_replacement:
        return canonical_type_use(type_replacement_as(td));
      case KEYtype_renaming:
        return canonical_type_use(type_renaming_old(td));
      }

      return canonical_type_use(USE_DECL(type_use_use(t)));
    }
    case KEYqual_use:
    {
      CanonicalType *from_canonicalized = canonical_type(qual_use_from(use));
      Declarations use_decl = USE_DECL(type_use_use(t));
      Declaration d = USE_DECL(type_use_use(t));

      Module m = NULL;
      Declaration type_inst_decl = NULL;

      switch (from_canonicalized->key)
      {
      case KEY_CANONICAL_USE:
      {
        struct Canonical_use *canonical_use_type = (struct Canonical_use *)from_canonicalized;

        m = type_inst_module(type_decl_type(canonical_use_type->decl));
        type_inst_decl = canonical_use_type->decl;

        break;
      }
      default:
        aps_error(t, "Failed to find the module for type use");
      }

      Declaration mdecl = USE_DECL(module_use_use(m));
      Block cblock = module_decl_contents(mdecl);

      Declaration de;
      for (de = first_Declaration(block_body(cblock)); de; de = DECL_NEXT(de))
      {
        switch (Declaration_KEY(de))
        {
        case KEYtype_decl:
        {
          if (intern_symbol(decl_name(de)) == qual_use_name(use))
          {
            // TODO: make sure type is unique
            if (Type_KEY(type_decl_type(de)) == KEYno_type)
            {
              return canonical_type_qual(from_canonicalized, de);
            }

            if (Type_KEY(type_decl_type(de)) == KEYtype_inst)
            {
              return canonical_type_qual(from_canonicalized, de);
            }

            if (intern_symbol("Result") == use_name(type_use_use(type_decl_type(de))))
            {
              if (module_decl_generating(mdecl))
              {
                return canonical_type(qual_use_from(use));
              }
              else
              {
                Type ae = base_type(type_decl_type(type_inst_decl));

                return canonical_type_use(USE_DECL(type_use_use(ae)));
              }
            }

            if (Declaration_KEY(USE_DECL(type_use_use(type_decl_type(de)))) == KEYtype_formal)
            {
              int index = 0;
              Declaration first_formal = first_Declaration(module_decl_type_formals(mdecl));
              while (first_formal != NULL)
              {
                if (strcmp(decl_name(first_formal), symbol_name(use_name(type_use_use(type_decl_type(de))))) == 0)
                {
                  break;
                }

                index++;
                first_formal = DECL_NEXT(first_formal);
              }

              Type e = first_TypeActual(type_inst_type_actuals(type_decl_type(type_inst_decl)));
              while (e != NULL)
              {
                if (index == 0)
                {
                  break;
                }

                e = TYPE_NEXT(e);
              }

              if (!module_decl_generating(mdecl))
              {
                return canonical_type_use(USE_DECL(type_use_use(e)));
              }
              else
              {
                return canonical_type_qual(from_canonicalized, USE_DECL(type_use_use(e)));
              }
            }

            return canonical_type_use(USE_DECL(type_use_use(type_decl_type(de))));
          }
        }
        break;
        }
      }

      struct Canonical_qual_type cqual_use = {KEY_CANONICAL_QUAL,
                                              USE_DECL(type_use_use(t)),
                                              canonical_type(qual_use_from(use))};

      void *memory = hash_cons_get(&cqual_use, sizeof(cqual_use), &canonical_type_table);

      memcpy(memory, &cqual_use, sizeof(cqual_use));

      return (CanonicalType *)memory;
    }
    default:
      aps_error(t, "Case of type use %d is not implemented in from_type()", (int)Use_KEY(use));
      return NULL;
    }
  }
  case KEYfunction_type:
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
      }
    }

    void *memory = hash_cons_get(&ctype_function, sizeof(ctype_function), &canonical_type_table);

    memcpy(memory, &ctype_function, sizeof(ctype_function));

    return (CanonicalType *)memory;
  }
  default:
    aps_error(t, "Case %d is not implemented in from_type()", (int)Type_KEY(t));
    return NULL;
  }
}
