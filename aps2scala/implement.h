#ifndef IMPLEMENT_H
#define IMPLEMENT_H
// Implementing APS in C++
// Different scheduling algorithms

// This file contains an abstract class that
// is used to schedule attribute grammars expressed
// in APS using C++ templates.

// The main code is in dump-cpp.cc,
// but some parts are different depending on whether 
// we have static or dynamic scheduling.

#include <iostream>
#include <vector>

using std::ostream;
using std::vector;

#define LOCAL_UNIQUE_PREFIX(ld) Def_info(value_decl_def(ld))->unique_prefix

// Abstract class:
class Implementation {
 public:
  virtual ~Implementation() {}

  class ModuleInfo {
  protected:
    Declaration module_decl;
    vector<Declaration> top_level_matches;
    vector<Declaration> attribute_decls;
    vector<Declaration> var_value_decls;
    vector<Declaration> local_attributes;

  public:
    ModuleInfo(Declaration module);
    virtual ~ModuleInfo() {};

    virtual void note_top_level_match(Declaration tlm,
				      ostream& oss);

    virtual void note_local_attribute(Declaration la,
				      ostream& oss);

    virtual void note_attribute_decl(Declaration ad,
				     ostream& oss);

    // Declaration will be declared by caller, but not initialized
    // unless this function desires to:
    virtual void note_var_value_decl(Declaration vd,
				     ostream& oss);

    // implement tlm's and var value decls, and generate finish() routine.
    virtual void implement(ostream& oss) = 0;
  };

  virtual ModuleInfo* get_module_info(Declaration module) = 0;

  // header is done, and indentation is set.
  virtual void implement_function_body(Declaration f, ostream&) = 0;

  // not sure what to do here
  // virtual void implement_procedure(Declaration p, ostream&) = 0;

  // if a Declaration has an implementation mark on it,
  // this function is called to implement its use:
  virtual void implement_value_use(Declaration vd, ostream&) = 0;
};

extern Implementation *dynamic_impl;
extern Implementation *static_impl;

#define IMPLEMENTATION_MARKS (127<<24)

// an implementation may wish to use these flags:
#define LOCAL_ATTRIBUTE_FLAG (1<<24)
#define VAR_VALUE_DECL_FLAG (1<<25)
#define ATTRIBUTE_DECL_FLAG (1<<26)

void clear_implementation_marks(Declaration d);

#endif
