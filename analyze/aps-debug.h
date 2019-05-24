#ifndef APS_DEBUG_H

extern void aps_clear_errors();
extern void aps_error(const void *tnode, const char *fmt, ...);
extern void aps_warning(const void *tnode, const char *fmt, ...);
extern void aps_check_error(const char *type); // exit if errors due to type
extern void set_debug_flags(const char *options); // exit when -DH

#endif
