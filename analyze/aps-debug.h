#ifndef APS_DEBUG_H

extern void aps_error(void *tnode, char *fmt, ...);
extern void aps_warning(void *tnode, char *fmt, ...);
extern void aps_check_error(char *type); // exit if errors due to type
extern void set_debug_flags(char *options); // exit when -DH

#endif
