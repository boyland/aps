typedef struct table *TABLE;

extern TABLE new_table();
#ifndef USING_CXX
extern void *get(TABLE,void *key);
extern void set(TABLE,void *key, void *value);
#endif
extern void *table_get(TABLE,void *key);
extern void table_set(TABLE,void *key, void *value);
