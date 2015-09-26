/* tree.i */
/* John Boyland, 1993, 1994 */

/* 
 * simple tree and list routines.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define INFO void

extern int yylineno; /* lexer keeps up to date */

/* Tree nodes are pretty easy */

typedef struct tree_node *TNODE;

struct tree_node {
  int operator;      /* a unique identifier */
  int line_number;   /* stash the line number when node is made */
  INFO *info;
#ifdef __GNUC__
  void *children[0]; /* structure allocated big enough for each node*/
#else
  void *children[1];
#endif
};

extern int info_size;

static TNODE create_tnode(int operator, int num_children) {
  int i, words;
  TNODE t = (TNODE)malloc(sizeof(struct tree_node)+
			  sizeof(TNODE)*num_children +
			  info_size);
  if (t == NULL) {
    fatal_error("create_tnode could not allocate a node with %d children.",
		num_children);
  }
  t->operator = operator;
  t->line_number = yylineno;
  t->info = ((char *)t)+sizeof(struct tree_node)+sizeof(TNODE)*num_children;
  words = num_children + info_size/sizeof(TNODE*);
  for (i=0; i < words; ++i) {
    t->children[i] = NULL;
  }
  return t;
}

int tnode_line_number(const void *t) {
  return ((TNODE)t)->line_number;
}

INFO *tnode_info(void *t) {
  return ((TNODE)t)->info;
}

void set_tnode_info(void *t, INFO *info) {
  ((TNODE)t)->info = info;
}

/*VARARGS1*/
void
fatal_error(const char *fmt, ...)
{
  va_list args;
  
  va_start(args,fmt);
  
  fflush(stdout);
  (void)  fprintf(stderr, "fatal error: ");
  (void) vfprintf(stderr, fmt, args);
  (void)  fprintf(stderr, "\n");
  (void)   fflush(stderr);
  
  va_end(args);
  abort();
}

