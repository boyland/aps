#include <stdio.h>
#include "aps-tree.h"

static void usage() {
  fprintf(stderr,"apsc: usage: apsc.boot <file.aps>\n");
  fprintf(stderr,"             compile APS to Lisp on stdout\n");
  exit(1);
}

Program the_tree = NULL;
int aps_parse_error = 0;

void dump_lisp_Symbol(Symbol s) {
  char *name = symbol_name(s);
  printf(" aps-boot::|");
  while (*name != '\0') {
    switch (*name) {
    case '\\':
    case '|':
      putchar('\\');
      putchar(*name);
      break;
    default:
      putchar(*name);
      break;
    }
    ++name;
  }
  putchar('|');
}

void dump_lisp_String(String st) {
  /* a disgusting kludge */
  char *s = (char *)st;
  char end = '\0';
  /* fprintf(stderr,"Converting %s\n",s); */
  printf(" \"");
  if (*s== '"') { /* a string */
    ++s;
    end = '"';
  }
  for (; *s != end; ++s) {
    if (s[0] == '\\') {
      ++s;
      if (s[0] == 'n') {
	printf("\n");
      } else {
	printf("\\%c",*s);
      }
    } else if (*s == '"') {
      printf("\\\"");
    } else {
      printf("%c",*s);
    }
  }
  printf("\"");
}

void dump_lisp_Boolean(Boolean b) {
  if (b) printf(" t"); else printf(" nil");
}

int info_size = 0;

main(int argc,char **argv) {
  extern FILE *aps_yyin;
  extern char *aps_yyfilename;
  if (argc != 2) usage();

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

  if ((aps_yyin = fopen(aps_yyfilename=argv[1],"r")) == NULL) {
    fprintf(stderr,"apsc.boot: cannot open file: %s\n",argv[1]);
    exit(1);
  }
  init_lexer(aps_yyin);
  if (aps_yyparse() == 1 || aps_parse_error) {
    fprintf(stderr,"apsc.boot: quit\n");
    exit(1);
  }

  dump_lisp_Program(the_tree);
  printf("\n");
  exit(0);
}

int yywrap()
{
  return 1;
}
