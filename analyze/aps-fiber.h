#ifndef APS_FIBER_H
#define APS_FIBER_H

typedef struct fiber {
  /* three possibilities:
   * 1> null, only for a special case base fiber (never stored in sets)
   * 2> attribute_decl, for forward fibers.
   * 3> normal_formal (of an attribute_decl), for backward fibers
   */
  Declaration field; /* null only for the special case base fiber. */
  struct fiber *shorter; /* null only for special case base fiber. */
  ALIST longer;
} *FIBER;
extern BOOL fiber_is_reverse(FIBER);

extern Declaration reverse_field(Declaration field);

extern FIBER lengthen_fiber(Declaration field, FIBER shorter);
extern FIBER base_fiber;

/* We attach to expressions and attribute declarations unique
 * fibersets.  Individual elements are linked in worklists.
 * We never put the base fiber (the empty fiber) in a forward fiberset.
 * (It may appear in reverse fibersets).
 */
typedef struct fiberset {
  struct fiberset *rest;
  FIBER fiber;
  void *tnode; /* Declaration (attr) or Expression */
  unsigned fiberset_type;
#define FIBERSET_TYPE_REVERSE 1
#define FIBERSET_TYPE_REQUIRE 2
#define FIBERSET_TYPE_FINAL 4
  struct fiberset *next_in_fiber_worklist;
} *FIBERSET;

#define fiberset_require_p(fs) ((fs)->fiberset_type & FIBERSET_TYPE_REQUIRE)
#define fiberset_reverse_p(fs) ((fs)->fiberset_type & FIBERSET_TYPE_REVERSE)

#define FIBERSET_NORMAL_PROVIDE 0
#define FIBERSET_REVERSE_PROVIDE FIBERSET_TYPE_REVERSE
#define FIBERSET_NORMAL_REQUIRE FIBERSET_TYPE_REQUIRE
#define FIBERSET_REVERSE_REQUIRE (FIBERSET_TYPE_REQUIRE|FIBERSET_TYPE_REVERSE)
#define FIBERSET_NORMAL_FINAL FIBERSET_TYPE_FINAL
#define FIBERSET_REVERSE_FINAL (FIBERSET_TYPE_FINAL|FIBERSET_TYPE_REVERSE)

#define BACKWARD_FLOW_P(fstype) \
  ((fstype)==FIBERSET_NORMAL_REQUIRE || (fstype)==FIBERSET_REVERSE_PROVIDE)
#define FORWARD_FLOW_P(fstype) \
  ((fstype)==FIBERSET_NORMAL_PROVIDE || (fstype)==FIBERSET_REVERSE_REQUIRE)

typedef struct fibersets {
  FIBERSET set[6];
} FIBERSETS;

#define fibersets_index(sets,fstype) (sets).set[fstype]

extern int fiberset_length(FIBERSET);
extern int member_fiberset(FIBER,FIBERSET);

struct analysis_state;

extern void add_fibers_to_state(struct analysis_state *s);
extern void fiber_module(Declaration module, struct analysis_state *s);

extern void print_fiber(FIBER,FILE *);
extern void print_fiberset(FIBERSET,FILE *);
extern void print_fiberset_entry(FIBERSET,FILE *);

extern int fiber_debug;
#define ADD_FIBER 1
#define ALL_FIBERSETS 2
#define PUSH_FIBER 4
#define FIBER_INTRO 8
#define FIBER_FINAL 16
#define CALLSITE_INFO 32 
#define ADD_FSA_EDGE 64

/* useful routines */

extern Declaration field_ref_p(Expression);
extern Declaration attr_ref_p(Expression);
extern Declaration local_call_p(Expression);
extern Declaration constructor_call_p(Expression);
extern Declaration object_decl_p(Declaration);
extern Declaration shared_use_p(Expression expr);

extern Expression field_ref_object(Expression); /* also good for attr_ref's */
#define attr_ref_object(expr) field_ref_object(expr)

extern Declaration node_decl_phylum(Declaration);
extern Declaration constructor_decl_phylum(Declaration);
extern Declaration some_function_decl_result(Declaration);
extern Declaration attribute_decl_phylum(Declaration attr);

/* routines for handling shared uses */
extern Declaration phylum_shared_info_attribute(Declaration,
						struct analysis_state *);
extern Declaration responsible_node_declaration(void *);
extern Declaration shared_use_p(Expression);
extern Declaration responsible_node_shared_info(void *,
						struct analysis_state *);
extern Declaration formal_in_case_p(Declaration);
Declaration function_actual_formal(Declaration func,
				   Expression actual,
				   Expression call);


typedef struct oset {
  struct oset *rest;
  Declaration o;
} *OSET;
typedef struct uset {
  struct uset *rest;
  Expression u;
  // f: field_ref_p(u)
  // dot: if EXPR_IS_LHS(u)
} *USET;
#define EMPTY_OSET (OSET)NULL
#define EMPTY_USET (USET)NULL

#endif
