#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "aps-ag.h"

static void usage() {
  fprintf(stderr,"apsc: usage: apsc file...\n");
  fprintf(stderr,"             compile APS files\n");
  fprintf(stderr,"*** IN PROGRESS: parsing and (simple) name binding done.\n");
  exit(1);
}

static int aps_error_count = 0;

void aps_error(void *tnode, char *fmt, ...)
{
  va_list args;
  va_start(args,fmt);

  (void)  fprintf(stderr, "%s:%d:",
		  aps_yyfilename,tnode_line_number(tnode));
  (void) vfprintf(stderr, fmt, args);
  (void)  fprintf(stderr, "\n");
  (void)   fflush(stderr);

  va_end(args);
  ++aps_error_count;
}

main(int argc,char **argv) {
  int i;
  for (i=1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      char *options = argv[i]+1;
      if (*options == '\0') usage();
      do {
	switch (*options++) {
	default: usage();
	case 'D':
	  if (*options == '\0') usage();
	  do {
	    switch (*options++) {
	    default: usage(); break;
	    case 'P': bind_debug |= PRAGMA_ACTIVATION; break;
	    case '+': fiber_debug |= ADD_FIBER; break;
	    case 'a': fiber_debug |= ALL_FIBERSETS; break;
	    case 'p': fiber_debug |= PUSH_FIBER; break;
	    case 'f': fiber_debug |= FIBER_INTRO; break;
	    case 'F': fiber_debug |= FIBER_FINAL; break;
	    case 's': fiber_debug |= CALLSITE_INFO; break;
	    case 'i': analysis_debug |= CREATE_INSTANCE; break;
	    case 'e': analysis_debug |= ADD_EDGE; break;
	    case 'c': analysis_debug |= CLOSE_EDGE; break;
	    case 'w': analysis_debug |= WORKLIST_CHANGES; break;
	    case 'x': analysis_debug |= SUMMARY_EDGE_EXTRA; break;
	    case 'E': analysis_debug |= SUMMARY_EDGE; break;
	    case 'D': analysis_debug |= DNC_FINAL; break;
	    case 'I': analysis_debug |= DNC_ITERATE; break;
	    case 'C': cycle_debug |= PRINT_CYCLE; break;
	    case 'O': oag_debug |= TOTAL_ORDER; break;
	    }
	  } while (*options != '\0');
	}
	break;
      } while (*options != '\0');
    } else {
      bind_Program(find_Program(make_string(argv[i])));
    }
  }

  if (aps_error_count != 0) {
    fprintf(stderr,"apsc: quit\n");
    exit(aps_error_count);
  }

  for (i=1; i < argc; ++i) {
    Program p;
    if (argv[i][0] == '-') continue;
    p = find_Program(make_string(argv[i]));
    aps_yyfilename = (char *)program_name(p);
    analyze_Program(p);
  }
  
  exit(aps_error_count);
}


