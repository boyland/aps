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

static void *clear_impl_marks(void *ignore, void *node) {
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration_info((Declaration)node)->decl_flags &= ~IMPLEMENTATION_MARKS;
  }
  return ignore;
}

void clear_implementation_marks(Declaration d) {
  int nothing;
  traverse_Declaration(clear_impl_marks,&nothing,d);
}
