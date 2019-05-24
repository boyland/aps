extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <cstring>
#include <strings.h>
#include "aps-ag.h"
}
#include <iostream>
#include <fstream>
#include "dump-scala.h"
#include "implement.h"
#include "version.h"

using namespace std;

extern "C" {

int callset_AI(Declaration module, STATE *s) { return 0; }

static const char* argv0 = "aps2scala";
void usage() {
  fprintf(stderr,"APS-to-Scala version %s (John Boyland, boyland@uwm.edu)\n",
	  VERSION);
  fprintf(stderr,"             compile APS to Scala\n");
  fprintf(stderr,"%s: usage: %s [-SVG] [-D...] <file.aps>\n",argv0,argv0);
  /*fprintf(stderr,"    -I    generate an incremental implementation\n");*/
  fprintf(stderr,"    -S    generate a scheduled implementation\n");
  fprintf(stderr,"    -D... turn on indicated debugging flags\n");
  fprintf(stderr,"    -DH   list debugging flags\n");
  fprintf(stderr,"    -V    increase verbosity of generation code\n");
  fprintf(stderr,"    -G    add Debug calls for every function\n");
  fprintf(stderr,"    -p path set the APSPATH (overriding env. variable)\n");
  exit(1);
}

extern void init_lexer(FILE*);
extern int aps_yyparse(void);
}

Implementation* impl = dynamic_impl;
bool static_schedule = 0;
int i = 1;
int argc;
char **argv;

void process_next_arg();
void process_arg();

void handle_signal(int signal_num) {
  if (signal_num == SIGABRT) {
    cout << "Static scheduler failed, falling back to dynamic" << endl;
    aps_clear_errors();
    impl = dynamic_impl;
    process_arg();
  }
}


void process_arg() {
    if (streq(argv[i],"--omit")) {
      omit_declaration(argv[++i]);
      process_next_arg();
    } else if (streq(argv[i],"--impl")) {
      impl_module(argv[i+1],argv[i+2]);
      i += 2;
      process_next_arg();
    } else if (streq(argv[i],"-I") || streq(argv[i],"--incremental")) {
      incremental = true;
      process_next_arg();
    } else if (streq(argv[i],"-S") || streq(argv[i],"--static")) {
      impl = static_impl;
      if (!impl) {
	cerr << "Warning: static scheduling not implemented: reverting to dynamic..." << endl;
	impl = dynamic_impl;
      }
      process_next_arg();
    } else if (streq(argv[i],"-V") || streq(argv[i],"--verbose")) {
      ++verbose;
      process_next_arg();
    } else if (streq(argv[i],"-G") || streq(argv[i],"--debug")) {
      ++debug;
      process_next_arg();
    } else if (streq(argv[i],"-p") || streq(argv[i],"--apspath")) {
      set_aps_path(argv[++i]);
      process_next_arg();
    } else if (argv[i][0] == '-' && argv[i][1] == 'D') {
      set_debug_flags(argv[i]+2);
      process_next_arg();
    } else if (argv[i][0] == '-') {
      usage();
    }

    Program p = find_Program(make_string(argv[i]));
    if (p == 0) {
      fprintf(stderr,"Cannot find APS compilation unit %s\n",argv[i]);


    }
    aps_check_error("parse");
    aps_yyfilename = (char*)program_name(p);
    bind_Program(p);
    aps_check_error("binding");
    type_Program(p);
    aps_check_error("type");

    if (impl == static_impl) {
      signal(SIGABRT, &handle_signal);
      analyze_Program(p);
      aps_check_error("analysis");
    }

    char* outfilename = str2cat(argv[i],".scala");
    ofstream out(outfilename);
    dump_scala_Program(p,out);
    process_next_arg();
}

void process_next_arg() {
    i++;
    if (i < argc) {
      process_arg();
    } else {
      exit(0);
    }
}

int main(int c,char **v) {
  argc = c;
  argv = v;
  argv0 = argv[0];
  if (argc < 2) usage();
  process_arg();
}




