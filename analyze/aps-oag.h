extern void compute_oag(Declaration,STATE *);

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
#define cto_if_false cto_next
};
      
extern int oag_debug;
#define TOTAL_ORDER 1
#define DEBUG_ORDER 2
#define PROD_ORDER 4
#define PROD_ORDER_DEBUG 8
#define TYPE_3_DEBUG 16
