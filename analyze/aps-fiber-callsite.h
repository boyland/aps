// declarations for 790-2 (Program Analysis) project
// Dec. 1999
// Yu Wang

#include <string.h>
#include "vector.h"

#define CALLSITE_SET		int
#define MAX_CALLSITE		32

#define DEBUG_INFO	if(SHOW_AI_INFO) printf


int callset_AI(Declaration, struct analysis_state*);
CALLSITE_SET interpret(void *) ;
void* traverser(void *changed, void *) ;
void* locater(void *node) ;
void* call_sites_report(void*, void*) ;
void *count_things(void *, void *) ;
void *init_things(void *, void *) ;
Declaration sth_use_p(Expression);
Declaration local_use_p(Expression) ;

int callsite_set_empty_p(CALLSITE_SET) ;
CALLSITE_SET empty_callsite_set() ;
void INCLUDE(CALLSITE_SET*, CALLSITE_SET);
int assign_sets(CALLSITE_SET , void* , CALLSITE_SET ) ;
