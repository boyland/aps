#ifndef JBB_STRING_H
#define JBB_STRING_H

typedef struct jbb_string *STRING;

extern STRING empty_string;
extern STRING make_string(char *); /* should only be passed a literal */
extern STRING make_saved_string(char *); /* can be passed an array */
extern STRING make_integer_string(int,int); /* integer, base */
extern STRING conc_string(STRING,STRING);
extern STRING make_stub_string(char *default_text);
extern STRING substitute_string(STRING sub, STRING stub, STRING subject);

extern int string_length(STRING);
extern void realize_string(char *, STRING);
/* extern char *temp_realize_string(STRING); */

extern void print_string(FILE *,STRING);

extern void string_error(); /* will be called after an error message */

#endif
