/* APS-UTIL
 * utility functions for accessing APS nodes.
 * Basically, they implement patterns in aps-tree.aps
 */
#include <stdio.h>
#include "aps-tree.h"
#include "aps-util.h"

extern char *Declaration_constructors[];

void *replacement_from(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("replacement_from: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_replacement: return class_replacement_class(_node);
  case KEYmodule_replacement: return module_replacement_module(_node);
  case KEYsignature_replacement: return signature_replacement_sig(_node);
  case KEYtype_replacement: return type_replacement_type(_node);
  case KEYvalue_replacement: return value_replacement_value(_node);
  case KEYpattern_replacement: return pattern_replacement_pattern(_node);
  }
}

void *replacement_to(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("replacement_to: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_replacement: return class_replacement_as(_node);
  case KEYmodule_replacement: return module_replacement_as(_node);
  case KEYsignature_replacement: return signature_replacement_as(_node);
  case KEYtype_replacement: return type_replacement_as(_node);
  case KEYvalue_replacement: return value_replacement_as(_node);
  case KEYpattern_replacement: return pattern_replacement_as(_node);
  }
}

Def renaming_def(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("renaming_def: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_renaming: return class_renaming_def(_node);
  case KEYmodule_renaming: return module_renaming_def(_node);
  case KEYsignature_renaming: return signature_renaming_def(_node);
  case KEYtype_renaming: return type_renaming_def(_node);
  case KEYvalue_renaming: return value_renaming_def(_node);
  case KEYpattern_renaming: return pattern_renaming_def(_node);
  }
}

void *renaming_old(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("renaming_old: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_renaming: return class_renaming_old(_node);
  case KEYmodule_renaming: return module_renaming_old(_node);
  case KEYsignature_renaming: return signature_renaming_old(_node);
  case KEYtype_renaming: return type_renaming_old(_node);
  case KEYvalue_renaming: return value_renaming_old(_node);
  case KEYpattern_renaming: return pattern_renaming_old(_node);
  }
}

Use some_use_u(void *_node) {
  switch (ABSTRACT_APS_tnode_phylum(_node)) {
  default: fatal_error("some_use_u: called with bad node");
  case KEYClass: return class_use_use((Class)_node);
  case KEYModule: return module_use_use((Module)_node);
  case KEYSignature: return sig_use_use((Signature)_node);
  case KEYType: return type_use_use((Type)_node);
  case KEYExpression: return value_use_use((Expression)_node);
  case KEYPattern: return pattern_use_use((Pattern)_node);
  }
}

Def formal_def(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("formal_def: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYnormal_formal: return normal_formal_def(_node);
  case KEYseq_formal: return seq_formal_def(_node);
  }
}

Type formal_type(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("formal_type: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYnormal_formal: return normal_formal_type(_node);
  case KEYseq_formal: return seq_formal_type(_node);
  }
}

Def some_type_formal_def(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_type_formal_def: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYtype_formal: return type_formal_def(_node);
  case KEYphylum_formal: return phylum_formal_def(_node);
  }
}

Signature some_type_formal_sig(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_type_formal_sig: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYtype_formal: return type_formal_sig(_node);
  case KEYphylum_formal: return phylum_formal_sig(_node);
  }
}

Def declaration_def(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("declaration_def: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_decl: return class_decl_def(_node);
  case KEYmodule_decl: return module_decl_def(_node);
  case KEYsignature_decl: return signature_decl_def(_node);
  case KEYphylum_decl: return phylum_decl_def(_node);
  case KEYtype_decl: return type_decl_def(_node);
  case KEYvalue_decl: return value_decl_def(_node);
  case KEYattribute_decl: return attribute_decl_def(_node);
  case KEYfunction_decl: return function_decl_def(_node);
  case KEYprocedure_decl: return procedure_decl_def(_node);
  case KEYconstructor_decl: return constructor_decl_def(_node);
  case KEYpattern_decl: return pattern_decl_def(_node);
  case KEYrenaming: return renaming_def(_node);
  case KEYformal: return formal_def(_node);
  case KEYsome_type_formal: return some_type_formal_def(_node);
  case KEYinheritance: return inheritance_def(_node);
  case KEYpolymorphic: return polymorphic_def(_node);
  }
}

Def some_class_decl_def(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_class_decl_def: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_decl: return class_decl_def(_node);
  case KEYmodule_decl: return module_decl_def(_node);
  }
}

Declarations some_class_decl_type_formals(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_class_decl_type_formals: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_decl: return class_decl_type_formals(_node);
  case KEYmodule_decl: return module_decl_type_formals(_node);
  }
}

Declaration some_class_decl_result_type(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_class_decl_result_type: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_decl: return class_decl_result_type(_node);
  case KEYmodule_decl: return module_decl_result_type(_node);
  }
}

Signature some_class_decl_parent(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_class_decl_parent: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_decl: return class_decl_parent(_node);
  case KEYmodule_decl: return module_decl_parent(_node);
  }
}

Block some_class_decl_contents(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_class_decl_contents: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYclass_decl: return class_decl_contents(_node);
  case KEYmodule_decl: return module_decl_contents(_node);
  }
}

Def some_type_decl_def(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_type_decl_def: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYtype_decl: return type_decl_def(_node);
  case KEYphylum_decl: return phylum_decl_def(_node);
  }
}

Signature some_type_decl_sig(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_type_decl_sig: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYtype_decl: return type_decl_sig(_node);
  case KEYphylum_decl: return phylum_decl_sig(_node);
  }
}

Type some_type_decl_type(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_type_decl_type: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYtype_decl: return type_decl_type(_node);
  case KEYphylum_decl: return phylum_decl_type(_node);
  }
}

Type some_value_decl_type(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_value_decl_type: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYvalue_decl: return value_decl_type(_node);
  case KEYattribute_decl: return attribute_decl_type(_node);
  case KEYfunction_decl: return function_decl_type(_node);
  case KEYprocedure_decl: return procedure_decl_type(_node);
  case KEYconstructor_decl: return constructor_decl_type(_node);
  case KEYformal: return formal_type(_node);
  }
}

Expression assign_lhs(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("assign_lhs: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYnormal_assign: return normal_assign_lhs(_node);
  case KEYcollect_assign: return collect_assign_lhs(_node);
  }
}

Expression assign_rhs(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("assign_rhs: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYnormal_assign: return normal_assign_rhs(_node);
  case KEYcollect_assign: return collect_assign_rhs(_node);
  }
}

Def some_function_decl_def(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_function_decl_def: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYfunction_decl: return function_decl_def(_node);
  case KEYprocedure_decl: return procedure_decl_def(_node);
  }
}

Type some_function_decl_type(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_function_decl_type: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYfunction_decl: return function_decl_type(_node);
  case KEYprocedure_decl: return procedure_decl_type(_node);
  }
}

Block some_function_decl_body(Declaration _node) {
  switch (Declaration_KEY(_node)) {
  default: fatal_error("some_function_decl_body: called with %s",
		       Declaration_constructors[Declaration_KEY(_node)]);
  case KEYfunction_decl: return function_decl_body(_node);
  case KEYprocedure_decl: return procedure_decl_body(_node);
  }
}

Expression some_case_stmt_expr(Declaration node) {
  switch (Declaration_KEY(node)) {
  default: fatal_error("some_case_stmt_expr: called with something on line %d",
		       tnode_line_number(node));
  case KEYcase_stmt: return case_stmt_expr(node);
  case KEYfor_stmt: return for_stmt_expr(node);
  }
}

Matches some_case_stmt_matchers(Declaration node) {
  switch (Declaration_KEY(node)) {
  default: fatal_error("some_case_stmt_matchers: called with something on line %d",
		       tnode_line_number(node));
  case KEYcase_stmt: return case_stmt_matchers(node);
  case KEYfor_stmt: return for_stmt_matchers(node);
  }
}
