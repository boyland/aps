#ifndef APS_ANALYZE_H
#define APS_ANALYZE_H

#include <stdbool.h>

#define MODULE_DECL_ANALYSIS_STATE(md) \
	(STATE*)(Declaration_info(md)->analysis_state)

extern bool static_scc_schedule;

extern void analyze_Program(Program); /* decorate modules with STATE */

#endif
