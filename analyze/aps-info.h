/* APS-INFO
 * Access slots inside APS nodes.
 * This file must be modified and aps-info.c recompiled each time
 * we need a new slot.
 */

extern struct Program_info {
  unsigned program_flags;
#define PROGRAM_BOUND_FLAG 1
} *Program_info(Program);
#define PROGRAM_IS_BOUND(p) (Program_info(p)->program_flags&PROGRAM_BOUND_FLAG)

extern struct Use_info {
  Declaration use_decl;
} *Use_info(Use);
#define USE_DECL(u) (Use_info(u)->use_decl)

extern struct Declaration_info {
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
} *Declaration_info(Declaration);

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
  (Declaration_info(decl)->decl_flags&DECL_SYNTAX_FLAG)
#define TYPE_FORMAL_IS_EXTENSION(decl) \
  (Declaration_info(decl)->decl_flags&TYPE_FORMAL_EXTENSION_FLAG)
#define ATTR_DECL_IS_SYN(decl) \
  (Declaration_info(decl)->decl_flags&ATTR_DECL_SYN_FLAG)
#define ATTR_DECL_IS_INH(decl) \
  (Declaration_info(decl)->decl_flags&ATTR_DECL_INH_FLAG)
#define FIELD_DECL_IS_STRICT(decl) \
  (Declaration_info(decl)->decl_flags&FIELD_DECL_STRICT_FLAG)
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
#define DECL_IS_SHARED(decl) \
  (Declaration_info(decl)->decl_flags&SHARED_DECL_FLAG)
#define DECL_IS_START_PHYLUM(decl) \
  (Declaration_info(decl)->decl_flags&START_PHYLUM_FLAG)

#define fibersets_for(decl) (Declaration_info(decl)->decl_fibersets)
#define fiberset_for(decl,fstype) (fibersets_index(fibersets_for(decl),fstype))

extern struct Expression_info {
  Expression next_actual; /* in a sequence of args to a function call */
#define next_expr next_actual
  Type expr_type; /* current not computed or used */
  Declaration call_decl; /* attribute or function decl called here */
  FIBERSETS expr_fibersets;
  int expr_flags;
#define EXPR_LHS_FLAG 1
#define EXPR_RHS_FLAG 2
} *Expression_info(Expression);
#define EXPR_IS_LHS(expr) (Expression_info(expr)->expr_flags&EXPR_LHS_FLAG)
#define EXPR_IS_RHS(expr) (Expression_info(expr)->expr_flags&EXPR_RHS_FLAG)

#define expr_fibersets_for(expr) (Expression_info(expr)->expr_fibersets)
#define expr_fiberset_for(expr,fstype) \
  (fibersets_index(expr_fibersets_for(expr),fstype))

extern struct Pattern_info {
  Pattern next_pattern_actual; /* in a sequence of args to a pattern call */
  Type pat_type;
} *Pattern_info(Pattern);

extern void set_tnode_parent(Program p);

extern void *tnode_parent(void *);

#define decl_name(decl) symbol_name(def_name(declaration_def(decl)))
