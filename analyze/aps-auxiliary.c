#include "aps-ag.h"
#include "jbb-alloc.h"
#include <stdio.h>

InferredSignature head = NULL;

/**
 * TODO
 * note: class names have to be unique so we probably can use use_name
 * > symbol_name(use_name(class_use_use(...)))
 */
Boolean class_equal(Class a, Class b) { return a == b; }

/**
 * TODO
 * note: maybe we can use print_Type(...) to check type equality
 */
Boolean type_equal(Type a, Type b) { return a == b; }

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
    InferredSignature current = head;
    while (current != NULL) {
        if (current->is_input == is_input && current->is_var == is_var &&
            class_equal(current->_class, class) && current->num_actuals == num_actuals &&
            types_equal(num_actuals, current->actuals, actuals)) {
            return current;
        }
        current = current->next;
    }

    InferredSignature result = (InferredSignature)HALLOC(sizeof(struct InferredSignature_t));
    result->is_input = is_input;
    result->is_var = is_var;
    result->_class = class;
    result->actuals = actuals;
    result->next = head;
    head = result;

    return result;
}
