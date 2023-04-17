#ifndef APS_DNC_H
#define APS_DNC_H

#include "jbb-vector.h"
#include "scc.h"
#include <stdbool.h>

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
enum instance_direction fibered_attr_direction(FIBERED_ATTRIBUTE *fa);
enum instance_direction instance_direction(INSTANCE *);

extern BOOL fiber_attr_circular(FIBERED_ATTRIBUTE* fiber_attr);
extern BOOL instance_circular(INSTANCE* in);
extern BOOL decl_is_circular(Declaration);

typedef unsigned DEPENDENCY;

#define SOME_DEPENDENCY 1
#define DEPENDENCY_NOT_JUST_FIBER 2
#define DEPENDENCY_MAYBE_CARRYING 4
#define DEPENDENCY_MAYBE_DIRECT 8
#define DEPENDENCY_MAYBE_SIMPLE 16

#define no_dependency 0
#define dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER|DEPENDENCY_MAYBE_CARRYING|DEPENDENCY_MAYBE_DIRECT)
#define max_dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER|DEPENDENCY_MAYBE_CARRYING|DEPENDENCY_MAYBE_DIRECT|DEPENDENCY_MAYBE_SIMPLE)
#define control_dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER|DEPENDENCY_MAYBE_DIRECT)
#define fiber_dependency (SOME_DEPENDENCY|DEPENDENCY_MAYBE_CARRYING|DEPENDENCY_MAYBE_DIRECT)
#define control_fiber_dependency (SOME_DEPENDENCY|DEPENDENCY_MAYBE_DIRECT)
#define indirect_control_dependency (SOME_DEPENDENCY|DEPENDENCY_NOT_JUST_FIBER)

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
  SCC_COMPONENTS* components; /* SCC components of instances in augmented dependency graph */
  bool* component_cycle;      /* boolean indicating whether SCC component at index is circular */
} AUG_GRAPH;
extern const char *aug_graph_name(AUG_GRAPH *);

typedef struct summary_dependency_graph {
  Declaration phylum;
  struct analysis_state *global_state;
  VECTOR(INSTANCE) instances;
  DEPENDENCY *mingraph; /* two-d array, indexed by instance number */
  struct summary_dependency_graph *next_in_phy_worklist;
  SCC_COMPONENTS* components; /* SCC components of instances in phylum graph */
  bool* component_cycle;      /* boolean indicating whether SCC component at index is circular */
  int *summary_schedule; /* one-d array, indexed by instance number */
  bool* cyclic_flags; /* one-d array, indexed by phase number indicating whether phase is circular or not */
  int max_phase;      /* integer denoting the maximum phase number for this phylum */
  bool* empty_phase;  /* one-d array, indexed by phase number there is no attribute belonging to this phase */
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
  BOOL loop_required;
  DEPENDENCY original_state_dependency;  // This is value of analysis_state_cycle
                                         // before removing fiber cycle or
                                         // linearization of phases in summary graph
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

/* Low level routines: use with caution */
extern void free_edge(EDGESET old, AUG_GRAPH *aug_graph);
extern void free_edgeset(EDGESET es, AUG_GRAPH *aug_graph);
extern void add_edge_to_graph(INSTANCE *source,
			      INSTANCE *sink,
			      CONDITION *cond,
			      DEPENDENCY kind,
			      AUG_GRAPH *aug_graph);

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
#define EDGESET_ASSERTIONS (1<<14)

#endif
