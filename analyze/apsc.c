#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "aps-ag.h"

static char* argv0 = "apssched";

void usage() {
  fprintf(stderr,"apsc: usage: %s [-DH] [-D...] [-p apspath] file...\n",argv0);
  fprintf(stderr,"             schedule APS files (omit '.aps' extension)\n");
  fprintf(stderr,"   -DH       print debug options\n");
  exit(1);
}

int main(int argc,char **argv) {
  int i;
  argv0 = argv[0];
  for (i=1; i < argc; ++i) {
	/* printf("argv[%d] = %s\n",i,argv[i]); */
    if (argv[i][0] == '-') {
      char *options = argv[i]+1;
      if (*options == 'D') {
	set_debug_flags(options+1);
      } else if (*options == 'p') {
	set_aps_path(argv[++i]);
      } else usage();
    } else {
      Program p = find_Program(make_string(argv[i]));
      bind_Program(p);
      aps_check_error("binding");
      type_Program(p);
      aps_check_error("type");
      analyze_Program(p);
      aps_check_error("analysis");
    }
  }

  /*
  for (i=1; i < argc; ++i) {
    Program p;
    if (argv[i][0] == '-') continue;
    p = find_Program(make_string(argv[i]));
    aps_yyfilename = (char *)program_name(p);
  }
  */
  
  exit(0);
}


