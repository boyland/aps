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
void *table_get(TABLE table, void *key) {
  return get(table,key);
}

void set(TABLE table,void *key, void *value) {
  if (setassoc(key,value,table->alist) != OK) {
    table->alist = acons(key,value,table->alist);
  }
}
void table_set(TABLE table,void *key, void *value) {
  set(table,key,value);
}
