#include <iostream>
#include <cctype>
#include <stack>
#include <map>
#include <sstream>
#include <vector>
#include <string>

extern "C" {
#include "aps-ag.h"
String get_code_name(Symbol);
}

#include "dump-cpp.h"

using namespace std;

using std::string;

// extra decl_flags flags:
#define LOCAL_ATTRIBUTE_FLAG (1<<24)
#define IMPLEMENTED_PATTERN_VAR (1<<25)

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

/*! hack! */
bool is_global(Type ty)
{
  switch (Type_KEY(ty)) {
  case KEYtype_use:
    {
      Declaration d = USE_DECL(type_use_use(ty));
      void *p;
      for (p = tnode_parent(d);
	   p &&
	     (ABSTRACT_APS_tnode_phylum(p) != KEYDeclaration ||
	      Declaration_KEY((Declaration)p) != KEYmodule_decl);
	   p = tnode_parent(p))
	;
      return p == 0;
    }
  default:
    return false; /*!!! Not really true */
  }
}

// parameterizations and options:

static char* omitted[80];
static int omitted_number = 0;

void omit_declaration(char *n)
{
  omitted[omitted_number++] = n;
}

static char*impl[80];
static int impl_number = 0;

void impl_module(char *mname, char*type)
{
  impl[impl_number++] = mname;
  impl[impl_number++] = type;
}

bool static_schedule = false;
bool incremental = false; //! unused
int verbose = 0;

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

struct output_streams {
  Declaration context;
  ostream &hs, &cpps, &is;
  output_streams(Declaration _c, ostream &_hs, ostream &_cpps, ostream &_is)
    : context(_c), hs(_hs), cpps(_cpps), is(_is) {}
};

void dump_cpp_Declaration(Declaration,const output_streams&);

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
    dump_cpp_Declaration(decl_unit_decl(u),output_streams(NULL,hs,cpps,is));
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

void dump_formal(Declaration formal,char *prefix,ostream&s)
{
  dump_Typed_decl(infer_formal_type(formal),formal,prefix,s);
  if (KEYseq_formal == Declaration_KEY(formal)) s << ",...";
}

void dump_Type_prefixed(Type,ostream&);

void dump_function_prototype(String prefix,char *name, Type ft, ostream&hs)
{
  Declarations formals = function_type_formals(ft);
  Declaration returndecl = first_Declaration(function_type_return_values(ft));
  if (returndecl == 0) {
    hs << "void ";
  } else {
    Type return_type = value_decl_type(returndecl);
    if (prefix != empty_string) {
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

#define MAXDEPTH 200

static int start_indent = 4;
static const int increment_indent = 2;
static void *attr_context[MAXDEPTH];
static int attr_context_depth = 0;   /* current depth of attribute assigns */
static int attr_context_started = 0; /* depth of last activation */

#define MAXINDENT 40
static char indentspaces[MAXINDENT+1];

static int get_indent()
{
  return start_indent + increment_indent * attr_context_started;
}

static char *indent()
{
  if (indentspaces[0] == '\0') {
    for (int i=0; i < MAXINDENT; ++i)
      indentspaces[i] = ' ';
  }
  int n = get_indent();
  if (n > MAXINDENT) n = MAXINDENT;
  return indentspaces + MAXINDENT - n;
}

static void push_attr_context(void *node)
{
  if (attr_context_depth >= MAXDEPTH) {
    aps_error(node,"nested too deep");
  } else {
    attr_context[attr_context_depth++] = node;
  }
}

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
    dump_seq_function(st,type_element_type(st),"empty",os);
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
	ostringstream ns;
	dump_seq_function(infer_pattern_type(pa),st,"first",ns);
	ns << "(" << node << ")";	
	dump_Pattern_cond(pa,ns.str(),os);
	os << "&&";
	ostringstream rs;
	dump_seq_function(infer_pattern_type(pa),st,"butfirst",rs);
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
      if (def_name(declaration_def(pfdecl)) == seq_symbol) {
	dump_seq_Pattern_cond(first_PatternActual(pactuals),rt,node,os);
	return;
      } else if (Declaration_KEY(pfdecl) != KEYconstructor_decl) {
	aps_error(pfuse,"Cannot handle calls to pattern functions");
	return;
      }
      os << node << "->cons==";
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

static void dump_context_open(void *c, ostream& os) {
  switch (ABSTRACT_APS_tnode_phylum(c)) {
  case KEYDeclaration:
    {
      Declaration decl = (Declaration)c;
      switch (Declaration_KEY(decl)) {
      case KEYif_stmt:
	os << indent() << "{ bool cond = ";
	dump_Expression(if_stmt_cond(decl),os);
	os << ";\n";
	return;
      case KEYcase_stmt:
	{
	  Type ty = infer_expr_type(case_stmt_expr(decl));
	  os << indent() << "{ ";
	  dump_Type(ty,os);
	  os << " node = ";
	  dump_Expression(case_stmt_expr(decl),os);
	  os << ";\n";
	  os << indent() << "  Constructor* cons = node->cons;\n";
	}
	return;
      case KEYfor_stmt:
	{
	  //!! Doesn't work.
	  Type ty = infer_expr_type(for_stmt_expr(decl));
	  os << indent() << "{ ";
	  dump_Type(ty,os);
	  os << " node = ";
	  dump_Expression(for_stmt_expr(decl),os);
	  os << ";\n";
	  os << indent() << "Constructor* cons = node->cons;\n";
	}
	return;
      case KEYfor_in_stmt:
	{
	  Declaration f = for_in_stmt_formal(decl);
	  Type ty = infer_formal_type(f);
	  os << indent() << "for (CollectionIterator<";
	  dump_Type(ty,os);
	  os << "> ci = CollectionIterator<";
	  dump_Type(ty,os);
	  os << ">(";
	  dump_Expression(for_in_stmt_seq(decl),os);
	  os << "); ci.has_item(); ci.advance()) {";
	  os << indent() << "  ";
	  dump_Type(ty,os);
	  os << " v_" << decl_name(f) << " = ci.item();\n";
	}
	return;
      case KEYtop_level_match:
	{
	  Type ty = infer_pattern_type(matcher_pat(top_level_match_m(decl)));
	  os << indent() << "for (int i=";
	  dump_Type_value(ty,os);
	  os << "->size(); i >= 0; --i) {\n";
	  os << indent() << "  ";
	  dump_Type(ty,os);
	  os << " node = ";
	  dump_Type_value(ty,os);
	  os << "->node(i);\n";
	  os << indent() << "Constructor* cons = node->cons;\n";
	}
	return;
      default:
	break;
      }
    }
    break;
  case KEYBlock:
    {
      Block b = (Block)c;
      // only for if_stmt for now
      Declaration parent = (Declaration)tnode_parent(b);
      if (b == if_stmt_if_true(parent)) {
	os << indent() << "if (cond) {\n";
      } else {
	os << indent() << "if (!cond) {\n";
      }
      return;
    }
  case KEYMatch:
    {
      Match m = (Match)c;
      Pattern p = matcher_pat(m);
      os << indent() << "if (";
      dump_Pattern_cond(p,"node",os);
      os << ") {\n";
      start_indent+=2;
      dump_Pattern_bindings(p,os);
      start_indent-=2;
#ifdef UNDEF
      Use pfuse;
      switch (Pattern_KEY(p)) {
      case KEYand_pattern:
	p = and_pattern_p2(p);
	break;
      default:
	break;
      }
      switch (Pattern_KEY(p)) {
      default:
	aps_error(p,"cannot find constructor (can only handle ?x=f(...) or f(...)");
	return;
      case KEYpattern_call:
	p = pattern_call_func(p);
	break;
      }
      switch (Pattern_KEY(p)) {
      default:
	aps_error(p,"cannot find constructor (can only handle ?x=f(...) or f(...)");
	return;
      case KEYpattern_use:
	pfuse = pattern_use_use(p);
	break;
      }
      Declaration cd = match_constructor_decl(m);
      Type ct = constructor_decl_type(cd);
      char *cname = decl_name(cd);
      os << indent() << "if (cons == ";
      dump_Use(pfuse,"c_",os);
      os << ") {\n";
      os << indent() << "  ";
      dump_TypeEnvironment(USE_TYPE_ENV(pfuse),os);
      os << "V_" << cname << "* n = (";
      dump_TypeEnvironment(USE_TYPE_ENV(pfuse),os);
      os << "V_" << cname << "*)node;\n";
      Declaration ld = match_lhs_decl(m);
      if (ld != NULL && !streq(decl_name(ld),"_")) {
	os << indent() << "  ";
	dump_Type(infer_formal_type(ld),os);
	os << " v_" << decl_name(ld) << " = n;\n";
      }
      Declaration rd = match_first_rhs_decl(m);
      Declaration cf = first_Declaration(function_type_formals(ct));
      for (; rd != NULL && cf != NULL;
	   rd = next_rhs_decl(rd), cf = DECL_NEXT(cf)) {
	if (streq(decl_name(rd),"_")) continue;
	os << indent() << "  ";
	dump_Type(infer_formal_type(rd),os);
	os << " v_" << decl_name(rd) << " = n->v_"
	   << decl_name(cf) << ";\n";
      }
#endif
      return;
    }
  default:
    break;
  }
  aps_error(c,"unknown context");
  os << indent() << "UNKNOWN {\n";
}

static void activate_attr_context(ostream& os)
{
  while (attr_context_started < attr_context_depth) {
    dump_context_open(attr_context[attr_context_started],os);
    ++attr_context_started;
  }
}

static void pop_attr_context(ostream& os)
{
  --attr_context_depth;
  while (attr_context_started > attr_context_depth) {
    --attr_context_started;
    os << indent() << "}\n";
  }
}

static void dump_attr_assign(void *vdecl, Declaration ass, ostream& os)
{
  Declaration decl = (Declaration)vdecl;
  Expression lhs = assign_lhs(ass);
  Expression rhs = assign_rhs(ass);
  Direction dir;
  Default def;
  Type ty = infer_expr_type(lhs);
  switch (Expression_KEY(lhs)) {
  case KEYvalue_use:
    if (USE_DECL(value_use_use(lhs)) != decl) return;
    dir = value_decl_direction(decl);
    def = value_decl_default(decl);
    activate_attr_context(os);
    os << indent();
    break;
  case KEYfuncall:
    if (USE_DECL(value_use_use(funcall_f(lhs))) != decl) return;
    activate_attr_context(os);
    dir = attribute_decl_direction(decl);
    def = attribute_decl_default(decl);
    os << indent() << "if (anode == ";
    dump_Expression(first_Actual(funcall_actuals(lhs)),os);
    os << ") ";
    break;
  default:
    return;
  }
  if (direction_is_collection(dir)) {
    os << "collection = ";
    switch (Default_KEY(def)) {
    case KEYno_default:
    case KEYsimple:
      dump_Type_value(ty,os);
      os << "->v_combine";
      break;
    case KEYcomposite:
      dump_Expression(composite_combiner(def),os);
      break;
    }
    os << "(collection,";
    dump_Expression(rhs,os);
    os << ");\n";
  } else {
    os << "return ";
    dump_Expression(rhs,os);
    os << ";\n";
  }
}

bool depends_on(void *vdecl, Declaration local, Block b)
{
  //! need to check if this is needed
  return true;
}

void dump_local_decl(void *, Declaration local, ostream& o)
{
  activate_attr_context(o);
  o << indent();
  dump_Typed_decl(value_decl_type(local),local,"v_",o);
  o << " = ";
  Default init = value_decl_default(local);
  switch (Default_KEY(init)) {
  case KEYsimple:
    dump_Expression(simple_value(init),o);
    break;
  default:
    aps_error(local,"Can only handle initialized locals");
    o << "0";
  }
  o << ";\n";
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
      char *mname = symbol_name(use_name(module_use_use(type_inst_module(ty))));
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
      char *mname = symbol_name(use_name(module_use_use(type_inst_module(ty))));
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
#ifdef UNDEF
      switch (Declaration_KEY(td)) {
      case KEYtype_decl:
	dump_Type_superinit(false,type_decl_type(td),os);
	return;
      case KEYphylum_decl:
	dump_Type_superinit(true,phylum_decl_type(td),os);
	return;
      case KEYtype_renaming:
	dump_Type_superinit(true,type_renaming_old(td),os);
	return;
      default:
	break; // fall through 
      }
      // fall through
#endif
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
#ifdef UNDEF
	    //don't need to transfer values:
	    os << "  C_" << name << " *t_" << name << ";\n";
	    is << ",\n    t_" << name << "((";
	    dump_Type_value(from,is);
	    is << ")->t_" << name <<")";
	    break;
	  case KEYconstructor_decl:
	  case KEYfunction_decl:
	  case KEYprocedure_decl:
	  case KEYattribute_decl:
	    {
	      Type ft = type_subst(fake,some_value_decl_type(d));
	      Declarations formals = function_type_formals(ft);
	      os << "  ";
	      dump_function_prototype(empty_string,name,ft,os);
	      os << "{\n    return (";
	      dump_Type_value(from,os);
	      os << ")->v_" << name << "(";
	      for (Declaration formal = first_Declaration(formals);
		   formal != NULL;
		   formal = DECL_NEXT(formal)) {
		if (formal != first_Declaration(formals))
		  os << ",";
		os << "v_" << decl_name(formal);
	      }
	      os << ");\n  }\n";
	    }
	    break;
	  case KEYvalue_decl:
	    {
	      Type vt = type_subst(fake,some_value_decl_type(d));
	      os << "  ";
	      dump_Type(vt,os);
	      os << " v_" << name << ";\n";
	      is << ",\n    v_" << name << "((";
	      dump_Type_value(from,os);
	      os << ")->v_" << name << ")";
	    }
	    break;
	  default:
	    if (ns & NAME_VALUE || ns & NAME_PATTERN) {
	      printf("unable to transfer service %s",
		     decl_name(d));
	    }
#else
	    break;
	  default:
#endif
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
#ifdef UNDEF
      if (as) {
	char* key;
	Symbol sym;
	if (is_phylum) {
	  key = "Phylum";
	  sym = phylum_sym;
	} else {
	  key = "Type";
	  sym = type_sym;
	}
	if (!sr[sym]) {
	  os << "  " << key << "* get" << sym << "() const { return (";
	  dump_Type_value(ty,os);
	  os << ")->get" << sym << "(); }\n";
	  sr[sym] |= NAME_TYPE;
	}
      }
#endif
    }
    break;
  }
}

void dump_local_attribute(Declaration d, Type at, Declaration context,
			  String prefix, const output_streams& oss)
{
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  ostream& is = oss.is;
  // ostream& bs = inline_definitions ? hs : cpps;

  char *name = decl_name(d);
  int i = Declaration_info(d)->instance_index; // unique number
  Type lt = value_decl_type(d);
	
      
  hs << " private:\n"
     << "  class A" << i << "_" << name << " : public Attribute<";
  dump_Type_signature(at,hs);
  hs << ",";
  dump_Type_signature(lt,hs);
  hs << "> {\n";
  hs << "    C_" << decl_name(context) << "* context;\n";
  hs << "  public:\n";
  hs << "    A" << i << "_" << name << "(C_" << decl_name(context) << "*c,";
  dump_Type_signature(at,hs);
  hs << " *nt, ";
  dump_Type_signature(lt,hs);
  hs << " *vt)";
  if (!inline_definitions) {
    hs << ";\n";
    cpps << "\n" << prefix << "A" << i << "_" << name
	 << "::A" << i << "_" << name
	 << "(C_" << decl_name(context) << "*c,";
    dump_Type_signature(at,cpps);
    cpps << " *nt, ";
    dump_Type_signature(lt,cpps);
    cpps << " *vt)";
  }
  cpps << " : Attribute<";
  dump_Type_signature(at,cpps);
  cpps << ",";
  dump_Type_signature(lt,cpps);
  cpps << ">(nt,vt,\"local " << i << "(" << name << ")\"), context(c) {}\n";
  hs << "    ";
  dump_Type(lt,hs);
  hs << " compute(";
  dump_Type(at,hs);
  hs << " node)";
  if (!inline_definitions) {
    hs << ";\n";
    dump_Type(lt,cpps);
    cpps << " " << prefix << "A" << i << "_" << name << "::compute(";
    dump_Type(at,cpps);
    cpps << " node)";
  }
  cpps << "{ return context->c" << i << "_" << name << "(node); }\n";
  hs << "};\n public:\n";

  hs << "  A" << i << "_" << name << " *a" << i << "_" << name << ";\n";
      
  is << ",\n    a" << i << "_" << name << "(new A" << i << "_" << name << "(";
  is << "this,";
  dump_Type_value(at,is);
  is << ",";
  dump_Type_value(lt,is);
  is << "))";
}

// record where we found a local attribute 
// (used in dynamic scheduling only)
struct LocalAttribute {
  Declaration local;
  Block block;
  string context_bindings; // string of local declarations from "anchor"
  LocalAttribute(Declaration l, Block b, string cb)
    : local(l), block(b), context_bindings(cb) {}
};

vector<LocalAttribute> local_attributes;

void dump_local_attributes(Block b, Type at, Declaration context,
			   string bindings, String prefix,
			   const output_streams& oss)
{
  for (Declaration d = first_Declaration(block_body(b)); d; d=DECL_NEXT(d)) {
    switch (Declaration_KEY(d)) {
    default:
      aps_error(d,"Cannot handle this kind of statement");
    case KEYvalue_decl:
      // only for dynamic scheduling:
      assert(static_schedule == false);
      Declaration_info(d)->instance_index = local_attributes.size();
      Declaration_info(d)->decl_flags |= LOCAL_ATTRIBUTE_FLAG;
      local_attributes.push_back(LocalAttribute(d,b,bindings));
      dump_local_attribute(d,at,context,prefix,oss);
      break;
    case KEYassign:
      break;
    case KEYblock_stmt:
      dump_local_attributes(block_stmt_body(d),at,context,bindings,prefix,oss);
      break;
    case KEYif_stmt:
      dump_local_attributes(if_stmt_if_true(d),at,context,bindings,prefix,oss);
      dump_local_attributes(if_stmt_if_false(d),at,context,bindings,prefix,oss);
      break;
    case KEYcase_stmt:
      {
	Expression expr = case_stmt_expr(d);
	static int unique = 0;
	ostringstream cns;
	cns << "cn" << ++unique;
	string cn = cns.str();

	ostringstream ns;
	ns << "  "; if (inline_definitions) ns << "  ";
	dump_Type(infer_expr_type(expr),ns);
	ns << " " << cn << " = ";
	dump_Expression(expr,ns);
	ns << ";\n";
	string more_bindings = bindings + ns.str();

	FOR_SEQUENCE
	  (Match,m,Matches,case_stmt_matchers(d),
	   dump_local_attributes(matcher_body(m),at,context,
				 more_bindings + matcher_bindings(cn,m),
				 prefix,oss));
	// don't need "cn" in default branch:
	dump_local_attributes(case_stmt_default(d),at,context,bindings,prefix,oss);
      }
      break;
    }
  }
}

void implement_local_attributes(String prefix, const output_streams& oss)
{
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  // ostream& is = oss.is;
  ostream& bs = inline_definitions ? hs : cpps;

  int n = local_attributes.size();
  for (int i=0; i <n; ++i) {
    Declaration d = local_attributes[i].local;
    Block b = local_attributes[i].block;
    char *name = decl_name(d);
    hs << "  ";
    dump_Type(value_decl_type(d),hs);
    hs << " c" << i << "_" << name << "(C_PHYLUM::Node* anchor)";
    if (!inline_definitions) {
      hs << ";\n";
      dump_Type_prefixed(value_decl_type(d),cpps);
      cpps << " " << prefix << "c" << i << "_" << name 
	   << "(C_PHYLUM::Node* anchor)";
    }
    bs << " {\n";
    bs << local_attributes[i].context_bindings;
    bs << "\n"; // blank line
    dump_Block(b,dump_attr_assign,d,bs);
    if (inline_definitions) hs << "  ";
    if (direction_is_collection(value_decl_direction(d))) {
      bs << "  return collection;\n";
    } else switch (Default_KEY(value_decl_default(d))) {
    default:
      bs << "  throw UndefinedAttributeException(\"local " << i << "("
	 << name << ")\");\n";
      break;
    case KEYsimple:
      bs << "  return ";
      dump_Expression(simple_value(value_decl_default(d)),bs);
      bs << ";\n";
      break;
    }
    if (inline_definitions) hs << "  ";
    bs << "}\n";
  }
}

void implement_attributes(Declaration first_decl, Declaration context,
			  const output_streams& oss)
{
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  // ostream& is = oss.is;
  ostream& bs = inline_definitions ? hs : cpps;

  if (!first_decl) return;

  String prefix = get_prefix(first_decl);

  for (Declaration decl = first_decl; decl; decl=DECL_NEXT(decl)) {
    if (Declaration_KEY(decl) != KEYattribute_decl) continue;
    char *name = decl_name(decl);
    Declarations afs = function_type_formals(attribute_decl_type(decl));
    Declaration af = first_Declaration(afs);
    Type at = formal_type(af);
    // Declaration pd = USE_DECL(type_use_use(at));
    Declarations rdecls = function_type_return_values(attribute_decl_type(decl));
    Type rt = value_decl_type(first_Declaration(rdecls));
    bool inh = (ATTR_DECL_IS_INH(decl) != 0);
    
    if (!static_schedule) {
      hs << "  "; dump_Type(rt,hs); hs << " c_" << name << "(";
      dump_Type(at,hs);
      hs << " anode)";
      if (!inline_definitions) {
	hs << ";\n";
	dump_Type(rt,cpps);
	cpps << " " << prefix << "c_" << name << "(";
	dump_Type(at,cpps);
	cpps << " anode)";
      }
      bs << " {\n";  if (inline_definitions) hs << "  ";
      if (inh) {
	bs << "  C_PHYLUM::Node* node=anode->parent;\n";
	bs << "  if (node != 0) {\n";
      } else {
	bs << "  C_PHYLUM::Node* node = anode;\n";
      }
      if (inline_definitions) hs << "  ";
      bs << "  C_PHYLUM::Node* anchor = node;\n";
      if (inline_definitions) hs << "  ";
      bs << "  Constructor* cons = node->cons;\n";
      for (Declaration d=first_Declaration(block_body(module_decl_contents(context)));
	   d != NULL; d = DECL_NEXT(d)) {
	switch (Declaration_KEY(d)) {
	case KEYtop_level_match:
	  {
	    Match m = top_level_match_m(d);
	    push_attr_context(m);
	    Block body = matcher_body(top_level_match_m(d));
	    dump_Block(body,dump_attr_assign,decl,bs);
	    pop_attr_context(bs);
	  }
	  break;
	default:
	  break;
	}
      }
      if (inh) {
	bs << "  }\n";
      }
      if (direction_is_collection(attribute_decl_direction(decl))) {
	bs << "  return collection;\n";
      } else switch (Default_KEY(attribute_decl_default(decl))) {
      default:
	bs << "  throw UndefinedAttributeException(std::string(\""
	     << name << " of \")+anode);\n";
	break;
      case KEYsimple:
	bs << "  return ";
	dump_Expression(simple_value(attribute_decl_default(decl)),bs);
	bs << ";\n";
	break;
      }
      bs << "}\n";
    }
  }
}

void dump_cpp_Declaration(Declaration decl,const output_streams& oss)
{
  ostream& hs = oss.hs;
  ostream& cpps = oss.cpps;
  ostream& is = oss.is;
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
  String prefix = get_prefix(decl);
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
	if (streq(impl[j],name)) impl_type = impl[j+1];
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
      if (impl_type) {
	Type_info(rut)->impl_type = impl_type;
      }
      dump_Type_superclass(rdecl_is_phylum,rut,hs);
      hs << " {\n";
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
      // is << "\n    /* starting transfers */";
      is.flush();

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

      // is << " /* transfers over */ ";

      for (Declaration d = first_decl; d ; d = DECL_NEXT(d)) {
	//if (decl_namespaces(d)) {
	//  is << "\n    /* about to do " << decl_name(d) << " */";
	//  is.flush();
	//}
	dump_cpp_Declaration(d,output_streams(decl,hs,bs,is));
	// undefined simple variables mean the constructor must be native:
	if (Declaration_KEY(d) == KEYvalue_decl) {
	  Default def = value_decl_default(d);
	  Direction dir = value_decl_direction(d);
	  if (Default_KEY(def) == KEYno_default &&
	      !direction_is_circular(dir) &&
	      !direction_is_collection(dir) &&
	      !direction_is_input(dir))
	    constructor_is_native = true;
	}
      }

      //is << "\n    /* body over */"; is.flush();

      first_decl = first_Declaration(body); // result is special
      if (static_schedule) {
	// schedule_attributes();
	fatal_error("static scheduling not implemented");
      } else {
	implement_local_attributes(prefix,oss);
	implement_attributes(first_decl,decl,oss);
      }

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

      hs << "  void finish()";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << "void C_" << name << "::finish()";
      }
      bs << " {";
      for (Declaration d = first_decl; d; d = DECL_NEXT(d)) {
	char* kind = NULL;
	switch(Declaration_KEY(d)) {
	case KEYphylum_decl:
	case KEYtype_decl:
	  kind = "t_";
	  break;
	case KEYattribute_decl:
	  kind = "a_";
	  break;
	default:
	  break;
	}
	if (kind != NULL) {
	  char *n = decl_name(d);
	  bs << "\n  " << kind << n << "->finish(); ";
	}
      }
      bs << " }\n\n";
      inline_definitions -= is_template;
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
	    cpps << "C_" << name << " *t_" << name << " = ";
	    dump_Type_value(type,cpps);
	    cpps << ";\n";
	  }
	  hs << endl;
	}
	break;
      }
      if (!is_result && context) {
	is << ",\n    t_" << name << "(";
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

      dump_function_prototype(empty_string,name,ft,hs);
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
	dump_Default(value_decl_default(decl),is);
	is << ")";
      }
    }
    break;
  case KEYattribute_decl:
    {
      Declarations afs = function_type_formals(attribute_decl_type(decl));
      Declaration af = first_Declaration(afs);
      Type at = formal_type(af);
      // Declaration pd = USE_DECL(type_use_use(at));
      Declarations rdecls = function_type_return_values(attribute_decl_type(decl));
      Type rt = value_decl_type(first_Declaration(rdecls));
      
      hs << " private:\n";
      hs << "  class A_" << name << " : public Attribute<";
      dump_Type_signature(at,hs);
      hs << ",";
      dump_Type_signature(rt,hs);
      hs << "> {\n";
      hs << "    C_" << decl_name(context) << "* context;\n";
      hs << "  public:\n";
      hs << "    A_" << name << "(C_" << decl_name(context) << "*c,";
      dump_Type_signature(at,hs);
      hs << " *nt, ";
      dump_Type_signature(rt,hs);
      hs << " *vt)";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << "\n" << prefix << "A_" << name << "::A_" << name
	     << "(C_" << decl_name(context) << "*c,";
	dump_Type_signature(at,cpps);
	cpps << " *nt, ";
	dump_Type_signature(rt,cpps);
	cpps << " *vt)";
      }
      cpps << " : Attribute<";
      dump_Type_signature(at,cpps);
      cpps << ",";
      dump_Type_signature(rt,cpps);
      cpps << ">(nt,vt,\"" << name << "\"), context(c) {}\n";
      hs << "    ";
      dump_Type(rt,hs);
      hs << " compute(";
      dump_Type(at,hs);
      hs << " node)";
      if (!inline_definitions) {
	hs << ";\n";
	dump_Type(rt,cpps);
	cpps << " " << prefix << "A_" << name << "::compute(";
	dump_Type(at,cpps);
	cpps << " node)";
      }
      if (static_schedule) {
	cpps << "{\n";
	if (inline_definitions) hs << "    ";
	cpps << "  T_String ns = values->v_string(node);\n";
	if (inline_definitions) hs << "    ";
	cpps << "  throw UndefinedAttributeException(ns+\"."
	     << name << "\");\n";
	if (inline_definitions) hs << "    ";
	cpps << "}\n";
      } else {
	cpps << "{ return context->c_" << name << "(node); }\n";
	hs << "};\n public:\n";
      }

      hs << "  A_" << name << " *a_" << name << ";\n";
      
      is << ",\n    a_" << name << "(new A_" << name << "(";
      is << "this,";
      dump_Type_value(at,is);
      is << ",";
      dump_Type_value(rt,is);
      is << "))";

      hs << "  ";
      dump_function_prototype(empty_string,name,attribute_decl_type(decl),hs);
      if (!inline_definitions) {
	hs << ";\n" << endl; 
	dump_function_prototype(prefix,name,attribute_decl_type(decl),cpps);
      }
      cpps << " {\n";
      if (static_schedule) {
	if (inline_definitions) hs << "    ";
	cpps << "  finish();\n";
      }
      if (inline_definitions) hs << "    ";
      cpps << "  return a_" << name << "->evaluate(v_";
      cpps << decl_name(af) << ");\n";
      if (inline_definitions) hs << "    ";
      cpps << "}\n" << endl; 
    }
    break;
  case KEYfunction_decl:
    {
      Type fty = function_decl_type(decl);
      Declaration rdecl = first_Declaration(function_type_return_values(fty));
      Block b = function_decl_body(decl);
      if (context) hs << "  ";
      dump_function_prototype(empty_string,name,fty,hs);
      if (!inline_definitions) {
	hs << ";\n" << endl;
      }
      // three kinds of definitions:
      // 1. simple default
      if (rdecl) {
	switch (Default_KEY(value_decl_default(rdecl))) {
	case KEYsimple:
	  if (!inline_definitions) {
	    dump_function_prototype(prefix,name,fty,cpps);
	  }
	  cpps << "{ return ";
	  dump_Expression(simple_value(value_decl_default(rdecl)),cpps);
	  cpps << "; }\n";
	  return;
	default:
	  break;
	}
      }
      if (first_Declaration(block_body(b))) {
	// 2. non-empty body
	if (!inline_definitions) {
	  dump_function_prototype(prefix,name,fty,cpps);
	}
	cpps << "{\n";
	dump_Block(b,dump_attr_assign,rdecl,cpps);
	cpps << indent()
	     << "throw UndefinedAttributeException(\"" << name << "\");\n";
	if (inline_definitions) cpps << "  ";
	cpps << "}\n" << endl;
      } else {
	// cout << name << " has no body.\n";
	if (inline_definitions) hs << ";\n";
	// 3. nothing -- leave to native code
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
	dump_cpp_Declaration(d,output_streams(decl,hs,hs,is));
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
    }
    --inline_definitions;
    break;
  case KEYtop_level_match:
    if (!static_schedule) {
      Match m = top_level_match_m(decl);
      Declaration cdecl = top_level_match_constructor_decl(decl);
      Type ct = constructor_decl_type(cdecl);
      Declaration rdecl = first_Declaration(function_type_return_values(ct));
      Type anchor_type = value_decl_type(rdecl);
      Block body = matcher_body(m);
      dump_local_attributes(body,anchor_type,context,
			    matcher_bindings("anchor",m),prefix,oss);
    }
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
	dump_function_prototype(empty_string,name,ty,hs);
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
	is << ",\n    t_" << name << "(";
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

void dump_Matches(Matches ms, bool exclusive, ASSIGNFUNC f, void*arg, ostream&os)
{
  FOR_SEQUENCE
    (Match,m,Matches,ms,
     push_attr_context(m);
     dump_Block(matcher_body(m),f,arg,os);
     bool need_else = attr_context_started >= attr_context_depth;
     pop_attr_context(os);
     if (exclusive && need_else) os << indent() << "else\n";
     );
}

void dump_Block(Block b,ASSIGNFUNC f,void*arg,ostream&os)
{
  FOR_SEQUENCE
    (Declaration,d,Declarations,block_body(b),
     switch (Declaration_KEY(d)) {
     case KEYassign:
       (*f)(arg,d,os);
       break;
     case KEYif_stmt:
       push_attr_context(d);
       { Block true_block = if_stmt_if_true(d);
         Block false_block = if_stmt_if_false(d);
	 push_attr_context(true_block);
	 dump_Block(true_block,f,arg,os);
	 pop_attr_context(os);
	 push_attr_context(false_block);
	 dump_Block(false_block,f,arg,os);
	 pop_attr_context(os);
       }
       pop_attr_context(os);
       break;
     case KEYcase_stmt:
       push_attr_context(d);
       //!! we implement case and for!!
       dump_Matches(case_stmt_matchers(d),true,f,arg,os);
       os << indent() << "{\n"; // because of "else"
       dump_Block(case_stmt_default(d),f,arg,os);
       os << indent() << "}\n";
       pop_attr_context(os);
       break;
     case KEYfor_stmt:
       push_attr_context(d);
       dump_Matches(for_stmt_matchers(d),false,f,arg,os);
       pop_attr_context(os);
       break;
     case KEYvalue_decl:
       if (!(Declaration_info(d)->decl_flags & LOCAL_ATTRIBUTE_FLAG) &&
	   depends_on(arg,d,b)) {
	 dump_local_decl(arg,d,os);
       }
       break;
     default:
       aps_error(d,"cannot handle this kind of statement");
       os << "0";
       break;
     });
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

void dump_Typed_decl(Type t, Declaration decl, char*prefix,ostream& o)
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
    dump_Use(type_use_use(t),"t_",o);
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

void dump_Default(Default d, ostream& o)
{
  switch (Default_KEY(d)) {
  case KEYsimple:
    dump_Expression(simple_value(d),o);
    break;
  case KEYcomposite:
    dump_Expression(composite_initial(d),o);
    break;
  default:
    /*? print nothing ?*/
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
      if (Declaration_info(d)->decl_flags & LOCAL_ATTRIBUTE_FLAG) {
	o << "a" << Declaration_info(d)->instance_index << "_"
	  << decl_name(d) << "->evaluate(anchor)";
      } else if (Declaration_info(d)->decl_flags & IMPLEMENTED_PATTERN_VAR) {
	o << *((string*)(Pattern_info((Pattern)tnode_parent(d))->pat_impl));
      } else {
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

void dump_Use(Use u, char *prefix, ostream& os)
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
