extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "/usr/include/string.h"
#include <strings.h>
#include "aps-ag.h"
}
#include <iostream>
#include <fstream>
#include "dump-cpp.h"

extern "C" 
{
  static int aps_error_count = 0;
  
  void aps_error(void *tnode, char *fmt, ...)
{
  va_list args;
  va_start(args,fmt);
  
  (void)  fprintf(stderr, "%s.aps:%d:",
                  aps_yyfilename,tnode_line_number(tnode));
  (void) vfprintf(stderr, fmt, args);
  (void)  fprintf(stderr, "\n");
  (void)   fflush(stderr);
  
  va_end(args);
  ++aps_error_count;
}  
}

static void usage() {
  fprintf(stderr,"apsc: usage: apsc.boot <file.aps>\n");
  fprintf(stderr,"             compile APS to Lisp on stdout\n");
  exit(1);
}

extern "C" {
extern void init_lexer(FILE*);
extern int aps_yyparse(void);
}

main(int argc,char **argv) {
  if (argc < 2) usage();
  for (int i=1; i < argc; ++i) {
    if (streq(argv[i],"--omit")) {
      omit_declaration(argv[++i]);
      continue;
    } else if (streq(argv[i],"--impl")) {
      impl_module(argv[i+1],argv[i+2]);
      i += 2;
      continue;
    } else if (streq(argv[i],"-Dt")) {
      type_debug = 1;
      continue;
    }
    aps_error_count = 0;
    Program p = find_Program(make_string(argv[i]));
    if (p == 0 || aps_error_count) {
      fprintf(stderr,"compilation halted due to parse errors.\n");
      continue;
    }
    aps_yyfilename = (char*)program_name(p);
    bind_Program(p);
    if (aps_error_count) {
      aps_error(p,"compilation halted due to name errors");
      continue;
    }
    type_Program(p);
    if (aps_error_count) {
      aps_error(p,"compilation halted due to type errors");
      continue;
    }
    char* hfilename = str2cat(argv[i],".h");
    char* cppfilename = str2cat(argv[i],".cpp");

    cout << "hfile = " << hfilename << ", cppfile = " << cppfilename << endl;
    ofstream header(hfilename);
    ofstream body(cppfilename);
    dump_cpp_Program(p,header,body);
  }
  exit(0);
}



