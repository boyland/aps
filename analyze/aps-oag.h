extern void compute_oag(Declaration,STATE *);

/**
 * Return phase (synthesized) or -phase (inherited)
 * for fibered attribute, given the phylum's summary dependence graph.
 */
extern int attribute_schedule(PHY_GRAPH *phy_graph, FIBERED_ATTRIBUTE* key);

typedef struct child_phase_type CHILD_PHASE;

struct child_phase_type
{ 
  short ph; // Phase: ph is negative for inherited attributes of the visit/phase, positive for synthesized attributes
  short ch; // Child number: ch is -1 for parent, and otherwise [0,nch)
};

/** A conditional total order is a tree of cto nodes.
 * null means the CTO is done.
 *
 * If the instance is an if_rule, it will have two children
 * otherwise, just one.
 */
typedef struct cto_node CTO_NODE;

struct cto_node {
  CTO_NODE* cto_prev;
  INSTANCE* cto_instance;
  CTO_NODE* cto_next;
  CTO_NODE* cto_if_true;
  CHILD_PHASE child_phase;
  Declaration child_decl;
#define cto_if_false cto_next
};
      
extern int oag_debug;
#define TOTAL_ORDER 1
#define DEBUG_ORDER 2
#define PROD_ORDER 4
#define PROD_ORDER_DEBUG 8
#define TYPE_3_DEBUG 16
