#ifndef JBB_SYMBOL_H
#define JBB_SYMBOL_H

/* define this to be what you want, 
 * and set symbol_info_size to sizeof(SYMBOL_INFO)
 */
#ifndef SYMBOL_INFO
#define SYMBOL_INFO void
#endif

extern int symbol_info_size;
typedef struct symbol *SYMBOL;

extern void init_symbols(); /* call this before anything else,
			       later calls OK, ignored. */

extern SYMBOL find_symbol(const char *text);
extern SYMBOL intern_symbol(const char *text);
extern const char *symbol_name(SYMBOL);
extern int symbol_id(SYMBOL);
extern SYMBOL_INFO *symbol_info(SYMBOL);
extern SYMBOL id_symbol(int);
extern SYMBOL gensym();

#endif
