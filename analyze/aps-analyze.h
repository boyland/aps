#ifndef APS_ANALYZE_H
#define APS_ANALYZE_H

#define MODULE_DECL_ANALYSIS_STATE(md) \
	(STATE*)(Declaration_info(md)->analysis_state)

extern void analyze_Program(Program); /* decorate modules with STATE */

#endif