/*
 * APS-CYCLE
 *
 * Routines for breaking cycles involving only fibers.
 */

extern void break_fiber_cycles(Declaration,STATE *,DEPENDENCY);
extern DEPENDENCY get_edgeset_combine_dependencies(EDGESET es);
extern int cycle_debug;

typedef struct cycle_description {
  int internal_info;
  VECTOR(INSTANCE) instances;
} CYCLE;

#define PRINT_CYCLE 1
#define PRINT_UP_DOWN 2
#define DEBUG_UP_DOWN 4

/* information about clauses */
#define CYC_ABOVE 1
#define CYC_LOCAL 2
#define CYC_BELOW 4