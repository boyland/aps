#include <iostream>
extern "C" {
#include <stdio.h>
#include "aps-ag.h"
}
#include "dump-cpp.h"
#include "implement.h"

Implementation::ModuleInfo::ModuleInfo(Declaration module)
  : module_decl(module) 
{}

void Implementation::ModuleInfo::note_top_level_match(Declaration tlm,
						 const output_streams&)
{
  top_level_matches.push_back(tlm);
}

void Implementation::ModuleInfo::note_var_value_decl(Declaration vd,
						const output_streams&)
{
  var_value_decls.push_back(vd);
}

void Implementation::ModuleInfo::note_local_attribute(Declaration ld,
						 const output_streams&)
{
  local_attributes.push_back(ld);
}

void Implementation::ModuleInfo::note_attribute_decl(Declaration ad,
						     const output_streams&)
{
  attribute_decls.push_back(ad);
}
