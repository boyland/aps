#include <iostream>
#include <cctype>
#include <stack>
#include <map>
#include <sstream>
#include <vector>
#include <string>

extern "C" {
#include <stdio.h>
#include <string.h>
#include "aps-ag.h"
String get_code_name(Symbol);
}

#include "dump-cpp.h"
#include "implement.h"

using namespace std;

using std::string;

// extra decl_flags flags:
#define IMPLEMENTED_PATTERN_VAR (1<<20)

extern int aps_yylineno;

ostream& operator<<(ostream&o,Symbol s)
{
  String str = get_code_name(s);
  if (str == NULL) o << symbol_name(s);
  else o << get_code_name(s);
  return o;
}

ostream& operator<<(ostream&o,String s)
{
  if (s == NULL) {
    o << "<NULL>";
    return o;
  }
  int n = string_length(s);
  // char *buf = new char[n+1];
  char buf[n+1];
  realize_string(buf,s);
  return o << buf;
  // delete buf;
}

void print_uppercase(String sn,ostream&os)
{
  for (char *s=(char*)sn; *s; ++s) {
    if (islower(*s)) os<< (char)toupper(*s);
    else if (*s == '-') os << "_";
    else os << *s;
  }
}

String get_prefix(Declaration decl);

// parameterizations and options:

static char* omitted[80];
static int omitted_number = 0;

void omit_declaration(char *n)
{
  omitted[omitted_number++] = n;
}

static char*impl_types[80];
static int impl_number = 0;

void impl_module(char *mname, char*type)
{
  impl_types[impl_number++] = mname;
  impl_types[impl_number++] = type;
}

bool incremental = false; //! unused
int verbose = 0;
int debug = 0;

int inline_definitions = 0;

void dump_cpp_Program(Program p,std::ostream&hs,std::ostream&cpps)
{
  String name=program_name(p);
  inline_definitions = 0;
  aps_yyfilename = (char *)program_name(p);
  hs << "#ifndef "; print_uppercase(name,hs); hs << "_H " << endl;
  hs << "#define "; print_uppercase(name,hs); hs << "_H " << endl;
  cpps << "#include \"aps-impl.h\"" << endl;
  if (!streq(aps_yyfilename,"basic"))
    cpps << "#include \"basic.h\"" << endl;
  cpps << "#include \"" << name << ".h\"\n\n";
  dump_cpp_Units(program_units(p),hs,cpps);
  hs << "#endif" << endl;
}

void dump_cpp_Units(Units us,std::ostream&hs,std::ostream&cpps)
{
  switch (Units_KEY(us)) {
  case KEYnil_Units: break;
  case KEYlist_Units:
    dump_cpp_Unit(list_Units_elem(us),hs,cpps);
    break;
  case KEYappend_Units:
    dump_cpp_Units(append_Units_l1(us),hs,cpps);
    dump_cpp_Units(append_Units_l2(us),hs,cpps);
  }
}

void dump_cpp_Unit(Unit u,std::ostream&hs,std::ostream&cpps)
{
  ostringstream is;
  switch(Unit_KEY(u)) {
  case KEYno_unit: break;
  case KEYwith_unit:
    {
      String name = with_unit_name(u);
      int n = string_length(name);
      char buf[n+1];
      realize_string(buf,name);
      buf[n-1] = '\0'; // clear final quote
      hs << "#include \"" << buf+1 << ".h\"" << endl;
    }
    break;
  case KEYdecl_unit:
    dump_cpp_Declaration(decl_unit_decl(u),
			 output_streams(NULL,hs,cpps,is,""));
    break;
  }
}

Declaration constructor_decl_base_type_decl(Declaration decl)
{
  Type t = constructor_decl_type(decl);
  Declaration returndecl = first_Declaration(function_type_return_values(t));
  Type return_type = value_decl_type(returndecl);
  Declaration tdecl = USE_DECL(type_use_use(return_type));
  return tdecl;
}

void dump_formal(Declaration formal,const char *prefix,ostream&s)
{
  dump_Typed_decl(infer_formal_type(formal),formal,prefix,s);
  if (KEYseq_formal == Declaration_KEY(formal)) s << ",...";
}

void dump_function_prototype(string prefix,char *name, Type ft, ostream&hs)
{
  Declarations formals = function_type_formals(ft);
  Declaration returndecl = first_Declaration(function_type_return_values(ft));
  if (returndecl == 0) {
    hs << "void ";
  } else {
    Type return_type = value_decl_type(returndecl);
    if (prefix != "") {
      dump_Type_prefixed(return_type,hs);
    } else {
      dump_Type(return_type,hs);
    }
    if (DECL_NEXT(returndecl)) {
      aps_error(ft,"cannot handle multiple return values");
    }
  }
  hs << " " << prefix;
  hs << "v_" << name << "(";
  for (Declaration formal = first_Declaration(formals);
       formal != NULL;
       formal = DECL_NEXT(formal)) {
    if (formal != first_Declaration(formals))
      hs << ",";
    dump_formal(formal,"v_",hs);
  }
  hs << ")";
}

void dump_function_prototype(char *name, Type ft, const output_streams& oss)
{
  Declarations formals = function_type_formals(ft);
  Declaration returndecl = first_Declaration(function_type_return_values(ft));
  Type rt;
  if (returndecl == 0) {
    rt = 0;
  } else {
    rt = value_decl_type(returndecl);
    if (DECL_NEXT(returndecl)) {
      aps_error(ft,"cannot handle multiple return values");
    }
  }
  oss << header_return_type<Type>(rt) << " "
      << header_function_name("v_") << name << "(";
  for (Declaration formal = first_Declaration(formals);
       formal != NULL;
       formal = DECL_NEXT(formal)) {
    if (formal != first_Declaration(formals))
      oss << ",";
    oss << infer_formal_type(formal) << " v_" << decl_name(formal);
  }
  oss << ")" << header_end();
}

void dump_function_debug(char *name, Type ft, ostream& os)
{
  Declarations formals = function_type_formals(ft);
  os << indent() << "Debug debug(std::string(\"" << name << "(\")";
  bool started = false;
  for (Declaration formal = first_Declaration(formals);
       formal != NULL;
       formal = DECL_NEXT(formal)) {
    if (started) os << "+\",\""; else started = true;
    os << "+v_" << decl_name(formal);
  }
  os << "+\")\");\n";
}

int nesting_level = 0;
string indent(int nl) { return string(indent_multiple*nl,' '); }

// Output code to test if pattern matches node (no binding)
// As a side-effect, information is left in place to enable
// dump_Pattern_bindings to work.
void dump_Pattern_cond(Pattern p, string node, ostream& os);

void dump_seq_function(Type et, Type st, string func_name, ostream& os) 
{
  os << "C__basic_";
  if (func_name == "first" || func_name == "last") {
    os << "16";
  } else if (func_name == "butfirst" || func_name == "butlast") {
    os << "17";
  } else if (func_name == "length" || func_name == "empty") {
    os << "22";
  }
  os << "<";
  dump_Type_signature(et,os);
  os << ",";
  dump_Type_signature(st,os);
  os << ">(";
  dump_Type_value(et,os);
  os << ",";
  dump_Type_value(st,os);
  os << ").v_" << func_name;
}

void dump_seq_Pattern_cond(Pattern pa, Type st, string node, ostream& os)
{
  if (pa == 0) {
    dump_seq_function(type_element_type(st),st,"empty",os);
    os << "(" << node << ")";
  } else {
    Pattern next_pa = PAT_NEXT(pa);
    switch (Pattern_KEY(pa)) {
    case KEYrest_pattern:
      if (Pattern_KEY(rest_pattern_constraint(pa)) != KEYno_pattern) {
	aps_error(pa,"Cannot handle complicated ... patterns");
      }
      if (next_pa) {
	if (PAT_NEXT(next_pa)) {
	  aps_error(next_pa,"Sequence pattern too complicated for now");
	} else {
	  ostringstream ns;
	  dump_seq_function(infer_pattern_type(pa),st,"last",ns);
	  ns << "(" << node << ")";	  
	  dump_Pattern_cond(next_pa,ns.str(),os);
	}
      } else {
	os << "true";
      }
      break;
    default:
      {	
	Type pat = infer_pattern_type(pa);
	os << "!";
	dump_seq_function(pat,st,"empty",os);
	os << "(" << node << ")&&";
	ostringstream ns;
	dump_seq_function(pat,st,"first",ns);
	ns << "(" << node << ")";	
	dump_Pattern_cond(pa,ns.str(),os);
	os << "&&";
	ostringstream rs;
	dump_seq_function(pat,st,"butfirst",rs);
	rs << "(" << node << ")";
	dump_seq_Pattern_cond(next_pa,st,rs.str(),os);
      }
      break;
    }
  }
}

void dump_Pattern_cond(Pattern p, string node, ostream& os) 
{
  switch (Pattern_KEY(p)) {
  default:
    aps_error(p,"Cannot implement this kind of pattern");
    break;
  case KEYmatch_pattern:
    os << node << "->type==";
    dump_Type_value(match_pattern_type(p),os);
    os <<"->get_type() && ";
    dump_Pattern_cond(match_pattern_pat(p),node,os);
    break;
  case KEYpattern_call:
    {
      static Symbol seq_symbol = intern_symbol("{}");
      static Symbol append_symbol = intern_symbol("append");
      static Symbol single_symbol = intern_symbol("single");
      static Symbol none_symbol = intern_symbol("none");
      Pattern pf = pattern_call_func(p);
      Type rt = infer_pattern_type(p);
      Type pft = infer_pattern_type(pf);
      PatternActuals pactuals = pattern_call_actuals(p);
      Use pfuse;
      switch (Pattern_KEY(pf)) {
      default:
	aps_error(p,"cannot find constructor (can only handle ?x=f(...) or f(...)");
	return;
      case KEYpattern_use:
	pfuse = pattern_use_use(pf);
	break;
      }
      Declaration pfdecl = USE_DECL(pfuse);
      Symbol pfname = def_name(declaration_def(pfdecl));
      if (pfname == seq_symbol) {
	dump_seq_Pattern_cond(first_PatternActual(pactuals),rt,node,os);
	return;
      } else if (Declaration_KEY(pfdecl) != KEYconstructor_decl) {
	if (pfname != append_symbol &&
	    pfname != single_symbol &&
	    pfname != none_symbol) {
	  aps_error(pfuse,"Cannot handle calls to pattern functions");
	}
      }
      os << node << "->cons==this->"; // added "this->" because of C++ compiler
      dump_Use(pfuse,"c_",os);

      ostringstream ts;
      ts << "((";
      dump_TypeEnvironment(USE_TYPE_ENV(pfuse),ts);
      ts << "V_" << decl_name(pfdecl) << "*)" << node << ")";
      string typed_node = ts.str();

      Pattern pa = first_PatternActual(pactuals);
      Declaration pff = first_Declaration(function_type_formals(pft));
      for ( ; pa && pff; pa = PAT_NEXT(pa), pff = DECL_NEXT(pff)) {
	os << "&&";
	dump_Pattern_cond(pa,typed_node+"->v_"+decl_name(pff),os);
      }
    }
    break;
  case KEYand_pattern:
    dump_Pattern_cond(and_pattern_p1(p),node,os);
    os <<"&&";
    dump_Pattern_cond(and_pattern_p2(p),node,os);
    break;
  case KEYpattern_var:
    {
      Declaration f = pattern_var_formal(p);
      static Symbol underscore_symbol = intern_symbol("_");
      if (def_name(formal_def(f)) != underscore_symbol) {
	Declaration_info(f)->decl_flags |= IMPLEMENTED_PATTERN_VAR;
	struct Pattern_info *pi = Pattern_info(p);
	pi->pat_impl = new string();
	*((string *)pi->pat_impl) = node;
      }
      os << "true";
    }
    break;
  case KEYcondition:
    dump_Expression(condition_e(p),os);
    break;
  }
}

// Print out the bindings to be generated by a pattern match.
// (Can only be called after dump_Pattern_cond)
void dump_Pattern_bindings(Pattern p, ostream& os)
{
  switch (Pattern_KEY(p)) {
  default:
    aps_error(p,"Cannot handle this kind of pattern");
    break;
  case KEYmatch_pattern:
    dump_Pattern_bindings(match_pattern_pat(p),os);
    break;
  case KEYpattern_call:
    for (Pattern pa = first_PatternActual(pattern_call_actuals(p));
	 pa; pa=PAT_NEXT(pa)) {
      dump_Pattern_bindings(pa,os);
    }
    break;
  case KEYrest_pattern:
    break;
  case KEYand_pattern:
    dump_Pattern_bindings(and_pattern_p1(p),os);
    dump_Pattern_bindings(and_pattern_p2(p),os);
    break;
  case KEYpattern_var:
    {
      Declaration f = pattern_var_formal(p);
      if (Declaration_info(f)->decl_flags & IMPLEMENTED_PATTERN_VAR) {
	Declaration_info(f)->decl_flags &= ~IMPLEMENTED_PATTERN_VAR;
	struct Pattern_info *pi = Pattern_info(p);
	string *ps = (string *)pi->pat_impl;
	pi->pat_impl = 0;
	os << indent();
	dump_Typed_decl(infer_formal_type(f),f,"v_",os);
	os << " = " << *ps << ";\n";
	delete ps;
      }
    }
    break;
  case KEYcondition:
    break;
  }
}

string matcher_bindings(string node, Match m)
{
  Pattern p = matcher_pat(m);
  ostringstream os1, os2;
  dump_Pattern_cond(p,node,os1);
  dump_Pattern_bindings(p,os2);
  // ignore os1 contents
  return os2.str();
}


bool template_decl_p(Declaration decl)
{
  switch (Declaration_KEY(decl)) {
  case KEYmodule_decl:
    {
      Declarations type_formals = module_decl_type_formals(decl);
      if (first_Declaration(type_formals) != NULL) return true;
    }
    break;
  case KEYclass_decl:
    return true;
    break;
  default:
    break;
  }
  return false;
}

// true for a module decl which is a simple mixin:
// [T :: ...] extends T
// there may be multiple parent classes.
// This code does work for multiple type formals,
// but later code does not.
Declaration simple_mixin_p(Declaration decl)
{
  if (Declaration_KEY(decl) == KEYmodule_decl) {
    Declarations tfs = module_decl_type_formals(decl);
    for (Declaration tf = first_Declaration(tfs);
	 tf != NULL; tf = DECL_NEXT(tf)) {
      if (TYPE_FORMAL_IS_EXTENSION(tf)) return tf;
    }
  }
  return 0;
}

String get_prefix(Declaration decl)
{
  static String saved_prefix = 0;
  static Declaration saved_decl = 0;
  if (decl == saved_decl) return saved_prefix;
  String prefix = empty_string;
  for (void *p=tnode_parent(decl); p!=NULL; p=tnode_parent(p)) {
    if (ABSTRACT_APS_tnode_phylum(p) == KEYDeclaration) {
      char *name = decl_name((Declaration)p);
      int n = strlen(name);
      char buf[n+1];
      strcpy(buf,name);
      for (int i=0; i < n; ++i)
	if (buf[i] == '-') buf[i] = '_';
      String pre = conc_string(conc_string(make_string("C_"),
					   make_saved_string(buf)),
			       make_string("::"));
#ifdef UNDEF
/* We now just generate definitions inline */
      if (template_decl_p(decl)) {
	has_templates = true; // remember to #include .cpp file
	String tpart = make_string("template <");
	bool started = false;
	Declarations tfs = some_class_decl_type_formals(decl);
	for (Declaration tf = first_Declaration(tfs);
	     tf != NULL; tf=DECL_NEXT(tf))
	  if (started) tpart = conc_string(tpart,make_string(", class T_"));
	  else { started = true; tpart = conc_string(tpart,make_string("class T_")); }
	tpart = conc_string(tpart,make_string(">\n"));
	pre = conc_string(tpart,pre);
      }
#endif
      prefix = conc_string(pre,prefix);
    }
  }
  saved_prefix = prefix;
  saved_decl = decl;
  return prefix;
}

/*
 * We have three ways of expressing an APS type
 * 1> As a "signature" or more properly, module type.
 *    C_name
 * 2> As a "type", the type of the values of the type
 *    T_name == C_name::T_Result
 * 3> As a "value", the instance of the module type:
 *    C_name *t_name
 */

void dump_Type_signature(Type ty, ostream& os)
{
  switch (Type_KEY(ty)) {
  case KEYprivate_type:
    dump_Type_signature(private_type_rep(ty),os);
    break;
  case KEYremote_type:
    dump_Type_signature(remote_type_nodetype(ty),os);
    break;
  case KEYno_type:
    if (Declaration_KEY((Declaration)tnode_parent(ty)) == KEYphylum_decl)
      os << "C_PHYLUM";
    else
      os << "C_TYPE";
    break;
  case KEYtype_inst:
    {
      char *mname = (char *)symbol_name(use_name(module_use_use(type_inst_module(ty))));
      os << "C_" << mname;
      TypeActuals tactuals = type_inst_type_actuals(ty);
      bool started = false;
      FOR_SEQUENCE(Type, ty, TypeActuals, tactuals,
		   if (started) {
		     os << ",";
		   } else {
		     started = true;
		     os << "< ";
		   }
		   dump_Type_signature(ty,os));
      if (started) os << " >";
    }
    break;
  case KEYtype_use:
    {
      Use u = type_use_use(ty);
      Declaration decl = USE_DECL(u);
      dump_TypeEnvironment(USE_TYPE_ENV(u),os);
      os << "C_" << decl_name(decl);
    }
    break;
  default:
    os << "Module"; // covers for function_type
    break;
  }
}

void dump_Declaration_superclass(Type extension, Declaration decl, ostream& os);

void dump_Type_superclass(bool is_phylum, Type ty, ostream& os)
{
  switch (Type_KEY(ty)) {
  case KEYprivate_type:
    dump_Type_superclass(is_phylum,private_type_rep(ty),os);
    break;
  case KEYremote_type:
    dump_Type_superclass(false,remote_type_nodetype(ty),os);
    break;
  case KEYno_type:
    if (is_phylum) { 
      os << "C_PHYLUM";
    } else {
      os << "C_TYPE";
    }
    break;
  case KEYtype_inst:
    {
      char *mname = (char*)symbol_name(use_name(module_use_use(type_inst_module(ty))));
      os << "C_" << mname;
      TypeActuals tactuals = type_inst_type_actuals(ty);
      bool started = false;
      FOR_SEQUENCE(Type, ty, TypeActuals, tactuals,
		   if (started) {
		     os << ",";
		   } else {
		     started = true;
		     os << "< ";
		   }
		   dump_Type_signature(ty,os));
      if (started) os << " >";
    }
    break;
  case KEYtype_use:
    {
      Declaration td = USE_DECL(type_use_use(ty));
      switch (Declaration_KEY(td)) {
      case KEYsome_type_formal:
	os << "C_" << decl_name(td);
	return;
      case KEYtype_decl:
	dump_Type_superclass(false,type_decl_type(td),os);
	return;
      case KEYphylum_decl:
	dump_Type_superclass(true,phylum_decl_type(td),os);
	return;
      case KEYtype_renaming:
	dump_Type_superclass(true,type_renaming_old(td),os);
	return;
      default:
	break; // fall through 
      }
    }
    // fall through
  default:
    os << "Module";
    break;
  }
}

void dump_Declaration_superinit(Type extension, Declaration decl, ostream& os);

void dump_Module(Module,ostream&);

void dump_type_inst_construct(Type t, ostream& o)
{
  dump_Module(type_inst_module(t),o);
  TypeActuals tacts = type_inst_type_actuals(t);
  Declaration mdecl = USE_DECL(module_use_use(type_inst_module(t)));
  Declarations tfs = module_decl_type_formals(mdecl);
  Declaration tf = first_Declaration(tfs);
  bool started = false;
  FOR_SEQUENCE
    (Type,tact,TypeActuals,tacts,
     if (started) o << ",";
     else { o << "<"; started = true; }
     dump_Type_signature(tact,o));
  if (started) o << ">";
  o << "(";
  started = false;
  FOR_SEQUENCE
    (Type,tact,TypeActuals,tacts,
     if (started) o << ","; else started = true;
     dump_Type_value(tact,o);
     if (tf == 0) {
       aps_error(tact,"too many formal parameters");
     }
     tf = DECL_NEXT(tf));
  FOR_SEQUENCE
    (Expression, act, Actuals, type_inst_actuals(t),
     if (started) o << ",";
     else started = true;
     dump_Expression(act,o));
  o << ")";
}

void dump_Type_superinit(bool is_phylum, Type ty, ostream& os)
{
  switch (Type_KEY(ty)) {
  case KEYprivate_type:
    dump_Type_superinit(is_phylum,private_type_rep(ty),os);
    break;
  case KEYremote_type:
    dump_Type_superinit(is_phylum,remote_type_nodetype(ty),os);
    break;
  case KEYno_type:
    if (is_phylum) {
      os << "C_PHYLUM()";
    } else {
      os << "C_TYPE()";
    }
    break;
  case KEYtype_inst:
    {
      dump_type_inst_construct(ty,os);
    }
    break;
  case KEYtype_use:
    {
      Declaration td = USE_DECL(type_use_use(ty));
      os << "C_" << decl_name(td) << "(*_t_" << decl_name(td) << ")";
      break;
    }
  default:
    os << "Module()";
    break;
  }
}

// Currently inheritances does the transfer of values,
// but we need this to do the transfer of types:

class ServiceRecord : public map<Symbol,int> {
public:
  void add(Declaration d) {
    int namespaces = decl_namespaces(d);
    if (namespaces) {
      (*this)[def_name(declaration_def(d))] |= namespaces;
    }
  }
  int missing(Declaration d) {
    if (int namespaces = decl_namespaces(d)) {
      return namespaces & ~(*this)[def_name(declaration_def(d))];
    }
    return 0;
  }
};

void dump_Signature_service_transfers(Type from, ServiceRecord& sr,
				      Signature s, ostream& os, ostream& is)
{
  switch (Signature_KEY(s)) {
  case KEYno_sig:
    break;
  case KEYsig_use:
    {
      Use u = sig_use_use(s);
      Declaration d = USE_DECL(u);
      switch (Declaration_KEY(d)) {
      case KEYsignature_decl:
	dump_Signature_service_transfers(from,sr,
					 sig_subst(u,signature_decl_sig(d)),
					 os,is);
	break;
      case KEYsignature_renaming:
	s = signature_renaming_old(d);
	dump_Signature_service_transfers(from,sr,sig_subst(u,s),os,is);
      default:
	// There shouldn't be any other possibilities
	aps_error(d,"unexpected signature decl");
      }
    }
    break;
  case KEYfixed_sig:
    break;
  case KEYmult_sig:
    dump_Signature_service_transfers(from,sr,mult_sig_sig1(s),os,is);
    dump_Signature_service_transfers(from,sr,mult_sig_sig2(s),os,is);
    break;
  case KEYsig_inst:
    {
      Declaration cd = USE_DECL(some_use_u(sig_inst_class(s)));
      dump_Signature_service_transfers(from,sr,some_class_decl_parent(cd),os,is);
      Block b = some_class_decl_contents(cd);
      TypeContour tc;
      tc.outer = 0;
      tc.source = cd;
      tc.type_formals = some_class_decl_type_formals(cd);
      tc.result = some_class_decl_result_type(cd);
      tc.u.type_actuals = sig_inst_actuals(s);
      static Symbol fake_sym = intern_symbol("*fake*");
      static Use fake = use(fake_sym);
      static struct Use_info* ui = Use_info(fake);
      for (Declaration d = first_Declaration(block_body(b)); d;
	   d = DECL_NEXT(d)) {
	if (sr.missing(d)) {
	  if (!def_is_public(declaration_def(d))) continue;
	  char *name = decl_name(d);
	  //int ns = decl_namespaces(d);
	  ui->use_decl = d;
	  ui->use_type_env = &tc;
	  switch (Declaration_KEY(d)) {
	  case KEYsome_type_decl:
	  case KEYtype_renaming:
	    os << "  typedef typename ";
	    dump_Type_signature(from,os);
	    os << "::C_" << name << " C_" << name << ";\n";
	    os << "  typedef typename ";
	    dump_Type_signature(from,os);
	    os << "::T_" << name << " T_" << name << ";\n";
	    break;
	  default:
	    break;
	  }
	  sr.add(d);
	}
      }
    }
  }
}

// read this code while reading dump_Type_super{class,init}
void dump_Type_service_transfers(ServiceRecord& sr,
				 Type from,
				 bool is_phylum,
				 Type ty, ostream& os, ostream& is)
{
  static Type fake_no_type = no_type();
  //static Symbol type_sym = intern_symbol("_type");
  //static Symbol phylum_sym = intern_symbol("_phylum");
  
  switch (Type_KEY(ty)) {
  case KEYno_type:
    break;
  case KEYremote_type:
    // set is_phylum to true because we want nodes.
    dump_Type_service_transfers(sr,from,true,remote_type_nodetype(ty),os,is);
    break;
  case KEYprivate_type:
    dump_Type_service_transfers(sr,from,is_phylum,private_type_rep(ty),os,is);
    break;
  case KEYfunction_type:
    // do nothing
    break;
  case KEYtype_inst:
    // do nothing: handled by dump_Type_super{class,init}
    break;
  case KEYtype_use:
    {
      Use u = type_use_use(ty);
      Type as = 0;
      Declaration td = USE_DECL(u);
      switch (Declaration_KEY(td)) {
      case KEYtype_decl:
	as = type_subst(u,type_decl_type(td));
	dump_Type_service_transfers(sr,ty,false,as,os,is);
	break;
      case KEYphylum_decl:
	as = type_subst(u,phylum_decl_type(td));
	dump_Type_service_transfers(sr,ty,true,as,os,is);
	break;
      case KEYphylum_formal:
	is_phylum = true;
	// fall through
      case KEYtype_formal:
	as = fake_no_type;
	{
	  Signature sig = sig_subst(u,some_type_formal_sig(td));
	  dump_Signature_service_transfers(ty,sr,sig,os,is);
	}
	break;
      case KEYtype_renaming:
	dump_Type_service_transfers(sr,ty,is_phylum,
				    type_subst(u,type_renaming_old(td)),os,is);
	break;
      default:
	aps_error(td,"What sort of type decl to get services from ?");
      }
    }
    break;
  }
}

void dump_some_attribute(Declaration d,
			 string i,
			 Type nt, Type vt,
			 Direction dir,
			 Default deft,
			 Implementation::ModuleInfo *info,
			 const output_streams& oss)
{
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  ostream& is = oss.is;
  ostream& bs = inline_definitions ? hs : cpps;

  char *cname = decl_name(oss.context);
  char *name = decl_name(d);
  bool is_col = direction_is_collection(dir);
  bool is_cir = direction_is_circular(dir);
  string attr_class_name = string("A") + i + "_" + name;

  hs << indent(nesting_level-1) << " private:\n";
  hs << indent() << "class " << attr_class_name << " : public "
     << (is_col ? "Collection<" : "")
     << (is_cir ? "Circular" : "") << "Attribute<"
     << as_sig(nt) << "," << as_sig(vt)
     << (is_col ? "> " : "") << "> {\n";
  ++nesting_level;
  output_streams noss(oss.context,hs,cpps,is,
		      oss.prefix + attr_class_name + "::");
  hs << indent() << "typedef " << nt << " node_type;\n";
  hs << indent() << "typedef " << vt << " value_type;\n";
  hs << "\n";
  hs << indent() << "C_" << cname << "* context;\n";
  hs << indent(nesting_level-1) << " public:\n";

  // dump the constructor:
  {
    noss << header_return_type<Type>(0)
	 << header_function_name(attr_class_name)
	 << "(C_" << cname << "*c, "
	 << as_sig(nt) << " *nt, "
	 << as_sig(vt) << " *vt";
    if (is_col) noss << ", " << vt << " init";
    noss << ")" << header_end();
    
    bs << " : " << (is_col ? "Collection<" : "")
       << (is_cir ? "Circular" : "") << "Attribute<"
       << as_sig(nt) << "," << as_sig(vt)
       << (is_col ? "> " : "")
       << ">(nt,vt,\"" << i << name << "\""
       << (is_col ? ",init" : "") << "), context(c) {}\n";
  }

  hs << "   protected:\n";

  // dump "combine"
  Expression combiner = NULL;
  if (is_col) {
    noss << header_return_type<string>("value_type") << " "
	 << header_function_name("combine")
	 << "(value_type v1, value_type v2)"
	 << header_end();
    INDEFINITION;
    bs << "{\n";
    ++nesting_level;
    switch (Default_KEY(deft)) {
    default:
      bs << indent() << "return values->v_combine(v1,v2);\n";
      break;
    case KEYcomposite:
      combiner = composite_combiner(deft);
      bs << indent() << "return context->co" << i << "_" << name
	 << "(v1,v2);\n";
    }
    --nesting_level;
    bs << "}\n";
  }

  if (Declaration_KEY(d) == KEYvalue_decl) {
    info->note_local_attribute(d,noss);
  } else {
    info->note_attribute_decl(d,noss);
  }

  --nesting_level;
  hs << indent() << "};\n\n";

  // dump the actual combiner:
  if (combiner) {
    // C++ language semantics bug: private nested classes can't get
    // get at private details:
    noss << " public:\n"; 
    noss << header_return_type<Type>(vt) << " "
	 << header_function_name(string("co")+i+"_"+name)
	 << "(" << vt << " v1, " << vt << " v2)"
	 << header_end();
    INDEFINITION;
    bs << " {\n";
    ++nesting_level;
    bs << indent() << "return " << combiner << "(v1,v2);\n";
    --nesting_level;
    bs << indent() << "}\n";
  }

  if (direction_is_input(dir))
    hs << indent(nesting_level-1) << " public:\n";
  hs << indent() << attr_class_name << " *a" << i << "_" << name << ";\n\n";
  if (!direction_is_input(dir))
    hs << indent(nesting_level-1) << " public:\n";
  
  is << ",\n    a" << i << "_" << name << "(new " << attr_class_name << "(";
  is << "this,this->" << as_val(nt) << "," << as_val(vt);
  if (is_col) {
    is << ",";
    switch (Default_KEY(deft)) {
    case KEYsimple:
      is << simple_value(deft);
      break;
    case KEYcomposite:
      is << composite_initial(deft);
      break;
    default:
      is << as_val(vt) << "->v_initial";
      break;
    }
  }
  is << "))";
}
  
void dump_local_attributes(Block b, Type at, Implementation::ModuleInfo* info,
			   const output_streams& oss)
{
  for (Declaration d = first_Declaration(block_body(b)); d; d=DECL_NEXT(d)) {
    switch (Declaration_KEY(d)) {
    default:
      aps_error(d,"Cannot handle this kind of statement");
      break;
    case KEYvalue_decl:
      {
	static int unique = 0;
	LOCAL_UNIQUE_PREFIX(d) = ++unique;
	ostringstream ns;
	ns << unique;
	dump_some_attribute(d,ns.str(),at,value_decl_type(d),
			    value_decl_direction(d),
			    value_decl_default(d),info,oss);
      }
      break;
    case KEYassign:
      break;
    case KEYblock_stmt:
      dump_local_attributes(block_stmt_body(d),at,info,oss);
      break;
    case KEYif_stmt:
      dump_local_attributes(if_stmt_if_true(d),at,info,oss);
      dump_local_attributes(if_stmt_if_false(d),at,info,oss);
      break;
    case KEYcase_stmt:
      {
	FOR_SEQUENCE
	  (Match,m,Matches,case_stmt_matchers(d),
	   dump_local_attributes(matcher_body(m),at,info,oss));

	dump_local_attributes(case_stmt_default(d),at,info,oss);
      }
      break;
    }
  }
}

#define EXPR_FUNCTION_IS_OK 256

void dump_make_lattice_functions(Expression first_actual,
				 const output_streams& oss)
{
  Expression bottom, compare, compare_equal, join, meet;
  ostream& hs = oss.hs;
  ostream& bs = inline_definitions? oss.hs : oss.cpps;
  ostream& is = oss.is;

  bottom = first_actual;
  compare = EXPR_NEXT(bottom);
  compare_equal = EXPR_NEXT(compare);
  join = EXPR_NEXT(compare_equal);
  meet = EXPR_NEXT(join);

  {
    hs << indent() << "T_Result v_bottom;\n";
    is << ",\n    v_bottom(" << bottom << ")";
  }

  {
    Expression_info(compare)->expr_flags |= EXPR_FUNCTION_IS_OK;
    oss << header_return_type<Type>(0) << "T_Boolean "
	<< header_function_name("v_compare")
	<< "(T_Result v1, T_Result v2)"
	<< header_end();
    INDEFINITION;
    bs << "{\n";
    ++nesting_level;
    bs << indent() << "return " << compare << "(v1,v2);\n";
    --nesting_level;
    bs << indent() << "}\n";
  }
  {
    Expression_info(compare_equal)->expr_flags |= EXPR_FUNCTION_IS_OK;
    oss << header_return_type<Type>(0) << "T_Boolean "
	<< header_function_name("v_compare_equal")
	<< "(T_Result v1, T_Result v2)"
	<< header_end();
    bs << "{\n";
    INDEFINITION;
    ++nesting_level;
    bs << indent() << "return " << compare_equal << "(v1,v2);\n";
    --nesting_level;
    bs << indent() << "}\n";
  }

  {
    Expression_info(join)->expr_flags |= EXPR_FUNCTION_IS_OK;
    oss << header_return_type<string>("T_Result") << " "
	<< header_function_name("v_join")
	<< "(T_Result v1, T_Result v2)"
	<< header_end();
    bs << "{\n";
    INDEFINITION;
    ++nesting_level;
    bs << indent() << "return " << join << "(v1,v2);\n";
    --nesting_level;
    bs << indent() << "}\n";
  }
  {
    Expression_info(meet)->expr_flags |= EXPR_FUNCTION_IS_OK;
    oss << header_return_type<string>("T_Result") << " "
	<< header_function_name("v_meet")
	<< "(T_Result v1, T_Result v2)"
	<< header_end();
    bs << "{\n";
    INDEFINITION;
    ++nesting_level;
    bs << indent() << "return " << meet << "(v1,v2);\n";
    --nesting_level;
    bs << indent() << "}\n";
  }
}

void dump_cpp_Declaration(Declaration decl,const output_streams& oss)
{
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  ostream& is = oss.is;
  string prefix = oss.prefix;
  Declaration context = oss.context;
  char *name = 0;
  switch (Declaration_KEY(decl)) {
  case KEYdeclaration:
    name = (char*)get_code_name(def_name(declaration_def(decl)));
    if (!name) name = decl_name(decl);
    break;
  default:
    break;
  }
  if (name)
    for (int i=0; i < omitted_number; ++i)
      if (streq(omitted[i],name)) return;

  ostream& bs = inline_definitions ? hs : cpps;
  switch (Declaration_KEY(decl)) {
  case KEYclass_decl:
  case KEYsignature_decl:
  case KEYclass_renaming:
  case KEYsignature_renaming:
    {
      // do nothing (classes and signatures have no C++ significance)
    }
    break;
  case KEYmodule_decl:
    {
      Declarations body = block_body(module_decl_contents(decl));
      Declaration rdecl = module_decl_result_type(decl);
      bool rdecl_is_phylum = (Declaration_KEY(rdecl) == KEYphylum_decl);
      DECL_NEXT(rdecl) = first_Declaration(body);
      Declaration first_decl = rdecl;
      bool is_template = template_decl_p(decl);
      inline_definitions += is_template;
      char *impl_type = 0;
      bool constructor_is_native = false;
      for (int j=0; j < impl_number; ++j)
	if (streq(impl_types[j],name)) impl_type = impl_types[j+1];
      if (impl_type) first_decl = first_Declaration(body);
      ostream& bs = inline_definitions ? hs : cpps;
      if (is_template) {
	bool started = false;
	hs << "template <";
	for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	     tf ; tf = DECL_NEXT(tf)) {
	  if (started) hs << ",";
	  else started = true;
	  hs << "class C_" << decl_name(tf);
	}
	hs << ">\n";
      }
      hs << "class C_" << name << " : public ";

      Type rut = some_type_decl_type(rdecl);

      // hack for MAKE_LATTICE:
      bool is_make_lattice = false;
      Expression first_ml_actual = 0;
      if (Type_KEY(rut) == KEYtype_inst) {
	Use u = module_use_use(type_inst_module(rut));
	Symbol sym;
	switch (Use_KEY(u)) {
	case KEYuse:
	  sym = use_name(u);
	  break;
	case KEYqual_use:
	  sym = qual_use_name(u);
	  break;
	}
	static Symbol make_lattice_sym = intern_symbol("MAKE_LATTICE");
	if (sym == make_lattice_sym) {
	  is_make_lattice = true;
	  first_ml_actual = first_Actual(type_inst_actuals(rut));
	  rut = first_TypeActual(type_inst_type_actuals(rut));
	  Declaration new_rdecl = type_decl(type_decl_def(rdecl),
					    type_decl_sig(rdecl),
					    rut);
	  DECL_NEXT(new_rdecl) = DECL_NEXT(rdecl);
	  rdecl = new_rdecl;
	}
      }
      
      if (impl_type) {
	Type_info(rut)->impl_type = impl_type;
      }
      dump_Type_superclass(rdecl_is_phylum,rut,hs);
      hs << " {\n";
      ++nesting_level;

      // hs << indent() << "// Indent = " << nesting_level*2 << endl;

      for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	   tf ; tf = DECL_NEXT(tf)) {
	hs << "  typedef typename C_" << decl_name(tf)
	   << "::T_Result T_" << decl_name(tf) << ";\n";
      }
      for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	   tf ; tf = DECL_NEXT(tf)) {
	hs << "  C_" << decl_name(tf)
	   << " *t_" << decl_name(tf) << ";\n";
      }
      for (Declaration vf=first_Declaration(module_decl_value_formals(decl));
	   vf ; vf = DECL_NEXT(vf)) {
	hs << "  ";
	dump_formal(vf,"v_",hs);
	hs << ";\n";
      }
      hs << " public:\n";

      ostringstream is;

      // The Result type as signature:
      hs << "  typedef C_" << name << " C_Result;\n";
      // The Result type as C++ type:
      if (impl_type) {
	hs << "  typedef " << impl_type << " T_Result;\n";
      } else {
	hs << "  typedef ";
	dump_Type(rut,hs);
	hs << " T_Result;\n";
      }
      // The Result type as value:
      // This is silly but makes it easier to do things:
      hs << "  C_Result* const t_Result;\n";
      is << ",\n    t_Result(this)";

      ServiceRecord sr;
      // get "inherited" services:
      // need to get typedefs which don't inherit
      for (Declaration d = first_decl; d ; d = DECL_NEXT(d)) {
	sr.add(d);
      }
      dump_Type_service_transfers(sr,rut,rdecl_is_phylum,rut,hs,is);

      Implementation::ModuleInfo *info = impl->get_module_info(decl);
      output_streams new_oss(decl,hs,bs,is,prefix+"C_"+name+"::");

      if (is_make_lattice) {
	dump_make_lattice_functions(first_ml_actual,new_oss);
      }
      
      for (Declaration d = first_decl; d ; d = DECL_NEXT(d)) {
	switch (Declaration_KEY(d)) {
	case KEYmodule_decl:
	case KEYtype_decl:
	case KEYphylum_decl:
	case KEYtype_renaming:
	  // do type decls first (try to avoid C++ typedef problems)
	  dump_cpp_Declaration(d,new_oss);
	  break;
	default:
	  break;
	}
      }
      for (Declaration d = first_decl; d ; d = DECL_NEXT(d)) {
	switch (Declaration_KEY(d)) {
	case KEYmodule_decl:
	case KEYtype_decl:
	case KEYphylum_decl:
	case KEYtype_renaming:
	  // already did this:
	  break;
	default:
	  dump_cpp_Declaration(d,new_oss);
	}
	// now specific to module things:
	switch (Declaration_KEY(d)) {
	case KEYattribute_decl:
	  {
	    Type fty = attribute_decl_type(d);
	    Declaration f = first_Declaration(function_type_formals(fty));
	    Declarations rdecls = function_type_return_values(fty);
	    Declaration rdecl = first_Declaration(rdecls);

	    dump_some_attribute(d,"",infer_formal_type(f),
				value_decl_type(rdecl),
				attribute_decl_direction(d),
				attribute_decl_default(d),info,new_oss);

	    new_oss << header_return_type<Type>(value_decl_type(rdecl)) << " "
		    << header_function_name(string("v_") + decl_name(d))
		    << "(" << infer_formal_type(f) << " anode)"
		    << header_end();
	    bs << "{ return a_" << decl_name(d) << "->evaluate(anode); }\n";
	  }
	  break;
	case KEYtop_level_match:
	  {
	    Match m = top_level_match_m(d);
	    Type anchor_type = infer_pattern_type(matcher_pat(m));
	    Block body = matcher_body(m);
	    dump_local_attributes(body,anchor_type,info,new_oss);
	  }
	  info->note_top_level_match(d,new_oss);
	  break;
	case KEYvalue_decl:
	  {
	    Def def = value_decl_def(d);
	    Default deft = value_decl_default(d);
	    Direction dir = value_decl_direction(d);
	    // undefined simple variables mean the constructor must be native:
	    if (Default_KEY(deft) == KEYno_default &&
		!direction_is_circular(dir) &&
		!direction_is_collection(dir) &&
		!direction_is_input(dir))
	      constructor_is_native = true;
	    if (!def_is_constant(def))
	      info->note_var_value_decl(d,new_oss);
	  }
	  break;
	default:
	  break;
	}
      }

      info->implement(new_oss);

      // The normal default constructor header:
      hs << "\n  C_" << name << "(";
      {
	bool started = false;
	for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	     tf ; tf = DECL_NEXT(tf)) {
	  if (started) hs << ","; else started = true;
	  hs << "C_" << decl_name(tf) 
	     << "* _t_" << decl_name(tf);
	}
	for (Declaration vf=first_Declaration(module_decl_value_formals(decl));
	     vf; vf = DECL_NEXT(vf)) {
	  if (started) hs << ","; else started = true;
	  dump_formal(vf,"_v_",hs);
	}
      }
      hs << ")";
      if (!inline_definitions && !constructor_is_native) {
	hs << ";\n";
	cpps << "\n" << prefix << "C_" << name << "::C_" << name << "(";
	bool started = false;
	for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	     tf ; tf = DECL_NEXT(tf)) {
	  if (started) cpps << ","; else started = true;
	  cpps << "C_" << decl_name(tf) 
	       << "* _t_" << decl_name(tf);
	}
	for (Declaration vf=first_Declaration(module_decl_value_formals(decl));
	     vf; vf = DECL_NEXT(vf)) {
	  if (started) cpps << ","; else started = true;
	  dump_formal(vf,"_v_",cpps);
	}
	cpps << ") ";	
      }

      // dump the default constructor body:
      if (constructor_is_native) {
	hs << ";\n";
      } else {
	bs << " : ";
	dump_Type_superinit(rdecl_is_phylum,rut,bs);
	for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	     tf ; tf = DECL_NEXT(tf)) {
	  bs << ",\n    t_" << decl_name(tf) << "(_t_" << decl_name(tf) << ")";
	}
	for (Declaration vf=first_Declaration(module_decl_value_formals(decl));
	     vf ; vf = DECL_NEXT(vf)) {
	  bs << ",\n    v_" << decl_name(vf) << "(_v_" << decl_name(vf) << ")";
	}
	bs << is.str();
	bs << " {}\n\n";
      }

      // we don't need to generate a copy constructor
      // (use the default generated one)

      inline_definitions -= is_template;
      --nesting_level;
      hs << "};\n\n";
    }
    break;
  case KEYtype_decl:
  case KEYphylum_decl:
    {
      bool is_phylum = (KEYphylum_decl == Declaration_KEY(decl));
      bool is_result = (context &&
			Declaration_KEY(context) == KEYmodule_decl &&
			module_decl_result_type(context) == decl) ;
      Declarations body = context ? block_body(module_decl_contents(context)) : 0;
      Type type = some_type_decl_type(decl);
      bool first = true;
      switch (Type_KEY(type)) {
      case KEYno_type:
	if (context) {
	  for (Declaration d=first_Declaration(body); d; d=DECL_NEXT(d)) {
	    if (KEYconstructor_decl == Declaration_KEY(d) &&
		constructor_decl_base_type_decl(d) == decl) {
	      if (first) {
		if (context) hs << "  ";
		hs << "enum P_" << name << "{";
		first = false;
	      } else hs << ",";
	      hs << " p_" << decl_name(d);
	    }
	  }
	  if (!first) hs << " };\n";
	}

	// if it's a result decl, we don't define this, but otherwise
	// we need definitions for C_name, T_name and t_Name
	if (!is_result) {
	  if (context) hs << "  ";
	  hs << "typedef C_" << (is_phylum ? "PHYLUM" : "TYPE") << " "
	     << "C_" << name << ";\n";
	  if (context) hs << "  ";
	  hs << "typedef C_" << (is_phylum ? "PHYLUM" : "TYPE")
	     << "::T_Result " << "T_" << name << ";\n";
	  if (context) hs << "  "; else hs << "extern ";
	  hs << "C_" << name << " *t_" << name << ";\n";
	  if (!context) {
	    cpps << "C_" << name << " *t_" << name
		 << " = new C_" << name << "();\n";
	  }
	  hs << endl;
	}	  
	break;
      default:
	if (!is_result) {
	  if (context) hs << "  ";
	  hs << "typedef ";
	  dump_Type_signature(type,hs);
	  hs << " C_" << name << ";\n";
	  if (context) hs << "  ";
	  hs << "typedef ";
	  dump_Type(type,hs);
	  hs << " T_" << name << ";\n";
	  if (context) hs << "  "; else hs << "extern ";
	  hs << "C_" << name << " *t_" << name << ";\n";
	  if (!context) {
	    hs << "C_" << name << " *get_" << name << "();\n";
	    cpps << "C_" << name << " *t_" << name
		 << " = get_" << name << "();\n";
	    cpps << "C_" << name << " *get_" << name << "() {\n"
		 << "  static C_" << name << " *t_" << name
		 << " = " << as_val(type) << ";\n"
		 << "  return t_" << name << ";\n" << "}\n";
	  }
	  hs << endl;
	}
	break;
      }
      if (!is_result && context) {
	is << ",\n    t_" << name << "("; // previously added "this->" for templates
	dump_Type_value(type,is);
	is << ")";
      }
    }
    break;
  case KEYconstructor_decl:
    {
      Type ft = constructor_decl_type(decl);
      Declarations formals = function_type_formals(ft);
      Declaration tdecl = constructor_decl_base_type_decl(decl);
      Declarations rdecls = function_type_return_values(ft);
      Type rt = value_decl_type(first_Declaration(rdecls));
      bool is_syntax = false;
      // char *base_type_name = decl_name(tdecl);
      switch (Declaration_KEY(tdecl)) {
      case KEYphylum_decl:
	is_syntax = true;
	break;
      case KEYtype_decl:
	is_syntax = false;
	break;
      default:
	aps_error(decl,"constructor not built on type or phylum");
	return;
      }
      if (!context) {
	aps_error(decl,"constructor cannot be declared at top-level");
	break;
      }
      /* header file */
      hs << "  struct V_" << name << " : public "
	 << (is_syntax ? "C_PHYLUM::Node" : "C_TYPE::Node")
         << " {" << endl;
      for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	hs << "    "; dump_formal(f,"v_",hs); hs << ";\n";
      }
      hs << "    V_" << name << "(Constructor *c";
      for (Declaration f = first_Declaration(function_type_formals(ft));
	   f != NULL; f = DECL_NEXT(f)) {
	hs << ", ";
	dump_Type(formal_type(f),hs);
	hs << " _" << decl_name(f);
      }
      hs << ")";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << prefix << "V_" << name << "::V_" << name
	  << "(Constructor * c";
	for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	  cpps << ", ";
	  dump_Type(formal_type(f),cpps);
	  cpps << " _" << decl_name(f);
	}
	cpps << ")";
      }
      bs << "\n  : "
	   << (is_syntax ? "C_PHYLUM::Node" : "C_TYPE::Node") << "(c)";
      for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	bs << ", v_" << decl_name(f) << "(_" << decl_name(f) << ")";
      }
      bs << "{";
      for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	Type ft = formal_type(f);
	switch (Type_KEY(ft)) {
	case KEYtype_use:
	  {
	    Declaration ftd = USE_DECL(type_use_use(ft));
	    if (!ftd) continue;
	    switch (Declaration_KEY(ftd)) {
	    case KEYphylum_decl:
	    case KEYphylum_formal:
	      /* syntactic parent */
	      bs << "\n";
	      if (inline_definitions) hs << "    ";
	      bs << "  _" << decl_name(f) << "->set_parent(this); ";
	      break;
	    default:
	      /*NOTHING*/
	      break;
	    }
	  }
	  break;
	default:
	  /*NOTHING*/
	  break;
	}
      }
      bs << "}\n";

      // a print function (defer to constructor context)
      hs << "    std::string to_string()";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << "\nstd::string " << prefix << "V_" << name << "::to_string()";
      }
      bs << " {\n";
      if (inline_definitions) hs << "    ";
      if (is_syntax) {
        bs << "  return std::string(\"" << name
	   << "#\")+t_Integer->v_string(index)+std::string(\"(\")";
      } else {
	bs << "  return std::string(\"" << name << "(\")";
      }
      bool started = false;
      for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	if (started) {
	  bs << "+\",\"";
	} else started = true;
	bs << "\n      ";
	if (inline_definitions) hs << "    ";
	bs << "+s_string(v_" << decl_name(f) << ")";
      }
      bs << "+\")\";\n";
      if (inline_definitions) hs << "    ";
      bs << "}\n";
      hs << "  };\n\n";

      hs << "  Constructor* c_" << name << ";\n  ";
      is << ",\n    c_" << name << "(new Constructor(";
      dump_Type_value(rt,is);
      is << "->get_type(),\"" << name << "\",p_" << name << "))";

      dump_function_prototype("",name,ft,hs);
      if (!inline_definitions) {
	hs << ";\n";
	dump_function_prototype(prefix,name,constructor_decl_type(decl),cpps);
      }
      bs << " {\n";
      if (inline_definitions) hs << "  ";
      bs << "  return new V_" << name << "(c_" << name;
      for (Declaration f = first_Declaration(function_type_formals(constructor_decl_type(decl)));
	   f != NULL; f = DECL_NEXT(f)) {
	bs << ",v_" << decl_name(f);
      }
      bs << ");\n";
      if (inline_definitions) hs << "  ";
      bs << "}\n";
    }
    break;
  case KEYvalue_decl:
    {
      if (context) 
	hs << "  ";
      else
	hs << "extern ";
      dump_Typed_decl(value_decl_type(decl),decl,"v_",hs);
      hs << ";\n" << endl;
      if (context == 0) {
	switch (Default_KEY(value_decl_default(decl))) {
	case KEYsimple:
	  dump_Typed_decl(value_decl_type(decl),decl,"v_",cpps);
	  cpps << " = ";
	  dump_Expression(simple_value(value_decl_default(decl)),cpps);
	  cpps << ";\n" << endl;
	  break;
	case KEYno_default:
	  // native value
	  break;
	default:
	  aps_error(decl,"Cannot generate code for this");
	  break;
	}
      } else {
	// in module
	is << ",\n    v_" << name << "(";
	dump_vd_Default(decl,is);
	is << ")";
      }
    }
    break;
  case KEYattribute_decl:
    // must be handled by the surrounding module
    break;
  case KEYfunction_decl:
    {
      Type fty = function_decl_type(decl);
      Declaration rdecl = first_Declaration(function_type_return_values(fty));
      Block b = function_decl_body(decl);
      if (context) hs << "  ";
      dump_function_prototype("",name,fty,hs);
      if (!inline_definitions) {
	hs << ";\n" << endl;
      }
      // three kinds of definitions:
      // 1. the whole thing: a non-empty body:
      if (first_Declaration(block_body(b))) {
	if (!inline_definitions) {
	  dump_function_prototype(prefix,name,fty,cpps);
	}
	INDEFINITION;
	cpps << "{\n";
	++nesting_level;
	if (debug)
	  dump_function_debug(name,fty,cpps);
	impl->implement_function_body(decl,cpps);
	--nesting_level;
	cpps << indent() << "}\n" << endl;
	return;
      } else if (rdecl) {
	// 2. simple default
	switch (Default_KEY(value_decl_default(rdecl))) {
	case KEYsimple:
	  {
	    if (!inline_definitions) {
	      dump_function_prototype(prefix,name,fty,cpps);
	    }
	    cpps << "{\n";
	    INDEFINITION;
	    ++nesting_level;
	    if (debug)
	      dump_function_debug(decl_name(decl),fty,cpps);
	    cpps << indent() << "return "
		 << simple_value(value_decl_default(rdecl)) << ";\n";
	    --nesting_level;
	    cpps << indent() << "}\n";
	    return;
	  }
	default:
	  break;
	}
      }
      // cout << name << " has no body.\n";
      // 3. nothing -- leave to native code
      if (inline_definitions) {
	hs << ";\n";
      }
    }
    break;
  case KEYpolymorphic:
    ++inline_definitions;
    {
      Declarations tfs = polymorphic_type_formals(decl);
      Declarations body = block_body(polymorphic_body(decl));
      bool started = false;
      hs << "template <";
      for (Declaration f=first_Declaration(tfs); f; f=DECL_NEXT(f)) {
	if (started) hs << ","; else started = true;
	hs << "class C_" << decl_name(f);
      }
      hs << ">\nstruct C_" << name << "{\n";
      ++nesting_level;
      for (Declaration f=first_Declaration(tfs); f; f=DECL_NEXT(f)) {
	char *tfn = decl_name(f);
	hs << "  typedef typename C_" << tfn << "::T_Result T_" << tfn << ";\n";
      }
      for (Declaration f=first_Declaration(tfs); f; f=DECL_NEXT(f)) {
	hs << "  C_" << decl_name(f);
	hs << " *t_" << decl_name(f) << ";\n";
      }
      ostringstream is;
      is << " ";

      for (Declaration d=first_Declaration(body); d; d=DECL_NEXT(d)) {
	dump_cpp_Declaration(d,output_streams(decl,hs,hs,is,""));
      }
      hs << "  C_" << name << "(";
      started = false;
      for (Declaration f=first_Declaration(tfs); f; f=DECL_NEXT(f)) {
	if (started) hs << ","; else started = true;
	hs << "C_" << decl_name(f)
	   << " *_t_" << decl_name(f);
      }
      hs << ") : ";
      started = false;
      for (Declaration f=first_Declaration(tfs); f; f=DECL_NEXT(f)) {
	if (started) hs << ",\n    "; else started = true;
	hs << "t_" << decl_name(f) << "(_t_" << decl_name(f) << ")";
      }
      hs << is.str();
      hs << " {}\n};\n\n";
      --nesting_level;
    }
    --inline_definitions;
    break;
  case KEYtop_level_match:
    break;
  case KEYvalue_renaming:
    {
      Expression old = value_renaming_old(decl);
      Type ty = infer_expr_type(old);
      if (Type_KEY(ty) == KEYfunction_type) {
	Declaration ff = first_Declaration(function_type_formals(ty));
	if (ff && Declaration_KEY(ff) == KEYseq_formal) break;
	if (context) hs << "  ";
	hs << "inline ";
	dump_function_prototype("",name,ty,hs);
	hs << " { return ";
	aps_yylineno = tnode_line_number(decl);
	Actuals as = nil_Actuals();
	for (Declaration f=first_Declaration(function_type_formals(ty));
	     f ; f = DECL_NEXT(f)) {
	  Use u = use(def_name(declaration_def(f)));
	  USE_DECL(u) = f;
	  as = append_Actuals(as,list_Actuals(value_use(u)));
	}
	dump_Expression(funcall(old,as),hs);
	hs << "; }\n";
      } else {
	if (context) hs << "  ";
	dump_Type(ty,hs);
	hs << " v_" << name;
	if (context) {
	  // initialization done by module:
	  hs << ";\n" << endl;
	  is << ",\n    v_" << name << "(";
	  dump_Expression(old,is);
	  is << ")";
	} else {
	  if (!inline_definitions) {
	    hs << ";\n" << endl;	
	    dump_Type(ty,cpps);
	    cpps << " v_" << name;
	  }
	  dump_Expression(old,cpps);
	  cpps << ";\n" << endl;
	}
      }
    }
    break;
  case KEYtype_renaming:
    {
      Type old = type_renaming_old(decl);
      if (context) hs << "  ";
      hs << "typedef ";
      dump_Type_signature(old,hs);
      hs << " C_" << name << ";\n";
      if (context) hs << "  ";
      hs << "typedef ";
      dump_Type(old,hs);
      hs << " T_" << name << ";\n";
      if (context) hs << "  ";
      hs << "C_" << name << " *t_" << name << ";\n";
      if (!context) {
	cpps << "C_" << name << " *t_" << name << " = ";
	dump_Type_value(old,cpps);
      }
      if (context) {
	is << ",\n    t_" << name << "(this->"; // added "this->" because of templates
	dump_Type_value(old,is);
	is << ")";
      }
    }
    break;
  case KEYpragma_call:
    break;
  case KEYpattern_decl:
    //! patterns not implemented
    break;
  default:
    cout << "Not handling declaration " << decl_name(decl) << endl;
  }
}

void dump_Module(Module t, ostream& o)
{
  switch (Module_KEY(t)) {
  case KEYmodule_use:
    o << "C_" << use_name(module_use_use(t));
    break;
  default:
    aps_error(t,"cannot handle this module expression");
  }
}

void dump_TypeEnvironment(TypeEnvironment*, ostream&os);

bool no_type_is_phylum(Type ty) 
{
  Declaration decl = (Declaration)tnode_parent(ty);
  if (ABSTRACT_APS_tnode_phylum(decl) == KEYDeclaration) {
    switch (Declaration_KEY(decl)) {
    case KEYtype_decl:
      return false;
    case KEYphylum_decl:
      return true;
    default:
      break;
    }
  }
  aps_error(decl,"no_type occurs in a strange place");
  return false;
}

// use a sloppy global variable to avoid rewriting code
static int type_prefix = 0;
void dump_Type_prefixed(Type t, ostream&o)
{
  ++type_prefix;
  dump_Type(t,o);
  --type_prefix;
}

void dump_Type(Type t, ostream& o)
{
  switch (Type_KEY(t)) {
  case KEYtype_use:
    {
      Use u = type_use_use(t);
      Declaration tdecl = USE_DECL(u);
      switch (Declaration_KEY(tdecl)) {
      default:
	dump_TypeEnvironment(USE_TYPE_ENV(u),o);
	if (type_prefix)
	  o << get_prefix(tdecl);
	/* FALL THROUGH */
      case KEYsome_type_formal:
	o << "T_" << decl_name(tdecl);
	break;
      }
    }
    break;
  case KEYtype_inst:
    {
      Declaration mdecl = USE_DECL(module_use_use(type_inst_module(t)));
      TypeActuals tacts = type_inst_type_actuals(t);
      if (inline_definitions) o << "typename ";
      dump_Module(type_inst_module(t),o);
      bool started = false;
      FOR_SEQUENCE
	(Type,tact,TypeActuals,tacts,
	 if (started) o << ",";
	 else { o << "<"; started = true; }
	 dump_Type_signature(tact,o));
      if (started) o << ">";
      o << "::T_" << decl_name(module_decl_result_type(mdecl));
    }
    break;
  case KEYremote_type:
    dump_Type(remote_type_nodetype(t),o);
    break;
  case KEYprivate_type:
    dump_Type(private_type_rep(t),o);
    break;
  case KEYno_type:
    if (no_type_is_phylum(t)) {
      o << "C_PHYLUM::Node*";
    } else {
      o << "C_TYPE::Node*";
    }
    break;
  case KEYfunction_type:
    {
      static int n = 0;
      ++n;
      o << "typedef ";
      Declaration rdecl = first_Declaration(function_type_return_values(t));
      dump_Type(value_decl_type(rdecl),o);
      o << "(*FUNC_" << n << ")(";
      bool started = false;
      for (Declaration f=first_Declaration(function_type_formals(t));
	   f ; f = DECL_NEXT(f)) {
	if (started) o << ","; else started = true;
	dump_Type(formal_type(f),o);
	if (Declaration_KEY(f) == KEYseq_formal)
	  o << "*";
      }
      o << ");\n";
      o << "  FUNC_" << n << " ";
    }
    break;
  default:
    aps_error(t,"cannot handle this type");
    break;
  }
}

void dump_Typed_decl(Type t, Declaration decl, const char*prefix,ostream& o)
{
  Symbol sym = def_name(declaration_def(decl));
  switch (Type_KEY(t)) {
  case KEYfunction_type:
    {
      static int n = 0;
      ++n;
      Declaration rdecl = first_Declaration(function_type_return_values(t));
      dump_Type(value_decl_type(rdecl),o);
      o << "(*" << prefix << sym << ")(";
      bool started = false;
      for (Declaration f=first_Declaration(function_type_formals(t));
	   f ; f = DECL_NEXT(f)) {
	if (started) o << ","; else started = true;
	dump_Type(formal_type(f),o);
	if (Declaration_KEY(f) == KEYseq_formal)
	  o << "*";
      }
      o << ")";
    }
    break;
  default:
    dump_Type(t,o);
    o << " " << prefix << sym;
    break;
  }
}

void dump_Type_value(Type t, ostream& o)
{
  switch (Type_KEY(t)) {
  case KEYtype_use:
    {
      void *p = tnode_parent(USE_DECL(type_use_use(t)));
      if (p != 0 && ABSTRACT_APS_tnode_phylum(p) == KEYUnit) {
	// A top-level type!
	// To avoid problems, we call the "get" function
	dump_Use(type_use_use(t),"get_",o);
	o << "()";
      } else {
        o << "this->"; // added because of C++ templates
	dump_Use(type_use_use(t),"t_",o);
      }
    }
    break;
  case KEYtype_inst:
    {
      o << "new ";
      dump_type_inst_construct(t,o);
    }
    break;
  case KEYremote_type:
    dump_Type_value(remote_type_nodetype(t),o);
    break;
  case KEYprivate_type:
    dump_Type_value(private_type_rep(t),o);
    break;
  case KEYfunction_type:
    o << "0"; // no services
    break;
  case KEYno_type:
    {
      if (no_type_is_phylum(t)) {
	o << "new C_PHYLUM()";
      } else {
	o << "new C_TYPE()";
      }
    }
    break;
  default:
    aps_error(t,"cannot handle this type");
    break;
  }
}

void dump_vd_Default(Declaration d, ostream& o)
{
  Type vt = value_decl_type(d);
  Direction dir = value_decl_direction(d);
  Default deft = value_decl_default(d);
  switch (Default_KEY(deft)) {
  case KEYsimple:
    dump_Expression(simple_value(deft),o);
    break;
  case KEYcomposite:
    dump_Expression(composite_initial(deft),o);
    break;
  default:
    if (direction_is_collection(dir)) {
      o << as_val(vt) << "->v_initial";
    } else if (direction_is_circular(dir)) {
      o << as_val(vt) << "->v_bottom";
    } else {
      /*? print something ?*/
      o << 0;
    }
    break;
  }
}

bool funcall_is_collection_construction(Expression e)
{
  static SYMBOL braces = intern_symbol("{}");

  switch (Expression_KEY(funcall_f(e))) {
  default:
    break;
  case KEYvalue_use:
    {
      Use u = value_use_use(funcall_f(e));
      Symbol sym = 0;
      switch (Use_KEY(u)) {
      case KEYuse:
	sym = use_name(u);
	break;
      case KEYqual_use:
	sym = qual_use_name(u);
	break;
      }
      if (sym == braces) return true;
    }
    break;
  }
  return false;
}

void dump_collect_Actuals(Type ctype, Actuals as, ostream& o)
{
  switch (Actuals_KEY(as)) {
  case KEYnil_Actuals:
    dump_Type_value(ctype,o);
    o << "->v_none()";
    break;
  case KEYlist_Actuals:
    {
      Expression e = list_Actuals_elem(as);
      /* Handle a special case of sequence expression: */
      if (Expression_KEY(e) == KEYrepeat) {
	Expression seq = repeat_expr(e);
	if (base_type_equal(infer_expr_type(seq),ctype)) {
	  dump_Expression(seq,o);
	  return;
	}
	/* Fall through and cause error */
      }
      dump_Type_value(ctype,o);
      o << "->v_single(";
      dump_Expression(list_Actuals_elem(as),o);
      o << ")";
    }
    break;
  case KEYappend_Actuals:
    dump_Type_value(ctype,o);
    o << "->v_append(";
    dump_collect_Actuals(ctype,append_Actuals_l1(as),o);
    o << ",";
    dump_collect_Actuals(ctype,append_Actuals_l2(as),o);
    o << ")";
  }
}

void dump_Expression(Expression e, ostream& o)
{
  switch (Expression_KEY(e)) {
  case KEYinteger_const:
    o << integer_const_token(e);
    break;
  case KEYreal_const:
    o << real_const_token(e);
    break;
  case KEYstring_const:
    o << string_const_token(e);
    break;
  case KEYfuncall:
    if (funcall_is_collection_construction(e)) {
      // inline code to call append, single and null constructors
      dump_collect_Actuals(infer_expr_type(e),funcall_actuals(e),o);
      return;
    }
    dump_Expression(funcall_f(e),o);
    o << "(";
    {
      bool start = true;
      FOR_SEQUENCE(Expression,arg,Actuals,funcall_actuals(e),
		   if (start) start = false;
		   else o << ",";
		   dump_Expression(arg,o));
      Declarations fs = function_type_formals(infer_expr_type(funcall_f(e)));
      for (Declaration f=first_Declaration(fs); f; f=DECL_NEXT(f))
	if (Declaration_KEY(f) == KEYseq_formal) {
	  if (start) start = false; else o << ",";
	  o << "0";
	}
    }
    o << ")";
    break;
  case KEYvalue_use:
    {
      Use u = value_use_use(e);
      Declaration d = USE_DECL(u);
      if (Declaration_info(d)->decl_flags & IMPLEMENTATION_MARKS) {
	impl->implement_value_use(d,o);
      } else if (Declaration_info(d)->decl_flags & IMPLEMENTED_PATTERN_VAR) {
	o << *((string*)(Pattern_info((Pattern)tnode_parent(d))->pat_impl));
      } else {
	if (Declaration_KEY(d) == KEYfunction_decl ||
	    (Declaration_KEY(d) == KEYvalue_renaming &&
	     Type_KEY(infer_expr_type(e)) == KEYfunction_type)) {
	  // function values only legal in particular situations.
	  Expression e2 = (Expression)tnode_parent(e);
	  Declaration d2 = (Declaration)e2;
	  if ((Expression_info(e)->expr_flags & EXPR_FUNCTION_IS_OK) == 0 &&
	      ((ABSTRACT_APS_tnode_phylum(e2) != KEYExpression ||
		Expression_KEY(e2) != KEYfuncall) &&
	       (ABSTRACT_APS_tnode_phylum(d2) != KEYDeclaration ||
		Declaration_KEY(d2) != KEYvalue_renaming) &&
	       (ABSTRACT_APS_tnode_phylum(d2) != KEYDefault))) {
	    void *u = tnode_parent(d);
	    if (ABSTRACT_APS_tnode_phylum(u) != KEYUnit) {
	      aps_error(e,"Cannot generate code for functions used in this way");
	    }
	  }
	}
	dump_Use(u,"v_",o);
      }
    }
    break;
  case KEYtyped_value:
    dump_Expression(typed_value_expr(e),o);
    break;
  default:
    aps_error(e,"cannot handle this kind of expression");
  }
}

static void dump_TypeContour(TypeContour *tc, bool instance, ostream& os) 
{
  if (!instance && inline_definitions) os << "typename ";
  os << "C_";
  os << decl_name(tc->source);
  vector<Type> type_actuals;
  switch (Declaration_KEY(tc->source)) {
  case KEYpolymorphic:
    {
      int n=0;
      for (Declaration f=first_Declaration(tc->type_formals);
	   f; f=DECL_NEXT(f))
	++n;
      for (int i=0; i < n; ++i) {
	type_actuals.push_back(tc->u.inferred[i]);
      }
    }
    break;
  case KEYmodule_decl:
    for (Type ta = first_TypeActual(tc->u.type_actuals);
	 ta ; ta = TYPE_NEXT(ta)) {
      type_actuals.push_back(ta);
    }
    break;
  default:
    fatal_error("%d: Not a source", tnode_line_number(tc->source));
  }

  bool started = false;
  for (unsigned i=0; i < type_actuals.size(); ++i) {
    if (started) os << ","; else { os << "< "; started = true;}
    if (type_actuals[i] == 0) {
      fatal_error("insufficient type inference");
    }
    dump_Type_signature(type_actuals[i],os);
  }
  if (started) os << ">";
  if (instance) {
    started = false;
    for (unsigned i=0; i < type_actuals.size(); ++i) {
      if (started) os << ","; else { os << "("; started = true;}
      dump_Type_value(type_actuals[i],os);
    }
    if (started) os << ")";
  }
}

static void dump_poly_inst(Use u, TypeEnvironment type_env, ostream& os)
{
  if (type_env == 0) return;
  if (!type_env->source || Declaration_KEY(type_env->source) != KEYpolymorphic) return;
  dump_poly_inst(u,type_env->outer,os);
  dump_TypeContour(type_env,true,os);
  os << ".";
}

void dump_TypeEnvironment(TypeEnvironment te, ostream&os)
{
  if (!te) return;
  dump_TypeEnvironment(te->outer,os);
  dump_TypeContour(te,false,os);
  os << "::";
}

void dump_Use(Use u, const char *prefix, ostream& os)
{
  Symbol sym;
  switch (Use_KEY(u)) {
  case KEYuse:
    sym = use_name(u);
    break;
  case KEYqual_use:
    {
      dump_Type_value(qual_use_from(u),os);
      os << "->";
      sym = qual_use_name(u);
      break;
    }
  }
  dump_poly_inst(u,USE_TYPE_ENV(u),os);
  os << prefix << sym;
}

void debug_fiber(FIBER fiber, ostream& os) {
  while (fiber != NULL && fiber->field != NULL) {
    if (FIELD_DECL_IS_REVERSE(fiber->field)) {
      os << "X" << decl_name(reverse_field(fiber->field));
    } else {
      os << decl_name(fiber->field);
    }
    fiber = fiber->shorter;
    if (fiber->field != NULL) os << "_";
  }
}

void debug_fibered_attr(FIBERED_ATTRIBUTE* fa, ostream& os)
{
  if (ATTR_DECL_IS_SHARED_INFO(fa->attr))
    os << "__global";
  else
    os << decl_name(fa->attr);
  if (fa->fiber == 0) return;
  os << "$";
  debug_fiber(fa->fiber,os);
}

void debug_Instance(INSTANCE *i, ostream& os) {
  if (i->node != NULL) {
    if (ABSTRACT_APS_tnode_phylum(i->node) != KEYDeclaration) {
      os << tnode_line_number(i->node) << ":?<" 
	 << ABSTRACT_APS_tnode_phylum(i->node) << ">";
    } else if (Declaration_KEY(i->node) == KEYnormal_assign) {
      Declaration pdecl = proc_call_p(normal_assign_rhs(i->node));
      os << decl_name(pdecl) << "@";
    } else if (Declaration_KEY(i->node) == KEYpragma_call) {
      os << pragma_call_name(i->node) << "(...):" 
	 << tnode_line_number(i->node);
    } else {
      os << symbol_name(def_name(declaration_def(i->node)));
    }
    os << ".";
  }
  if (i->fibered_attr.attr == NULL) {
    os << "(nil)";
  } else if (ABSTRACT_APS_tnode_phylum(i->fibered_attr.attr) == KEYMatch) {
    os << "<match@" << tnode_line_number(i->fibered_attr.attr) << ">";
  } else switch(Declaration_KEY(i->fibered_attr.attr)) {
  case KEYcollect_assign: {
    Expression lhs = collect_assign_lhs(i->node);
    Declaration field = field_ref_p(lhs);
    os << "<collect[" << decl_name(field) << "]@"
       << tnode_line_number(i->fibered_attr.attr) << ">";
  }
  case KEYif_stmt:
  case KEYcase_stmt:
    os << "<cond@" << tnode_line_number(i->fibered_attr.attr) << ">";
    break;
  default:
    os << symbol_name(def_name(declaration_def(i->fibered_attr.attr)));
  }
  if (i->fibered_attr.fiber != NULL) {
    os << "$";
    debug_fiber(i->fibered_attr.fiber,os);
  }
}

string operator+(string s, int i)
{
  ostringstream os;
  os << s << i;
  return os.str();
}
