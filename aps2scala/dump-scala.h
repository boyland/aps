#ifndef DUMP_SCALA_H
#include <iostream>
#include <string>

using std::ostream;
using std::string;

// don't generate any code for this declaration:
void omit_declaration(const char *name);

// The result type is as given:
void impl_module(const char *name, const char *type);

extern bool incremental;
extern bool static_schedule;
extern int verbose;
extern int debug;
extern bool include_comments;
extern bool static_scc_schedule;

class Implementation;

extern Implementation *impl;

void dump_scala_Program(Program,ostream&);

void dump_scala_Declaration(Declaration,ostream&);

static const int indent_multiple = 2;
extern int nesting_level;
string indent(int level = nesting_level);
class InDefinition {
  int saved_nesting;
 public:
  InDefinition(int nn=0) { saved_nesting = nesting_level; nesting_level = nn; }
  ~InDefinition() { nesting_level = saved_nesting; }
};

#define INDEFINITION InDefinition xx(nesting_level)

void dump_Signature(Signature,string,ostream&);
void dump_Type_prefixed(Type,ostream&);
void dump_Type(Type,ostream&);
void dump_Type_value(Type,ostream&);
void dump_Type_signature(Type,ostream&);
void dump_Expression(Expression,ostream&);
void dump_Use(Use,const char *prefix,ostream&);
void dump_vd_Default(Declaration,ostream&);

void dump_function_prototype(string,Type ft, bool, ostream& oss);
void dump_debug_end(ostream& os);

// these two must always be called in pairs: the first
// leaves information around for the second:
void dump_Pattern(Pattern p, ostream&);

// override <<
ostream& operator<<(ostream&o,Symbol s);
ostream& operator<<(ostream&o,String s);

inline ostream& operator<<(ostream&o,Expression e)
{
  dump_Expression(e,o); 
  return o;
}

inline ostream& operator<<(ostream&o,Type t)
{
  dump_Type(t,o);
  return o;
}

inline ostream& operator<<(ostream&o, Pattern p)
{
  dump_Pattern(p,o);
  return o;
}

// wrappers
struct as_val {
  Type type;
  as_val(Type t) : type(t) {}
};

inline ostream& operator<<(ostream& o, as_val tav)
{
  dump_Type_value(tav.type,o);
  return o;
}

// we have some debugging functions:
extern void debug_Instance(INSTANCE*,ostream&);
inline ostream& operator<<(ostream&os, INSTANCE*i) {
  debug_Instance(i,os);
  return os;
}

extern string operator+(string, int);

extern bool check_surrounding_decl(void* node, KEYTYPE_Declaration decl_key, Declaration* result_decl);

extern bool should_include_ast_for_objects();

#endif
