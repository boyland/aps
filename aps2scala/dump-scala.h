#ifndef DUMP_CPP_H
#include <iostream>
#include <string>

using std::ostream;
using std::string;

// don't generate any code for this declaration:
void omit_declaration(char *name);

// The result type is as given:
void impl_module(char *name, char *type);

extern bool incremental;
extern bool static_schedule;
extern int verbose;

class Implementation;

extern Implementation *impl;

void dump_cpp_Program(Program,ostream&,ostream&);
void dump_cpp_Units(Units,ostream&,ostream&);
void dump_cpp_Unit(Unit,ostream&,ostream&);

// If non-zero, this variable means that we
// should do everything in oss.hs, not oss.cpps
extern int inline_definitions;

struct output_streams {
  Declaration context;
  ostream &hs, &cpps, &is;
  string prefix;
  output_streams(Declaration _c, ostream &_hs, ostream &_cpps, ostream &_is,
		 string _p)
    : context(_c), hs(_hs), cpps(_cpps), is(_is), prefix(_p) {}
};

void dump_cpp_Declaration(Declaration,const output_streams&);

static const int indent_multiple = 2;
extern int nesting_level;
string indent();
class InDefinition {
  int saved_nesting;
 public:
  InDefinition(int nn=0) { saved_nesting = nesting_level; nesting_level = nn; }
  ~InDefinition() { nesting_level = saved_nesting; }
};

#define INDEFINITION InDefinition xx(inline_definitions ? nesting_level : 0)

void dump_Type_prefixed(Type,ostream&);
void dump_Type(Type,ostream&);
void dump_Type_value(Type,ostream&);
void dump_Type_signature(Type,ostream&);
void dump_Typed_decl(Type,Declaration,char*prefix,ostream&);
void dump_Expression(Expression,ostream&);
void dump_Use(Use,char *prefix,ostream&);
void dump_TypeEnvironment(TypeEnvironment,ostream&);
void dump_Default(Default,ostream&);

// these two must always be called in pairs: the first
// leaves information around for the second:
void dump_Pattern_cond(Pattern p, string node, ostream&);
void dump_Pattern_bindings(Pattern p, ostream&);

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

inline ostream& operator<<(ostream& o, Default d) 
{
  dump_Default(d,o);
  return o;
}

// wrappers
struct as_sig {
  Type type;
  as_sig(Type t) : type(t) {}
};
struct as_val {
  Type type;
  as_val(Type t) : type(t) {}
};

inline ostream& operator<<(ostream& o, as_sig tas) 
{
  dump_Type_signature(tas.type,o);
  return o;
}

inline ostream& operator<<(ostream& o, as_val tav)
{
  dump_Type_value(tav.type,o);
  return o;
}

#endif
