/*
 * APS-CYCLE
 *
 * Routines for breaking cycles involving only fibers.
 */

#ifndef APS_CYCLE_H
#define APS_CYCLE_H

extern void break_fiber_cycles(Declaration,STATE *,DEPENDENCY);
extern int cycle_debug;

typedef struct cycle_description {
  int internal_info;
  DEPENDENCY kind;
  VECTOR(INSTANCE) instances;
} CYCLE;

#define PRINT_CYCLE 1
#define PRINT_UP_DOWN 2
#define DEBUG_UP_DOWN 4

/* information about clauses */
#define CYC_ABOVE 1
#define CYC_LOCAL 2
#define CYC_BELOW 4

#endif
