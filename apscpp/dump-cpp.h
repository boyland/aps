#ifndef DUMP_CPP_H
#include <iostream>

std::ostream& operator<<(std::ostream&,String);
std::ostream& operator<<(std::ostream&,Symbol);

// don't generate any code for this declaration:
void omit_declaration(char *name);

// The result type is as given:
void impl_module(char *name, char *type);

extern bool incremental;
extern bool static_schedule;
extern int verbose;

void dump_cpp_Program(Program,std::ostream&,std::ostream&);
void dump_cpp_Units(Units,std::ostream&,std::ostream&);
void dump_cpp_Unit(Unit,std::ostream&,std::ostream&);

struct ContextRecordNode {
  Declaration context;
  void *extra; /* branch in context (if any) */
  ContextRecordNode* outer;
  ContextRecordNode(Declaration d, void* e, ContextRecordNode* o)
    : context(d), extra(e), outer(o) {}
};

typedef void (*ASSIGNFUNC)(void *,Declaration,std::ostream&);
void dump_Block(Block,ASSIGNFUNC,void*arg,std::ostream&);
void dump_Type(Type,std::ostream&);
void dump_Type_value(Type,std::ostream&);
void dump_Type_signature(Type,std::ostream&);
void dump_Typed_decl(Type,Declaration,char*prefix,std::ostream&);
void dump_Expression(Expression,std::ostream&);
void dump_Use(Use,char *prefix,std::ostream&);
void dump_TypeEnvironment(TypeEnvironment,std::ostream&);
void dump_Default(Default,std::ostream&);

#endif
