#ifndef APS_TYPE_H
#define APS_TYPE_H

extern Type function_type_return_type(Type);

/* We use local type inference a la Turner and Pierce
 * We special case function calls. These functions store
 * a type (which needs context for functional objects)
 * on the expression as well.
 *
 * The types we read may need to be constructed.
 * Also, we make use of a fact of the way aps-bind handles
 * services with $: the USE_TYPE_ENV is the environment relative
 * to the environment in the type. Hence, one cannot just use
 * the type of the use_decl + the TYPE_ENV stored in the Use:
 * you need to also check if the use is a qual use.  Implicit qualification
 * and polymorphic uses *will* have complete USE_TYPE_ENV informatio.
 *
 * Thus instead of passing around TypeEnvironments, we will pass Use's
 * around which hold a chain of TypeEnvironment's, as in
 *     T$U$V$s
 * For these Use's, we ignore the name and USE_DECL.
 */

extern void type_Program(Program);

extern Type infer_expr_type(Expression);
extern void check_expr_type(Expression,Type);
extern Type infer_element_type(Expression); /* must be seq, ret elem type */
extern void check_element_type(Expression,Type);
extern Type check_actuals(Actuals args, Type ftype, Use type_envs);

extern Type infer_pattern_type(Pattern);
extern void check_pattern_type(Pattern,Type);

extern void check_default_type(Default,Type);

extern void check_matchers_type(Matches,Type);

extern Type infer_formal_type(Declaration formal);

/* for the following functions, we don't assume the actuals
 * have been checked yet.
 */
extern Type infer_function_return_type(Expression,Actuals);
extern void check_function_return_type(Expression,Actuals,Type);

extern Type infer_pfunction_return_type(Pattern,PatternActuals);
extern void check_pfunction_return_type(Pattern,PatternActuals,Type);

extern void check_type_signatures(void*,Type,Use,Signature);
extern void check_type_actuals(TypeActuals,Declarations,Use);

extern BOOL type_is_phylum(Type);
extern Type type_element_type(Type);

/* return true is environment is ready to use
 * (no remaining inference required)
 * Since polymorphic scopes may not export type declarations
 * we don't need to pass in a whole Use
 */
int is_complete(TypeEnvironment);

Type make_type_use(Use type_envs, Declaration tdecl);

Use type_envs_nested(Use);

Type type_inst_base(Type);  // unwind to get a parameter
Type base_type(Type);

/* Interpret a type relative to the environments in a Use. 
 * it may create new type nodes.  The environment(s) in the Use
 * must be complete.
 */
Type type_subst(Use,Type);

Signature sig_subst(Use,Signature);

/* This can take a Use with an incomplete environment.
 * It can also take a void Use
 */
void check_type_subst(void *node, Type t1, Use u2, Type t2);

void check_type_equal(void* node, Type,Type);

/*! This function will fail to terminate in the case of recursive types */
extern int base_type_equal(Type b1, Type b2);

void print_TypeEnvironment(TypeEnvironment,FILE*);
void print_Use(Use,FILE*);
void print_Type(Type,FILE*);
void print_Signature(Signature,FILE*);

extern int type_debug;

#endif
