#ifndef DUMP_CPP_H
#include <iostream>

ostream& operator<<(ostream&,String);
ostream& operator<<(ostream&,Symbol);

// don't generate any code for this declaration:
void omit_declaration(char *name);

// The result type is as given:
void impl_module(char *name, char *type);

void dump_cpp_Program(Program,std::ostream&,std::ostream&);
void dump_cpp_Units(Units,std::ostream&,std::ostream&);
void dump_cpp_Unit(Unit,std::ostream&,std::ostream&);
void dump_cpp_Declaration(Declaration,Declaration context,std::ostream&,std::ostream&);

struct ContextRecordNode {
  Declaration context;
  void *extra; /* branch in context (if any) */
  ContextRecordNode* outer;
  ContextRecordNode(Declaration d, void* e, ContextRecordNode* o)
    : context(d), extra(e), outer(o) {}
};

typedef void (*ASSIGNFUNC)(void *,Declaration,ostream&);
void dump_Block(Block,ASSIGNFUNC,void*arg,ostream&);
void dump_Type(Type,ostream&);
void dump_Type_value(Type,ostream&);
void dump_Typed_decl(Type,Declaration,char*prefix,ostream&);
void dump_Expression(Expression,ostream&);
void dump_Use(Use,char *prefix,ostream&);
void dump_TypeEnvironments(Use,ostream&);
void dump_Default(Default,ostream&);

#endif
