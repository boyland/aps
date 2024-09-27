#ifndef DUMP_H
#include <iostream>
#include <string>
#include <vector>

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

class Implementation;

extern Implementation *impl;

static const int indent_multiple = 2;
extern int nesting_level;
string indent(int level = nesting_level);
class InDefinition {
  int saved_nesting;
 public:
  InDefinition(int nn=0) { saved_nesting = nesting_level; nesting_level = nn; }
  ~InDefinition() { nesting_level = saved_nesting; }
};

#ifdef APS2SCALA
/*
 * The Scala generation does everything in one file.
 */
#define GEN_OUTPUT ostream
#define INDEFINITION InDefinition xx(nesting_level)

#else /* APS2SCALA */
/*
 * The C++  generation has separate header and implementation files.
 */
struct output_streams {
  Declaration context;
  ostream &hs, &cpps, &is;
  string prefix;
  output_streams(Declaration _c, ostream &_hs, ostream &_cpps, ostream &_is,
                 string _p)
    : context(_c), hs(_hs), cpps(_cpps), is(_is), prefix(_p) {}
};
#define GEN_OUTPUT output_streams

extern int inline_definitions;
#define INDEFINITION InDefinition xx(inline_definitions ? nesting_level : 0)
#endif /* APS2SCALA */

void dump_Declaration(Declaration,GEN_OUTPUT&);

void dump_Signature(Signature,string,ostream&);
void dump_Type_prefixed(Type,ostream&);
void dump_Type(Type,ostream&);
void dump_Type_value(Type,ostream&);
void dump_Type_signature(Type,ostream&);
void dump_Typed_decl(Type,Declaration,const char*prefix,ostream&);
void dump_Expression(Expression,ostream&);
void dump_Use(Use,const char *prefix,ostream&);
void dump_TypeEnvironment(TypeEnvironment,ostream&);
void dump_vd_Default(Declaration,ostream&);

void dump_function_prototype(string,Type ft, GEN_OUTPUT& oss);
void dump_debug_end(ostream& os);

#ifndef APC2SCALA
// these two must always be called in pairs: the first
// leaves information around for the second:
void dump_Pattern_cond(Pattern p, string node, ostream&);
void dump_Pattern_bindings(Pattern p, ostream&);
string matcher_bindings(string node, Match m);
#endif
void dump_Pattern(Pattern p, ostream&); // only defined for Scala

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

// we have some debugging functions:
extern void debug_Instance(INSTANCE*,ostream&);
inline ostream& operator<<(ostream&os, INSTANCE*i) {
  debug_Instance(i,os);
  return os;
}

extern string operator+(string, int);

#ifndef APS2SCALA
// special C++ generation code
// sending to oss copies to cpps, ...
template <class Any>
inline const output_streams& operator<<(const output_streams& oss, Any x) {
  oss.hs << x;
  if (!inline_definitions) oss.cpps << x;
  return oss;
}

// ... except for a header return type, ...
// (null for constructors)
template <class Type>
struct header_return_type {
  Type rt;
  header_return_type(Type ty) : rt(ty) {}
};
template<>
inline const output_streams& operator<< <header_return_type<Type> >
(const output_streams& oss, header_return_type<Type> hrt) {
  oss.hs << indent();
  if (hrt.rt) oss.hs << hrt.rt;
  if (!inline_definitions) {
    oss.cpps << "\n";
    if (hrt.rt) dump_Type_prefixed(hrt.rt,oss.cpps);
  }
  return oss;
}
template<>
inline const output_streams& operator<< <header_return_type<string> >
(const output_streams& oss, header_return_type<string> hrt) {
  oss.hs << indent();
  oss.hs << hrt.rt;
  if (!inline_definitions) {
    oss.cpps << "\n" << oss.prefix << hrt.rt;
  }
  return oss;
}

// ... and header name ...
struct header_function_name {
  std::string fname;
  header_function_name(std::string fn) : fname(fn) {}
};
template<>
inline const output_streams& operator<< <header_function_name>
(const output_streams& oss, header_function_name hfn) {
  oss.hs << hfn.fname;
  if (!inline_definitions) oss.cpps << oss.prefix << hfn.fname;
  return oss;
}
// ... and header end
struct header_end {
};
template <>
inline const output_streams& operator<< <header_end>
(const output_streams& oss, header_end) {
  if (!inline_definitions) oss.hs << ";\n";
  return oss;
}
#endif /* APS2SCALA */

extern AUG_GRAPH* current_aug_graph;
extern STATE* current_state;

#endif
