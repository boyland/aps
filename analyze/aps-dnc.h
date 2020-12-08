#include "jbb-vector.h"

typedef struct attrset {
  struct attrset *rest;
  Declaration attr;
} *ATTRSET;

typedef struct fibered_attribute {
  Declaration attr; /* attribute_decl or local */
  FIBER fiber;
} FIBERED_ATTRIBUTE;

extern BOOL fibered_attr_equal(FIBERED_ATTRIBUTE*,FIBERED_ATTRIBUTE*);

typedef struct attribute_instance {
  FIBERED_ATTRIBUTE fibered_attr;
  Declaration node; /* NULL for locals */
  int index;
} INSTANCE;

enum instance_direction {instance_local, instance_inward, instance_outward};
enum instance_direction instance_direction(INSTANCE *);

typedef unsigned DEPENDENCY;

#define SOME_DEPENDENCY 1
#define DEPENDENCY_NOT_JUST_FIBER 2
#define DEPENDENCY_MAYBE_CARRYING 4
#define DEPENDENCY_MAYBE_DIRECT 8
#define DEPENDENCY_MAYBE_SIMPLE 16

#define no_dependency 0
#define fiber_dependency (SOME_DEPENDENCY|DEPENDENCY_MAYBE_CARRYING|DEPENDENCY_MAYBE_DIRECT|DEPENDENCY_MAYBE_SIMPLE)
#define dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER|DEPENDENCY_MAYBE_CARRYING|DEPENDENCY_MAYBE_DIRECT|DEPENDENCY_MAYBE_SIMPLE)
#define control_fiber_dependency (SOME_DEPENDENCY|DEPENDENCY_MAYBE_DIRECT|DEPENDENCY_MAYBE_SIMPLE)
#define control_dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER|DEPENDENCY_MAYBE_DIRECT|DEPENDENCY_MAYBE_SIMPLE)
#define indirect_fiber_dependency (SOME_DEPENDENCY|DEPENDENCY_MAYBE_CARRYING|DEPENDENCY_MAYBE_SIMPLE)
#define indirect_dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER|DEPENDENCY_MAYBE_CARRYING|DEPENDENCY_MAYBE_SIMPLE)
#define indirect_control_fiber_dependency (SOME_DEPENDENCY|DEPENDENCY_MAYBE_SIMPLE)
#define indirect_control_dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER|DEPENDENCY_MAYBE_SIMPLE)
#define max_dependency dependency
#define indirect_circular_dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER|DEPENDENCY_MAYBE_CARRYING)

#define AT_MOST(k1,k2) (((k1)&~(k2))==0)

extern DEPENDENCY dependency_join(DEPENDENCY,DEPENDENCY);
extern DEPENDENCY dependency_trans(DEPENDENCY,DEPENDENCY);
extern DEPENDENCY dependency_indirect(DEPENDENCY);

typedef struct edgeset {
  struct edgeset *rest;
  INSTANCE *source;
  INSTANCE *sink;
  CONDITION cond;
  DEPENDENCY kind;
  struct edgeset *next_in_edge_worklist;
} *EDGESET;

extern DEPENDENCY edgeset_kind(EDGESET);

typedef struct augmented_dependency_graph {
  Declaration match_rule;
  struct analysis_state *global_state;
  Declaration syntax_decl, lhs_decl, first_rhs_decl;
  VECTOR(void*) if_rules;
  VECTOR(INSTANCE) instances;
  EDGESET *graph; /* two-d array, indexed by instance number */
  EDGESET worklist_head, worklist_tail;
  struct augmented_dependency_graph *next_in_aug_worklist;
  int *schedule; /* one-d array, indexed by instance number */
  struct cto_node *total_order;
} AUG_GRAPH;
extern const char *aug_graph_name(AUG_GRAPH *);

typedef struct summary_dependency_graph {
  Declaration phylum;
  struct analysis_state *global_state;
  VECTOR(INSTANCE) instances;
  DEPENDENCY *mingraph; /* two-d array, indexed by instance number */
  struct summary_dependency_graph *next_in_phy_worklist;
  int *summary_schedule; /* one-d array, indexed by instance number */
} PHY_GRAPH;
extern const char *phy_graph_name(PHY_GRAPH *);
  
typedef VECTOR(struct cycle_description) CYCLES;

typedef struct analysis_state {
  Declaration module;
  TABLE phylum_attrset_table;
  VECTOR(Declaration) match_rules;
  AUG_GRAPH *aug_graphs;
  VECTOR(Declaration) phyla;
  PHY_GRAPH *phy_graphs;
  Declaration start_phylum;
  AUG_GRAPH global_dependencies;
  VECTOR(FIBER) fibers;
  CYCLES cycles;
} STATE;

extern PHY_GRAPH* summary_graph_for(STATE *, Declaration);
extern ATTRSET attrset_for(STATE *, Declaration);

extern Declaration proc_call_p(Expression);

extern int if_rule_p(void*);
extern int if_rule_index(void*);

extern INSTANCE *get_instance(Declaration attr, FIBER fiber,
			      Declaration node, AUG_GRAPH *aug_graph);

extern void assert_closed(AUG_GRAPH*);
extern void dnc_close(STATE *);
extern STATE *compute_dnc(Declaration module);

/* The following routines return TRUE if a change occurs. */
extern int close_augmented_dependency_graph(AUG_GRAPH *);
extern int close_summary_dependency_graph(PHY_GRAPH *);
extern DEPENDENCY analysis_state_cycle(STATE *);

extern void print_instance(INSTANCE *, FILE *);
extern void print_edge(EDGESET, FILE *);
extern void print_edge_helper(DEPENDENCY, CONDITION *, FILE*);
extern void print_edgeset(EDGESET, FILE *);
extern void print_analysis_state(STATE *, FILE *);
extern void print_cycles(STATE *, FILE *);

extern int analysis_debug;
#define ADD_EDGE 16
#define SUMMARY_EDGE 32
#define SUMMARY_EDGE_EXTRA 64
#define CREATE_INSTANCE 128
#define CLOSE_EDGE 256
#define WORKLIST_CHANGES 512
#define DNC_FINAL 1024
#define DNC_ITERATE (1<<11)
#define TWO_EDGE_CYCLE (1<<12)
#define ASSERT_CLOSED (1<<13)
