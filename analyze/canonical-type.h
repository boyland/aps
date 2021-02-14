#ifndef CANONICAL_TYPE_H
#define CANONICAL_TYPE_H

#define KEY_CANONICAL_FUNC 0
#define KEY_CANONICAL_USE 1
#define KEY_CANONICAL_QUAL 2

struct canonicalTypeBase
{
  int key;
};
typedef struct canonicalTypeBase CanonicalType;

// Type function_type(Declarations formals,Declarations return_values)
struct Canonical_function_type
{
  int key; /* KEY_CANONICAL_FUNC */
  int num_formals;
  CanonicalType *return_type;
  CanonicalType *param_types[];
};

struct Canonical_qual_type
{
  int key; /* KEY_CANONICAL_QUAL */
  Declaration decl;
  CanonicalType *source;
};

struct Canonical_use_type
{
  int key; /* KEY_CANONICAL_USE */
  Declaration decl;
};

typedef struct CanonicalTypeSet_type CanonicalTypeSet;

/**
 * Given an AST Type, it calculates a hashconsed canonical type
 * @param type Type AST
 * @return Hashconsed canonical type
 */
CanonicalType *canonical_type(Type type);

/**
 * Given a hashconsed canonical type, it returns a canonical base type
 * @param canonical_type Hashconsed canonical type
 * @return Hashconsed canonical base type
 */
CanonicalType *canonical_type_base_type(CanonicalType *canonical_type);

/**
 * Joins two canonical types and returns a hashconsed canonical type 
 * @param ctype_outer outer canonical type
 * @param ctype_inner inner canonical type
 * @param is_base_type flag indicating whether result should be base_type or not
 * @return Hashconsed canonical type
 */
CanonicalType *canonical_type_join(CanonicalType *ctype_outer, CanonicalType *ctype_inner, bool is_base_type);

/**
 * Creates a new canonical type use
 * @param decl Declaration
 * @return Hashconsed canonical type use
 */
CanonicalType *new_canonical_type_use(Declaration decl);

/**
 * Compares two canonical types
 * @param ctype1 Canonical type A
 * @param ctype2 Canonical type B
 * @return Integer value representing comparison of two canonical types
 */
int canonical_type_compare(CanonicalType *ctype1, CanonicalType *ctype2);

/**
 * Given a canonical type, it returns a Declaration
 * @param canonical_type Canonical type
 * @return Declaration
 */
Declaration canonical_type_decl(CanonicalType *canonical_type);

/**
 * Given an untyped CanonicalType, it returns a its hash value
 * @arg untyped CanonicalType
 * @return hash value
 */
int canonical_type_hash(void *arg);

/**
 * Given a canonical type, prints it to the file output
 * @param untyped untyped canonical type
 * @return f FILE output
 */
void print_canonical_type(void *untyped, FILE *f);

#endif
