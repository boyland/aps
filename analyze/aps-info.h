/* APS-INFO
 * Access slots inside APS nodes.
 * This file must be modified and aps-info.c recompiled each time
 * we need a new slot.
 */

struct Program_info {
  unsigned program_flags;
#define PROGRAM_BOUND_FLAG 1
#define PROGRAM_TYPED_FLAG 2
};
extern struct Program_info *Program_info(Program);
#define PROGRAM_IS_BOUND(p) (Program_info(p)->program_flags&PROGRAM_BOUND_FLAG)
#define PROGRAM_IS_TYPED(p) (Program_info(p)->program_flags&PROGRAM_TYPED_FLAG)

struct Use_info {
  Declaration use_decl;
  TypeEnvironment use_type_env;
};
extern struct Use_info *Use_info(Use);
#define USE_DECL(u) (Use_info(u)->use_decl)
#define USE_TYPE_ENV(u) (Use_info(u)->use_type_env)

struct Declaration_info {
  Declaration next_decl; /* in a sequence of Declarations */
  Declaration next_field_decl; /* next field (init'ed in aps-fiber) */
  Declaration dual_decl; /* a declaration created out of nowhere */
  Declaration copied_decl; /* pointer to copied version (scratch) */
  FIBERSETS decl_fibersets;
  int if_index; /* count of if_stmt within a top-level match */
#define instance_index if_index
  CONDITION decl_cond; /* condition that must be satisfied to take effect */
  struct summary_dependency_graph *node_phy_graph;
  unsigned decl_flags;
  void * call_sites;
  void *analysis_state;
#define DECL_LHS_FLAG 1
#define DECL_RHS_FLAG 2
#define DECL_OBJECT_FLAG 4
#define TYPE_FORMAL_EXTENSION_FLAG 8
#define ATTR_DECL_SYN_FLAG 16
#define ATTR_DECL_INH_FLAG 32
#define FIELD_DECL_STRICT_FLAG 64
#define FIELD_DECL_CYCLIC_FLAG 128
#define FIELD_DECL_FLAG 256
#define DECL_LOCAL_FLAG 512 /* declaration declared in module being analyzed */
#define FIELD_DECL_REVERSE_FLAG 1024
#define SHARED_INFO_FLAG (1<<11)
#define SHARED_DECL_FLAG (1<<12) /* decl'n declared at top-level of module */
#define START_PHYLUM_FLAG (1<<13)
#define FIELD_DECL_UNTRACKED_FLAG (1<<14)
#define SELF_MANAGED_FLAG (1<<15)
#define UP_DOWN_FLAG (1<<16)
	OSET oset;				/* oset of the declaration */
	USET uset;				/* uset of the declaration */
	int	 index;			  /* represent the nodeis Qd and Qd(-) */
										/* odd: Qd, even: Qd(-). such as (1,2) */
};
extern struct Declaration_info *Declaration_info(Declaration);

#define DECL_NEXT(decl) (Declaration_info(decl)->next_decl)
#define NEXT_FIELD(decl) (Declaration_info(decl)->next_field_decl)
#define DUAL_DECL(decl) (Declaration_info(decl)->dual_decl)
#define MODULE_SHARED_INFO_PHYLUM(m) DUAL_DECL(m)
#define MODULE_SHARED_INFO_POLYMORPHIC(m) NEXT_FIELD(m)
#define POLYMORPHIC_INSTANCES(a) DUAL_DECL(a) /* not used generally yet */
#define COPIED_DECL(decl) (Declaration_info(decl)->copied_decl)
#define DECL_IS_LHS(decl) (Declaration_info(decl)->decl_flags&DECL_LHS_FLAG)
#define DECL_IS_RHS(decl) (Declaration_info(decl)->decl_flags&DECL_RHS_FLAG)
#define DECL_IS_OBJECT(decl) \
  (Declaration_info(decl)->decl_flags&DECL_OBJECT_FLAG)
#define DECL_IS_SYNTAX(decl) \
  (Declaration_info(decl)->decl_flags&(DECL_LHS_FLAG|DECL_RHS_FLAG))
#define TYPE_FORMAL_IS_EXTENSION(decl) \
  (Declaration_info(decl)->decl_flags&TYPE_FORMAL_EXTENSION_FLAG)
#define ATTR_DECL_IS_SYN(decl) \
  (Declaration_info(decl)->decl_flags&ATTR_DECL_SYN_FLAG)
#define ATTR_DECL_IS_INH(decl) \
  (Declaration_info(decl)->decl_flags&ATTR_DECL_INH_FLAG)
#define FIELD_DECL_IS_STRICT(decl) \
  (Declaration_info(decl)->decl_flags&FIELD_DECL_STRICT_FLAG)
#define FIELD_DECL_IS_UNTRACKED(decl) \
  (Declaration_info(decl)->decl_flags&FIELD_DECL_UNTRACKED_FLAG)
#define FIELD_DECL_IS_CYCLIC(decl) \
  (Declaration_info(decl)->decl_flags&FIELD_DECL_CYCLIC_FLAG)
#define FIELD_DECL_P(decl) \
  (Declaration_info(decl)->decl_flags&FIELD_DECL_FLAG)
#define DECL_IS_LOCAL(decl) \
  (Declaration_info(decl)->decl_flags&DECL_LOCAL_FLAG)
#define FIELD_DECL_IS_REVERSE(decl) \
  (Declaration_info(decl)->decl_flags&FIELD_DECL_REVERSE_FLAG)
#define ATTR_DECL_IS_SHARED_INFO(decl) \
  (Declaration_info(decl)->decl_flags&SHARED_INFO_FLAG)
#define ATTR_DECL_IS_UP_DOWN(decl) \
  (Declaration_info(decl)->decl_flags&UP_DOWN_FLAG)
#define DECL_IS_SHARED(decl) \
  (Declaration_info(decl)->decl_flags&SHARED_DECL_FLAG)
#define DECL_IS_START_PHYLUM(decl) \
  (Declaration_info(decl)->decl_flags&START_PHYLUM_FLAG)
#define DECL_IS_SELF_MANAGED(decl) \
  (Declaration_info(decl)->decl_flags&SELF_MANAGED_FLAG)

#define fibersets_for(decl) (Declaration_info(decl)->decl_fibersets)
#define fiberset_for(decl,fstype) (fibersets_index(fibersets_for(decl),fstype))

struct Match_info {
  Declaration header; /* for, case or top-level-match */
  Match next_match;
  int if_index; /* count of if_stmt within a top-level match */
  CONDITION match_cond; /* condition that must be satisfied to take effect */
  Expression match_test; /* last expression that must be evaluated,
			   expr_next points to previous, back to first */
};
extern struct Match_info *Match_info(Match);
#define MATCH_NEXT(m) Match_info(m)->next_match
  
struct Expression_info {
  Expression next_actual; /* in a sequence of args to a function call */
#define next_expr next_actual
  Type expr_type; /* current not computed or used */
  Declaration call_decl; /* attribute or function decl called here */
  FIBERSETS expr_fibersets;
  int expr_flags;
  int expr_helper_num; /* used in incremental impl. */
  struct attribute_instance* value_for; /* This is the RHS that gives value to this instance */
  Declaration funcall_proxy; /* This is a Decl that permits a funcall node to be an INSTANCE node */
#define EXPR_LHS_FLAG 1
#define EXPR_RHS_FLAG 2
	int	 index;			// represent the nodes of Qe and Qe(-).
};
extern struct Expression_info *Expression_info(Expression);
#define EXPR_NEXT(expr) (Expression_info(expr)->next_expr)
#define EXPR_IS_LHS(expr) (Expression_info(expr)->expr_flags&EXPR_LHS_FLAG)
#define EXPR_IS_RHS(expr) (Expression_info(expr)->expr_flags&EXPR_RHS_FLAG)

#define expr_fibersets_for(expr) (Expression_info(expr)->expr_fibersets)
#define expr_fiberset_for(expr,fstype) \
  (fibersets_index(expr_fibersets_for(expr),fstype))
#define expr_helper_for(expr) (Expression_info(expr)->expr_helper_num)

struct Pattern_info {
  Pattern next_pattern_actual; /* in a sequence of args to a pattern call */
  Type pat_type;
  void* pat_impl; /* how it is implemented */
};
extern struct Pattern_info *Pattern_info(Pattern);
#define PAT_NEXT(pat) (Pattern_info(pat)->next_pattern_actual)

struct Type_info {
  Type next_type_actual; /* in a series of type actuals */
  void *binding_temporary;
  char *impl_type;
};
extern struct Type_info *Type_info(Type);
#define TYPE_NEXT(type) (Type_info(type)->next_type_actual)

struct Signature_info {
  void *binding_temporary;
};
extern struct Signature_info *Signature_info(Signature);

struct Def_info {
  int unique_prefix; /* to distinguish local attributes */
};
extern struct Def_info *Def_info(Def);

extern void set_tnode_parent(Program p);

extern void *tnode_parent(void *);

#define decl_name(decl) symbol_name(def_name(declaration_def(decl)))
