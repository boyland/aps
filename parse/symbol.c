#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "jbb.h"
#include "jbb-symbol.h"

/* stupid slow version for now */

struct symbol {
  const char *name;
#ifdef __GNUC__
  void *info[0];
#else
  void *info[1]; /* waste space */
#endif
};

#define MAX_SYMBOL 4000

static int symbol_size = 0;
void *symbols;
#define SYMBOL_NUM(i) (*(SYMBOL)((char *)symbols+((i)*(symbol_size))))
int num_symbols = 0;

void init_symbols() {
  if (symbol_size == 0) {
    symbol_size = sizeof(struct symbol)+symbol_info_size;
    symbols = (void *)calloc(symbol_size,MAX_SYMBOL);
  }
}

SYMBOL find_symbol(const char *s) {
  int i;
  for (i=0; i < num_symbols; ++i) {
    if (streq(SYMBOL_NUM(i).name,s)) return &SYMBOL_NUM(i);
  }
  return NULL;
}

static SYMBOL add_symbol(const char *s) {
  SYMBOL sym;
  if (num_symbols == MAX_SYMBOL) {
    fprintf(stderr,"symbol table full\n");
    exit(1);
  }
  SYMBOL_NUM(num_symbols).name = strsave(s);
  sym = &SYMBOL_NUM(num_symbols);
  ++num_symbols;

  /*  fprintf(stderr,"Interned \"%s\" as #%d\n",
   *          symbol_name(sym),symbol_id(sym));
   */
  
  return sym;
}

SYMBOL intern_symbol(const char *s) {
  SYMBOL sym = find_symbol(s);
  if (sym == NULL) {
    return add_symbol(s);
  } else {
    return sym;
  }
}

const char *symbol_name(SYMBOL s) {
  return s->name;
}

int symbol_id(SYMBOL s) {
  return ((char *)s-(char *)symbols)/symbol_size;
}

SYMBOL id_symbol(int i) {
  return &SYMBOL_NUM(i);
}

SYMBOL_INFO *symbol_info(SYMBOL s) {
  return (SYMBOL_INFO *)&s->info;
}

SYMBOL gensym() { /* not quite CL, more like gentemp */
  static char name[10]; /* precisely big enough */
  static int unique = 0;
  do {
    sprintf(name,"G%06d",++unique);
  } while (find_symbol(name) != NULL);
  return add_symbol(name);
}
