// declarations for 790-2 (Program Analysis) project
// Dec. 1999
// Yu Wang

#ifndef APS_FIBER_AI_H
#define APS_FIBER_AI_H

#include <string.h>
#include "vector.h"

#define CALLSITE_SET		int
#define MAX_CALLSITE		32

#define DEBUG_INFO	if(SHOW_AI_INFO) printf


int callset_AI(Declaration, struct analysis_state*);
CALLSITE_SET interpret(void *node) ;
void* traverser(void *changed, void *node) ;
void* locater(void *node) ;
void* call_sites_report(void*, void*) ;
void *count_things(void *ref_num, void *node) ;
void *init_things(void *ref_num, void *node) ;
void INCLUDE(CALLSITE_SET*, CALLSITE_SET);
Declaration sth_use_p(Expression);

Declaration local_use_p(Expression expr) ;
void *check_all_decls(void *nouse, void * node) ;
void expr_type(Expression e) ;
void decl_type(Declaration d) ;
void value_use_decl_type(Expression e) ;

#endif