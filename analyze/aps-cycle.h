/*
 * APS-CYCLE
 *
 * Routines for breaking cycles involving only fibers.
 */

extern void break_fiber_cycles(Declaration,STATE *);
extern int cycle_debug;

typedef struct cycle_description {
  int internal_info;
  VECTOR(INSTANCE) instances;
} CYCLE;

#define PRINT_CYCLE 1
#define DEBUG_UP_DOWN 2

/* information about clauses */
#define CYC_ABOVE 1
#define CYC_LOCAL 2
#define CYC_BELOW 4
