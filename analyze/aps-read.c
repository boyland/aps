/* APS-READ
 * Read APS programs
 * John Boyland
 */
#include <stdio.h>
#include "jbb.h"
#include "aps-tree.h"
#include "aps-read.h"

#define MAX_PROGRAMS 100

static int num_programs = 0;
static Program programs[MAX_PROGRAMS];

extern FILE *aps_yyin;
extern char *aps_yyfilename;
Program the_tree = NULL;
int aps_parse_error = 0;

static int initialized = FALSE;
static int initialize() {
  if (!initialized) {
    /* get the builtin operator precedences */
    if ((aps_yyin = fopen(aps_yyfilename="aps-builtin.aps","r")) == NULL) {
      fprintf(stderr,"apsc.boot: cannot open builtins: aps-builtin.aps\n");
      exit(1);
    }
    init_lexer(aps_yyin);
    if (aps_yyparse() == 1 || aps_parse_error) {
      fprintf(stderr,"apsc.boot: builtins had syntax errors\n");
      exit(1);
    }
    initialized=TRUE;
  }
}

Program find_Program(String name) {
  int i;
  initialize();
  for (i=0; i < num_programs; ++i) {
    if (streq((char *)name,(char *)program_name(programs[i]))) {
      return programs[i];
    }
  }
  /* printf("Reading %s.aps\n",name); */
  if (num_programs == MAX_PROGRAMS) {
    fprintf(stderr,"apsc: too many source files (max %d)\n", MAX_PROGRAMS);
    exit(1);
  }
  aps_yyfilename=str2cat((char *)name,".aps");
  if ((aps_yyin = fopen(aps_yyfilename,"r")) == NULL) {
    fprintf(stderr,"apsc: cannot open file: %s\n",aps_yyfilename);
    exit(1);
  }
  init_lexer(aps_yyin);
  if (aps_yyparse() == 1 || aps_parse_error) {
    fprintf(stderr,"apsc: quit\n");
    exit(1);
  }
  programs[num_programs++] = the_tree;
  free(aps_yyfilename);
  aps_yyfilename=NULL;
  return the_tree;
}

int yywrap()
{
  return 1;
}

