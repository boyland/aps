/* APS-UTIL
 * utility functions for accessing APS nodes.
 * Basically, they implement patterns in aps-tree.aps
 */

#define type_value_t(x) type_value_T(x) /* generation problem */

#define KEYreplacement KEYclass_replacement: \
                  case KEYmodule_replacement: \
                  case KEYsignature_replacement: \
                  case KEYtype_replacement: \
                  case KEYvalue_replacement: \
                  case KEYpattern_replacement
extern void *replacement_from(Declaration);
extern void *replacement_to(Declaration);

#define KEYrenaming KEYclass_renaming: \
               case KEYmodule_renaming: \
               case KEYsignature_renaming: \
               case KEYtype_renaming: \
               case KEYvalue_renaming: \
               case KEYpattern_renaming
extern Def renaming_def(Declaration);
extern void *renaming_old(Declaration);

#define KEYsome_use KEYclass_use: \
               case KEYmodule_use: \
               case KEYsignature_use: \
               case KEYtype_use: \
               case KEYvalue_use: \
               case KEYpattern_use
extern Use some_use_u(void *);

#define KEYformal KEYnormal_formal: case KEYseq_formal
extern Def formal_def(Declaration);
extern Type formal_type(Declaration);

#define KEYsome_type_formal KEYtype_formal: case KEYphylum_formal
extern Def some_type_formal_def(Declaration);
extern Signature some_type_formal_sig(Declaration);

#define KEYdeclaration KEYclass_decl: \
                  case KEYmodule_decl: \
                  case KEYsignature_decl: \
                  case KEYphylum_decl: \
                  case KEYtype_decl: \
                  case KEYvalue_decl: \
                  case KEYattribute_decl: \
                  case KEYfunction_decl: \
                  case KEYprocedure_decl: \
                  case KEYconstructor_decl: \
                  case KEYpattern_decl: \
                  case KEYrenaming: \
                  case KEYformal: \
                  case KEYsome_type_formal: \
                  case KEYinheritance: \
                  case KEYpolymorphic
extern Def declaration_def(Declaration);

#define KEYsome_class_decl KEYclass_decl: case KEYmodule_decl
extern Def some_class_decl_def(Declaration);
extern Declarations some_class_decl_type_formals(Declaration);
extern Declaration some_class_decl_result_type(Declaration);
extern Signature some_class_decl_parent(Declaration);
extern Block some_class_decl_contents(Declaration);

#define KEYsome_type_decl KEYtype_decl: case KEYphylum_decl
extern Def some_type_decl_def(Declaration);
extern Signature some_type_decl_sig(Declaration);
extern Type some_type_decl_type(Declaration);

#define KEYsome_value_decl KEYvalue_decl: \
                      case KEYattribute_decl: \
                      case KEYfunction_decl: \
                      case KEYprocedure_decl: \
                      case KEYconstructor_decl: \
		      case KEYformal
extern Type some_value_decl_type(Declaration);

#define KEYassign KEYnormal_assign: case KEYcollect_assign
extern Expression assign_lhs(Declaration);
extern Expression assign_rhs(Declaration);

/* Others that should have beenn given definitions in aps-tree.aps */
#define KEYsome_function_decl KEYfunction_decl: case KEYprocedure_decl
extern Def some_function_decl_def(Declaration);
extern Type some_function_decl_type(Declaration);
extern Block some_function_decl_body(Declaration);

#define KEYsome_case_stmt KEYcase_stmt: \
                     case KEYfor_stmt
extern Expression some_case_stmt_expr(Declaration);
extern Matches some_case_stmt_matchers(Declaration);

/* for loop for accessing elements (only works in C++ with stl) */
#define FOR_SEQUENCE(etype,id,stype,seq,body) \
   { std::stack<stype> _st; _st.push(seq); \
     while (!_st.empty()) { \
       stype _t = _st.top(); _st.pop(); \
       switch(stype##_KEY(_t)) { \
        case KEYnil_##stype: break; \
        case KEYlist_##stype: \
         { etype id = list_##stype##_elem(_t); body; } break;\
        case KEYappend_##stype: \
         _st.push(append_##stype##_l2(_t)); \
	 _st.push(append_##stype##_l1(_t)); \
         break; } } }
