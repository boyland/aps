#include "jbb.h"
#include "jbb-alloc.h"
#include "jbb-table.h"

/* use simple, slow representation for now */
#include "jbb-alist.h"

struct table {
  ALIST alist;
};

TABLE new_table() {
  TABLE t = (TABLE)HALLOC(sizeof(struct table));
  t->alist = NULL;
  return t;
}

void *get(TABLE table, void *key) {
  return assoc(key,table->alist);
}

void set(TABLE table,void *key, void *value) {
  if (setassoc(key,value,table->alist) != OK) {
    table->alist = acons(key,value,table->alist);
  }
}
