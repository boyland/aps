extern void compute_oag(Declaration,STATE *);

/** A conditional total order is a tree of cto nodes.
 * There are two kinds of cto nodes:
 * CTO_INSTANCE means a node with an instance
 * that is next to be evaluated.
 * CTO_CONDITION means a node with a condition
 * for which there are two branches.
 *
 * null means the CTO is done.
 */
typedef enum { CTO_INSTANCE, CTO_CONDITION } CTO_TYPE;

typedef struct cto_node CTO_NODE;

struct cto_node {
  CTO_NODE* cto_prev;
  CTO_TYPE cto_type;
  union {
    struct {
      INSTANCE* instance;
      CTO_NODE* next;
    } as_instance;
    struct {
      int cond_index;
      CTO_NODE *if_false, *if_true;
    } as_condition;
  } cto_variant;
};

#define cto_instance cto_variant.as_instance.instance
#define cto_next cto_variant.as_instance.next
#define cto_cond_index cto_variant.as_condition.cond_index
#define cto_cond_if_false cto_variant.as_condition.if_false
#define cto_cond_if_true cto_variant.as_condition.if_true
      
extern int oag_debug;
#define TOTAL_ORDER 1
#define DEBUG_ORDER 2
#define PROD_ORDER 4
#define PROD_ORDER_DEBUG 8
