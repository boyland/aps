typedef struct table *TABLE;

extern TABLE new_table();
extern void *get(TABLE,void *key);
extern void set(TABLE,void *key, void *value);
