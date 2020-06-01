#include <stdio.h>
#include "aps-ag.h"
#include "hash_table.h"
#include "jbb-alloc.h"

ht_hash_table *htable = NULL;

unsigned long hash_string(unsigned char *str);
unsigned long hash_mix(unsigned long h1, unsigned long h2);
unsigned long hash_type(Type t);
unsigned long hash_types(int count, Type *t);
unsigned long hash_use(Use u);
unsigned long hash_class(Class cl);

/**
 * http://www.cse.yorku.ca/~oz/hash.html
 */
unsigned long hash_string(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

/**
 * Combine two hash values into one
 */
unsigned long hash_mix(unsigned long h1, unsigned long h2) {
    int hash = 17;
    hash = hash * 31 + h1;
    hash = hash * 31 + h2;
    return hash;
}

unsigned long hash_type(Type t) {
    if (t == 0) {
        return 0;
    }
    switch (Type_KEY(t)) {
    case KEYtype_use:
        return hash_use(type_use_use(t));
    case KEYprivate_type:
        return hash_mix(hash_string("private"), hash_type(private_type_rep(t)));
    case KEYremote_type:
        return hash_mix(hash_string("remote"), hash_type(remote_type_nodetype(t)));
    case KEYfunction_type: {
        unsigned long h = hash_string("function");

        Declarations fs = function_type_formals(t);
        Declarations rs = function_type_return_values(t);
        Declaration a, r;
        int started = FALSE;
        for (a = first_Declaration(fs); a; a = DECL_NEXT(a)) {
            h = hash_mix(h, hash_string(decl_name(a)));
        }

        for (r = first_Declaration(rs); r; r = DECL_NEXT(r)) {
            h = hash_mix(h, hash_string(decl_name(r)));
        }

        return h;
    }
    case KEYtype_inst: {
        unsigned long h = hash_use(module_use_use(type_inst_module(t)));

        TypeActuals as = type_inst_type_actuals(t);
        Type a;
        for (a = first_TypeActual(as); a; a = TYPE_NEXT(a)) {
            h = hash_mix(h, hash_type(a));
        }
        return h;
    }
    case KEYno_type:
        return hash_string("<notype>");
        break;
    default:
        return 0x1ffffffffu;
    }
}

unsigned long hash_types(int count, Type *t) {
    unsigned long h = 0;
    int i;
    for (i = 0; i < count; i++) {
        h = hash_mix(h, hash_type(t[i]));
    }

    return h;
}

unsigned long hash_use(Use u) {
    if (u == 0) {
        return 0;
    }
    switch (Use_KEY(u)) {
    case KEYuse:
        return hash_string(symbol_name(use_name(u)));
    case KEYqual_use:
        return hash_mix(hash_type(qual_use_from(u)), hash_string(symbol_name(qual_use_name(u))));
    default:
        return 0x1ffffffffu;
    }
}

unsigned long hash_class(Class cl) {
    if (cl == 0) {
        return 0;
    }
    switch (Class_KEY(cl)) {
    case KEYclass_use:
        return hash_use(class_use_use(cl));
    default:
        return 0x1ffffffffu;
    }
}

Boolean class_equal(Class a, Class b) { return hash_class(a) == hash_class(b); }

Boolean type_equal(Type a, Type b) { return hash_type(a) == hash_type(b); }

Boolean types_equal(int count, Type *a, Type *b) {
    Boolean result = TRUE;
    int i;
    for (i = 0; i < count && result; i++) {
        result &= type_equal(a[i], b[i]);
    }

    return result;
}

InferredSignature create_inferred_sig(Boolean is_input, Boolean is_var, Class class,
                                      int num_actuals, Type *actuals) {

    if (htable == NULL) {
        htable = ht_new();
    }

    long long_key =
        hash_mix((long)is_input,
                 hash_mix((long)is_var,
                          hash_mix(hash_class(class),
                                   hash_mix((long)num_actuals, hash_types(num_actuals, actuals)))));

    char key[50];
    sprintf(key, "%lu", long_key);
    InferredSignature result;

    if ((result = (InferredSignature)ht_search(htable, key)) != NULL) {
        return result;
    }

    result = (InferredSignature)HALLOC(sizeof(struct InferredSignature_t));
    result->is_input = is_input;
    result->is_var = is_var;
    result->_class = class;
    result->actuals = actuals;

    return result;
}
