#ifndef CANONICAL_TYPE_H
#define CANONICAL_TYPE_H

#define KEY_CANONICAL_FUNC 0
#define KEY_CANONICAL_USE  1
#define KEY_CANONICAL_QUAL 2

struct canonicalTypeBase { int key; };
typedef struct canonicalTypeBase CanonicalType;

struct Canonical_function_type {
    int key; /* KEY_CANONICAL_FUNC */
    int num_formals;
    CanonicalType* return_type;
    CanonicalType* param_types;
    int param_count;
};

struct Canonical_qual_type {
    int key; /* KEY_CANONICAL_QUAL */
    Declaration decl;
    CanonicalType* source;
};

struct Canonical_use {
    int key; /* KEY_CANONICAL_USE */
    Declaration decl;
};

CanonicalType* from_type(Type ty);

#endif
