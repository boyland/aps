typedef struct alist *ALIST;
extern ALIST acons(void *key, void *value, ALIST rest);
extern void *assoc(void *key, ALIST alist);
extern void *rassoc(void *value, ALIST alist);

/* modifies the entry, and returns OK, or else returns NOT_OK */
extern BOOL setassoc(void *key, void *value, ALIST alist);

/** accessors legal for non-null ALISTs */
extern void* alist_key(ALIST);
extern void* alist_value(ALIST);
extern ALIST alist_next(ALIST);

