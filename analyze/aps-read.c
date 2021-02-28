/* APS-READ
 * Read APS programs
 * John Boyland
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "jbb.h"
#include "aps-lex.h"
#include "aps-tree.h"
#include "aps-read.h"
#include "aps.tab.h"

#define MAX_PROGRAMS 100

static int num_programs = 0;
static Program programs[MAX_PROGRAMS];

extern FILE *aps_yyin;
extern char *aps_yyfilename;
Program the_tree = NULL;
int aps_parse_error = 0;

static char** aps_path = 0;

void set_aps_path(char *path) {
  int i,n = 1;
  char *p;
  for (p=path; *p; ++p) {
    if (*p == ':') ++n;
  }
  aps_path = newblocks(n+1,char*);
  for (i=0,p=path; i<n; ++i) {
    char *q = strchr(p,':');
    int s = (q == 0) ? strlen(p) : (q-p);
    aps_path[i] = strnsave(p,s);
    aps_path[i][s] = '\0';
    /* printf("APSPATH includes %s\n",aps_path[i]); */
    p=q+1;
  }
}

static FILE *aps_open(char *filename) {
  char buf[MAXPATHLEN+1];
  char **pi;
  if (!aps_path) {
    char *path = getenv("APSPATH");
    if (path != NULL) set_aps_path(path);
    else set_aps_path(".");
  }
  for (pi = aps_path; *pi; ++pi) {
    int m = strlen(*pi) + strlen(filename);
    FILE *f;
    if (m >= MAXPATHLEN) continue;
    strcpy(buf,*pi);
    strcat(buf,"/");
    strcat(buf,filename);
    f = fopen(buf,"r");
    if (f != NULL) return f;
  }
  return 0;
}


static int initialized = FALSE;
static int initialize() {
  if (!initialized) {
    /* get the builtin operator precedences */
    if ((aps_yyin = aps_open(aps_yyfilename="aps-builtin.aps")) == NULL) {
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
  return initialized;
}

Program find_Program(String name) {
  int i;
  /* printf("Finding %s\n",name);
   * if (*(char*)name == '.') abort();
   */
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
  if ((aps_yyin = aps_open(aps_yyfilename)) == NULL) {
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

