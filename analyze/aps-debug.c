#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include "aps-ag.h"

static int aps_error_count = 0;
  
void aps_error(const void *tnode, const char *fmt, ...)
{
  va_list args;
  va_start(args,fmt);
  
  fflush(stdout);

  (void)  fprintf(stderr, "%s.aps:%d:",
                  aps_yyfilename,tnode_line_number(tnode));
  (void) vfprintf(stderr, fmt, args);
  (void)  fprintf(stderr, "\n");
  (void)   fflush(stderr);
  
  va_end(args);
  ++aps_error_count;
}  

void aps_warning(const void *tnode, const char *fmt, ...)
{
  va_list args;
  va_start(args,fmt);
  
  fflush(stdout);

  (void)  fprintf(stderr, "%s.aps:%d:Warning: ",
                  aps_yyfilename,tnode == NULL ? -1 : tnode_line_number(tnode));
  (void) vfprintf(stderr, fmt, args);
  (void)  fprintf(stderr, "\n");
  (void)   fflush(stderr);
  
  va_end(args);
}  

void aps_check_error(const char *type)
{
  if (aps_error_count) {
    fflush(stdout);
    fprintf(stderr,"Compilation stopped");
    if (type) fprintf(stderr," due to %s errors.\n",type);
    else fputs(".\n",stderr);
    exit(1);
  }
}

extern void usage();

static void list_debug_flags() {
  fprintf(stderr,"The following flags may be listed after -D\n");
  fprintf(stderr,"Capital letters signify more important information\n");
  fprintf(stderr,"\tP  PRAGMA_ACTIVATION\n");
  fprintf(stderr,"\tt  TYPE CHECKING\n");
  fprintf(stderr,"\t+  ADD_FIBER\n");
  fprintf(stderr,"\ta  ADD_FSA_EDGE");
  fprintf(stderr,"\tA  ALL_FIBERSETS\n");
  fprintf(stderr,"\tp  PUSH_FIBER\n");
  fprintf(stderr,"\tf  FIBER_INTRO\n");
  fprintf(stderr,"\tF  FIBER_FINAL\n");
  /* fprintf(stderr,"\ts  CALLSITE_INFO\n"); */
  fprintf(stderr,"\ti  CREATE_INSTANCE\n");
  fprintf(stderr,"\te  ADD_EDGE\n");
  fprintf(stderr,"\tc  CLOSE_EDGE\n");
  fprintf(stderr,"\tw  WORKLIST_CHANGES\n");
  fprintf(stderr,"\tx  SUMMARY_EDGE_EXTRA\n");
  fprintf(stderr,"\t0  ASSERT_CLOSED\n");
  fprintf(stderr,"\t@  EDGESET_ASSERTIONS\n");
  fprintf(stderr,"\tE  SUMMARY_EDGE\n");
  fprintf(stderr,"\t2  TWO_EDGE_CYCLE\n");
  fprintf(stderr,"\tD  DNC_FINAL\n");
  fprintf(stderr,"\tI  DNC_ITERATE\n");
  fprintf(stderr,"\tC  PRINT_CYCLE\n");
  fprintf(stderr,"\tu  DEBUG_UP_DOWN\n");
  fprintf(stderr,"\tU  PRINT_UP_DOWN\n");
  fprintf(stderr,"\to  DEBUG_ORDER\n");
  fprintf(stderr,"\tO  TOTAL_ORDER\n");
  fprintf(stderr,"\tT  PROD_ORDER\n");
  fprintf(stderr,"\t3  TYPE_3_DEBUG\n");
  exit(0);
}

void set_debug_flags(const char *options)
{
  if (*options == '\0') usage();
  do {
    /* fprintf(stderr,"debug option: '%c'\n",*options); */
    switch (*options++) {
    default: usage(); break;
    case 'H': list_debug_flags(); break;
    case 'P': bind_debug |= PRAGMA_ACTIVATION; break;
    case 't': type_debug = -1; break;
    case '+': fiber_debug |= ADD_FIBER; break;
    case 'a': fiber_debug |= ADD_FSA_EDGE; break;
    case 'A': fiber_debug |= ALL_FIBERSETS; break;
    case 'p': fiber_debug |= PUSH_FIBER; break;
    case 'f': fiber_debug |= FIBER_INTRO; break;
    case 'F': fiber_debug |= FIBER_FINAL; break;
    case 's': fiber_debug |= CALLSITE_INFO; break;
    case 'i': analysis_debug |= CREATE_INSTANCE; break;
    case 'e': analysis_debug |= ADD_EDGE; break;
    case 'c': analysis_debug |= CLOSE_EDGE; break;
    case 'w': analysis_debug |= WORKLIST_CHANGES; break;
    case 'x': analysis_debug |= SUMMARY_EDGE_EXTRA; break;
    case '2': analysis_debug |= TWO_EDGE_CYCLE; break;
    case '0': analysis_debug |= ASSERT_CLOSED; break;
    case 'E': analysis_debug |= SUMMARY_EDGE; break;
    case 'D': analysis_debug |= DNC_FINAL; break;
    case 'I': analysis_debug |= DNC_ITERATE; break;
    case '@': analysis_debug |= EDGESET_ASSERTIONS; break;
    case 'C': cycle_debug |= PRINT_CYCLE; break;
    case 'u': cycle_debug |= DEBUG_UP_DOWN; break;
    case 'U': cycle_debug |= PRINT_UP_DOWN; break;
    case 'o': oag_debug |= DEBUG_ORDER; break;
    case 'O': oag_debug |= TOTAL_ORDER; break;
    case 'v': oag_debug |= DEBUG_ORDER_VERBOSE; break;
    case 'T': oag_debug |= PROD_ORDER; break;
    case '3': oag_debug |= TYPE_3_DEBUG; break;
    }
  } while (*options != '\0');
}

