#include <stdio.h>
extern "C" {
#include "../aps/string.h"
}
#include "/usr/include/string.h"
#include <iostream>
#include <cctype>
#include <stack>
extern "C" {
#include "aps-ag.h"
String get_code_name(Symbol);
}
#include "dump-cpp.h"

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

#ifdef UNDEF
static void *activate_pragmas(void *ignore, void *node) {
  static Symbol code_symbol=NULL;
  if (code_symbol == NULL) {
    code_symbol=intern_symbol("code");
  }
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration decl=(Declaration)node;
    switch (Declaration_KEY(decl)) {
    case KEYpragma_call:
      { Symbol pragma_sym = pragma_call_name(decl);
        Expressions exprs = pragma_call_parameters(decl);
        Expression expr = first_Expression(exprs);
	if (pragma_sym == code_symbol) {
	  ...
	  for (; expr != NULL; expr=Expression_info(expr)->next_expr) {
	    switch (Expression_KEY(expr)) {
	    }}}}}}}
#endif

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
    dump_cpp_Declaration(decl_unit_decl(u),NULL,hs,cpps);
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

void dump_function_prototype(String prefix,char *name, Type ft, ostream&hs)
{
  Declarations formals = function_type_formals(ft);
  Declaration returndecl = first_Declaration(function_type_return_values(ft));
  if (returndecl == 0) {
    hs << "void ";
  } else {
    Type return_type = value_decl_type(returndecl);
    dump_Type(return_type,hs);
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
	  os << indent() << "Constructor* cons = node->cons;\n";
	}
	return;
      case KEYfor_stmt:
	{
	  Type ty = infer_expr_type(for_stmt_expr(decl));
	  os << indent() << "{ ";
	  dump_Type(ty,os);
	  os << " node = ";
	  dump_Expression(for_stmt_expr(decl),os);
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
      dump_TypeEnvironments(pfuse,os);
      if (USE_TYPE_ENV(pfuse)) os << "::";
      os << "V_" << cname << "* n = (";
      dump_TypeEnvironments(pfuse,os);
      if (USE_TYPE_ENV(pfuse)) os << "::";
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

void dump_Class(Class cl,ostream&os)
{
  switch (Class_KEY(cl)) {
  case KEYclass_use:
    dump_Use(class_use_use(cl),"C_",os);
    break;
  }
}

void dump_Signature(Type result,Signature sig,ostream&os)
{
  switch (Signature_KEY(sig)) {
  case KEYsig_inst:
    {
      dump_Class(sig_inst_class(sig),os);
      Declaration cdecl = USE_DECL(class_use_use(sig_inst_class(sig)));
      TypeActuals tactuals = sig_inst_actuals(sig);
      int started = false;
      if (Declaration_KEY(cdecl) == KEYclass_decl) {
	os << "< ";
	dump_Type(result,os);
	started = true;
      }
      FOR_SEQUENCE(Type, ty, TypeActuals, tactuals,
		   if (started) os << ","; else os << "< ";
		   started = true;
		   dump_Type(ty,os));
      if (started) os << " >";
    }
    break;
  case KEYsig_use:
    {
      Use u = sig_use_use(sig);
      Declaration sd = USE_DECL(u);
      switch (Declaration_KEY(sd)) {
      case KEYsignature_decl:
	dump_Signature(result,signature_decl_sig(sd),os);
	break;
      case KEYsignature_renaming:
	dump_Signature(result,signature_renaming_old(sd),os);
	break;
      default:
	break;
      }
    }
    break;
  case KEYmult_sig:
    dump_Signature(result,mult_sig_sig1(sig),os);
    // dump_Signature(mult_sig_sig2(sig),os);
    break;
  case KEYno_sig:
  case KEYfixed_sig:
    os << "Module";
    break;
  }
}

void dump_Type_signature(Type extension, Type ty, ostream& os)
{
  switch (Type_KEY(ty)) {
  case KEYprivate_type:
    dump_Type_signature(extension,private_type_rep(ty),os);
    break;
  case KEYremote_type:
    dump_Type_signature(extension,remote_type_nodetype(ty),os);
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
		   dump_Type(ty,os));
      if (started) os << " >";
    }
    break;
  case KEYtype_use:
    {
      Declaration decl = USE_DECL(type_use_use(ty));
      switch (Declaration_KEY(decl)) {
      case KEYsome_type_formal:
	dump_Signature(extension,some_type_formal_sig(decl),os);
	break;
      case KEYsome_type_decl:
	dump_Type_signature(extension,some_type_decl_type(decl),os);
	break;
      case KEYtype_renaming:
	dump_Type_signature(extension,type_renaming_old(decl),os);
	break;
      default:
	os << "Module";
	break;
      }
    }
    break;
  default:
    os << "Module";
    break;
  }
}

void dump_Signature_superclass(Type extension, Signature sig,ostream&os)
{
  switch (Signature_KEY(sig)) {
  case KEYsig_inst:
    {
      Declaration cdecl = USE_DECL(class_use_use(sig_inst_class(sig)));
      os << ", virtual public ";
      dump_Class(sig_inst_class(sig),os);
      TypeActuals tactuals = sig_inst_actuals(sig);
      bool started = false;
      if (Declaration_KEY(cdecl) == KEYclass_decl) {
	os << "< ";
	if (char *impl_type = Type_info(extension)->impl_type)
	  os << impl_type;
	else
	  dump_Type(extension,os);
	started = true;
      }
      FOR_SEQUENCE(Type, ty, TypeActuals, tactuals,
		   if (started) os << ","; else os << "< ";
		   started = true;
		   dump_Type(ty,os));
      if (started) os << " >";
    }
    break;
  case KEYsig_use:
    {
      Use u = sig_use_use(sig);
      Declaration sd = USE_DECL(u);
      switch (Declaration_KEY(sd)) {
      case KEYsignature_decl:
	dump_Signature_superclass(extension,signature_decl_sig(sd),os);
	break;
      case KEYsignature_renaming:
	dump_Signature_superclass(extension,signature_renaming_old(sd),os);
	break;
      default:
	break;
      }
    }
    break;
  case KEYmult_sig:
    dump_Signature_superclass(extension,mult_sig_sig1(sig),os);
    dump_Signature_superclass(extension,mult_sig_sig2(sig),os);
    break;
  case KEYno_sig:
  case KEYfixed_sig:
    break;
  }
}

void dump_Declaration_superclass(Type extension, Declaration decl, ostream& os);

void dump_Type_superclass(Type extension, Type ty, ostream& os)
{
  switch (Type_KEY(ty)) {
  case KEYprivate_type:
    dump_Type_superclass(extension,private_type_rep(ty),os);
    break;
  case KEYno_type:
    break;
  case KEYtype_inst:
    {
      os << ", virtual public ";
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
		   dump_Type(ty,os));
      if (started) os << " >";
    }
    break;
  case KEYtype_use:
    dump_Declaration_superclass(extension,USE_DECL(type_use_use(ty)),os);
    break;
  default:
    break;
  }
}

void dump_Declaration_superclass(Type extension, Declaration decl, ostream& os)
{
  switch (Declaration_KEY(decl)) {
  case KEYsome_type_formal:
    //mixins!
    //!!!! This doesn't work except for the primitive types
    //os << ", virtual public TypeTraits<T_" << decl_name(decl)
    //   << ">::ModuleType";
    dump_Signature_superclass(extension,some_type_formal_sig(decl),os);
    break;
  case KEYsome_type_decl:
    dump_Signature_superclass(extension,some_type_decl_sig(decl),os);
    dump_Type_superclass(extension,some_type_decl_type(decl),os);
    break;
  default:
    break;
  }
}


void dump_Signature_superinit(Type extension, Signature sig,ostream&os)
{
  switch (Signature_KEY(sig)) {
  case KEYsig_inst:
    {
      Declaration cdecl = USE_DECL(class_use_use(sig_inst_class(sig)));
      if (Declaration_KEY(cdecl) == KEYclass_decl)
	return;
      /* only modules need actual initialization */
      os << ", ";
      dump_Class(sig_inst_class(sig),os);
      TypeActuals tactuals = sig_inst_actuals(sig);
      bool started = false;
      if (Declaration_KEY(cdecl) == KEYclass_decl) {
	os << "< ";
	if (char *impl_type = Type_info(extension)->impl_type)
	  os << impl_type;
	else
	  dump_Type(extension,os);
	started = true;
      }
      FOR_SEQUENCE(Type, ty, TypeActuals, tactuals,
		   if (started) os << ","; else os << "< ";
		   started = true;
		   dump_Type(ty,os));
      if (started) os << " >";
      os << "(*_"; //! need to get formal before assigned!
      dump_Type_value(extension,os);
      os << ")";
    }
    break;
  case KEYsig_use:
    {
      Use u = sig_use_use(sig);
      Declaration sd = USE_DECL(u);
      switch (Declaration_KEY(sd)) {
      case KEYsignature_decl:
	dump_Signature_superinit(extension,signature_decl_sig(sd),os);
	break;
      case KEYsignature_renaming:
	dump_Signature_superinit(extension,signature_renaming_old(sd),os);
	break;
      default:
	break;
      }
    }
    break;
  case KEYmult_sig:
    dump_Signature_superinit(extension,mult_sig_sig1(sig),os);
    dump_Signature_superinit(extension,mult_sig_sig2(sig),os);
    break;
  case KEYno_sig:
  case KEYfixed_sig:
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
     dump_Type(tact,o));
  if (started) o << ">";
  o << "(";
  started = false;
  TypeContour tc =
  { 0, mdecl, tfs,
    (Declaration)tnode_parent(t) /*? OK if not a decl? */ };
  tc.u.type_actuals = tacts;
  Use u = use(intern_symbol("_fake"));
  USE_TYPE_ENV(u) = &tc;
  FOR_SEQUENCE
    (Type,tact,TypeActuals,tacts,
     if (started) o << ","; else started = true;
     o << "(";
     dump_Signature(tact,sig_subst(u,some_type_formal_sig(tf)),o);
     o << "*)(";
     dump_Type_value(tact,o);
     o << ")";
     tf = DECL_NEXT(tf));
  FOR_SEQUENCE
    (Expression, act, Actuals, type_inst_actuals(t),
     if (started) o << ",";
     else started = true;
     dump_Expression(act,o));
  o << ")";
}

void dump_Type_superinit(Type extension, Type ty, ostream& os)
{
  switch (Type_KEY(ty)) {
  case KEYprivate_type:
    dump_Type_superinit(extension,private_type_rep(ty),os);
    break;
  case KEYno_type:
    break;
  case KEYtype_inst:
    {
      os << ", ";
      dump_type_inst_construct(ty,os);
    }
    break;
  case KEYtype_use:
    dump_Declaration_superinit(extension,USE_DECL(type_use_use(ty)),os);
    break;
  default:
    break;
  }
}

void dump_Declaration_superinit(Type extension, Declaration decl, ostream& os)
{
  switch (Declaration_KEY(decl)) {
  case KEYsome_type_formal:
    //mixins!
    //!!!! This doesn't work except for the primitive types
    //os << ", virtual public TypeTraits<T_" << decl_name(decl)
    //   << ">::ModuleType";
    dump_Signature_superinit(extension,some_type_formal_sig(decl),os);
    break;
  case KEYsome_type_decl:
    dump_Signature_superinit(extension,some_type_decl_sig(decl),os);
    dump_Type_superinit(extension,some_type_decl_type(decl),os);
    break;
  default:
    break;
  }
}


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

void dump_cpp_Declaration(Declaration decl,Declaration context,
			  ostream&hs,ostream&cpps)
{
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
  switch (Declaration_KEY(decl)) {
  case KEYclass_decl:
    {
      Declaration rdecl = class_decl_result_type(decl);
      hs << "template <class T_" << decl_name(rdecl);
      for (Declaration tf=first_Declaration(class_decl_type_formals(decl));
	   tf ; tf = DECL_NEXT(tf)) {
	hs << ", class T_" << decl_name(tf);
      }
      hs << ">\n";
      hs << "class C_" << name << ": virtual public Module";
      dump_Signature_superclass(make_type_use(0,rdecl),class_decl_parent(decl),hs);
      hs << " {\n public:\n";
      for (Declaration d
	   = first_Declaration(block_body(class_decl_contents(decl)));
	   d != 0; d = DECL_NEXT(d)) {
	switch (Declaration_KEY(d)) {
	  //! for now only functions, so we do not need constructors:
	default:
          break;
	case KEYfunction_decl:
	  hs << "  virtual ";
	  {
	    Type ft = function_decl_type(d);
	    Declaration rdecl =
	      first_Declaration(function_type_return_values(ft));
	    dump_Typed_decl(value_decl_type(rdecl),d,"v_",hs);
	    hs << "(";
	    bool started = false;
	    for (Declaration f = first_Declaration(function_type_formals(ft));
		 f ; f = DECL_NEXT(f)) {
	      if (started) hs << ","; else started = true;
	      dump_formal(f,"v_",hs);
	    }
	    hs << ") = 0;\n";
	  }
	  break;
	}
      }
      hs << "};\n" << endl;
    }
    break;
  case KEYmodule_decl:
    {
      Declarations body = block_body(module_decl_contents(decl));
      Declaration rdecl = module_decl_result_type(decl);
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
	  hs << "class T_" << decl_name(tf);
	}
	hs << ">\n";
      }
      hs << "class C_" << name << " : virtual public Module";
      Type rut = some_type_decl_type(rdecl);
      if (impl_type) {
	Type_info(rut)->impl_type = impl_type;
      }
      if (!impl_type) {
	if (Type_KEY(some_type_decl_type(rdecl)) == KEYno_type) {
	  if (Declaration_KEY(rdecl) == KEYphylum_decl) {
	    hs << ", virtual public C_PHYLUM";
	  } else {
	    hs << ", virtual public C_TYPE";
	  }
	} 
	dump_Type_superclass(rut,some_type_decl_type(rdecl),hs);
      }
      dump_Signature_superclass(rut,module_decl_parent(decl),hs);
      hs << " {\n";
      for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	   tf ; tf = DECL_NEXT(tf)) {
	hs << "  ";
	dump_Signature(make_type_use(0,tf),some_type_formal_sig(tf),hs);
	hs << " *t_" << decl_name(tf) << ";\n";
      }
      for (Declaration vf=first_Declaration(module_decl_value_formals(decl));
	   vf ; vf = DECL_NEXT(vf)) {
	hs << "  ";
	dump_formal(vf,"v_",hs);
	hs << ";\n";
      }

      if (impl_type) {
	hs << "  typedef " << impl_type << " T_" << decl_name(rdecl) << ";\n";
	hs << "  C_" << name << " *t_" << decl_name(rdecl) << ";\n";
      }
      for (Declaration d = first_decl; d ; d = DECL_NEXT(d)) {
	dump_cpp_Declaration(d,decl,hs,bs);
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

      first_decl = first_Declaration(body); // result is special

      // The normal default constructor header:
      hs << "\n public:\n";
      hs << "  C_" << name << "(";
      {
	bool started = false;
	for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	     tf ; tf = DECL_NEXT(tf)) {
	  if (started) hs << ","; else started = true;
	  dump_Signature(make_type_use(0,tf),some_type_formal_sig(tf),hs);
	  hs << "* _t_" << decl_name(tf);
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
	  dump_Signature(make_type_use(0,tf),some_type_formal_sig(tf),cpps);
	  cpps << "* _t_" << decl_name(tf);
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
	bs << " :\n  Module(\"" << name << "\")";
	if (!impl_type)
	  dump_Type_superinit(rut,some_type_decl_type(rdecl),bs);
	for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	     tf ; tf = DECL_NEXT(tf)) {
	  bs << ",\n  t_" << decl_name(tf) << "(_t_" << decl_name(tf) << ")";
	}
	for (Declaration vf=first_Declaration(module_decl_value_formals(decl));
	     vf ; vf = DECL_NEXT(vf)) {
	  bs << ",\n  v_" << decl_name(vf) << "(_v_" << decl_name(vf) << ")";
	}
	{
	  char *n = decl_name(rdecl);
	  bs << ",\n  t_" << n << "(this)";
	}
	for (Declaration d = first_decl; d; d = DECL_NEXT(d)) {
	  switch(Declaration_KEY(d)) {
	  case KEYsome_type_decl:
	    bs << ",\n";
	    bs << "  t_" << decl_name(d) << "(";
	    dump_Type_value(some_type_decl_type(d),bs);
	    bs << ")";
	    break;
	  case KEYtype_renaming:
	    bs << ",\n";
	    bs << "  t_" << decl_name(d) << "(";
	    dump_Type_value(type_renaming_old(d),bs);
	    bs << ")";
	    break;	    
	  case KEYvalue_decl:
	    bs << ",\n";
	    bs << "  v_" << decl_name(d) << "(";
	    dump_Default(value_decl_default(d),bs);
	    bs << ")";
	    break;
	  case KEYvalue_renaming:
	    if (Type_KEY(infer_expr_type(value_renaming_old(d)))
		== KEYfunction_type)
	      break;
	    bs << ",\n";
	    bs << "  v_" << decl_name(d) << "(";
	    dump_Expression(value_renaming_old(d),bs);
	    bs << ")";
	    break;	    
	  case KEYattribute_decl:
	    bs << ",\n";
	    bs << "  a_" << decl_name(d) << "(new A_" << decl_name(d) << "(";
	    bs << "this";
	    bs << "))";
	    break;
	  case KEYconstructor_decl:
	    bs << ",\n";
	    bs << "  c_" << decl_name(d) << "(new C_" << decl_name(d) << "(";
	    {
	      Type cty = constructor_decl_type(d);
	      Declarations rdecls = function_type_return_values(cty);
	      Type rt = value_decl_type(first_Declaration(rdecls));
	      dump_Type_value(rt,bs);
	      bs << ",this))";
	    }
	    break;
	  default:
	    break;
	  }
	}
	bs << " {}\n\n";
      }

      // now the copy constructor
      hs << "  C_" << name << "(const C_" << name << "& from)";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << "\n" << prefix << "C_" << name << "::C_" << name << "("
	     << "const C_" << name << "& from)";	
      }
      {
	bool at_start = true;
	for (Declaration tf=first_Declaration(module_decl_type_formals(decl));
	     tf ; tf = DECL_NEXT(tf)) {
	  if (!at_start) bs << ","; else { bs << " :"; at_start = false; }
	  bs << "\n  t_" << decl_name(tf) << "(from.t_" << decl_name(tf) << ")";
	}
	for (Declaration vf=first_Declaration(module_decl_value_formals(decl));
	     vf ; vf = DECL_NEXT(vf)) {
	  if (!at_start) bs << ","; else { bs << " :"; at_start = false; }
	  bs << "\n  v_" << decl_name(vf) << "(from.v_" << decl_name(vf) << ")";
	}
	{
	  char *n = decl_name(rdecl);
	  if (at_start) bs << " :\n  "; else bs << ",\n  ";
	  at_start = false;
	  bs << "t_" << n << "(from.t_" << n << ")";
	}
	for (Declaration d = first_decl; d; d = DECL_NEXT(d)) {
	  char* kind = NULL;
	  switch(Declaration_KEY(d)) {
	  case KEYsome_type_decl:
	  case KEYtype_renaming:
	    kind = "t_";
	    break;
	  case KEYvalue_renaming:
	    if (Type_KEY(infer_expr_type(value_renaming_old(d)))
		== KEYfunction_type)
	      break;
	    /* FALL THROUGH */
	  case KEYvalue_decl:
	    kind = "v_";
	    break;
	  case KEYattribute_decl:
	    kind = "a_";
	    break;
	  case KEYconstructor_decl:
	    kind = "c_";
	    break;
	  default:
	    break;
	  }
	  if (kind != NULL) {
	    char *n = decl_name(d);
	    if (at_start) bs << " :\n  ";
	    else bs << ",\n  ";
	    at_start = false;
	    bs << kind << n << "(from." << kind << n << ")";
	  }
	}
	bs << " {}\n\n";
      }

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
      Declarations body = context ? block_body(module_decl_contents(context)) : 0;
      if (context) hs << " public:\n";
      Type type = some_type_decl_type(decl);
      bool first = true;
      switch (Type_KEY(type)) {
      case KEYno_type:
	if (!context) break;
	for (Declaration d=first_Declaration(body); d; d=DECL_NEXT(d)) {
	  if (KEYconstructor_decl == Declaration_KEY(d) &&
	      constructor_decl_base_type_decl(d) == decl) {
	    if (first) {
	      hs << "  enum P_" << name << "{";
	      first = false;
	    } else hs << ",";
	    hs << " p_" << decl_name(d);
	  }
	}
	if (!first) hs << " };\n";
	if (first) {
	  // no type declared
	  hs << "  typedef void T_" << name << ";\n";
	  if (context && Declaration_KEY(context) == KEYmodule_decl &&
	      module_decl_result_type(context) == decl) {
	    hs << "  C_" << decl_name(context) << "* t_" << name << ";\n";
	  } else {
	    hs << "  C_" << (is_phylum ? "PHYLUM" : "TYPE") << "* t_" << name << ";\n";
	  }
	  break;
	}
	hs << "  class C_" << name << " : public Node {\n   public:\n";
	hs << "    C_" << name << "(Constructor*c)";
	if (!inline_definitions) {
	  hs << ";\n";
	  cpps << prefix << "C_" << name << "::C_" << name
	       << "(Constructor*c)";
	}
	cpps << " : Node(c) {};\n" << endl;
	  
	hs << "  };\n";
	hs << "  typedef Node* T_" << name << ";\n";
	//? We can't use C_name as the type of t_name
	//? because of the signature.
	hs << "  " << (is_phylum ? "C_PHYLUM" : "C_TYPE")
	   << "* t_" << name << ";\n";
	hs << endl;

	break;
      default:
	hs << "  typedef "; dump_Type(type,hs); hs << " T_" << name << ";\n";
	if (context && Declaration_KEY(context) == KEYmodule_decl &&
	    module_decl_result_type(context) == decl) {
	  hs << "  C_" << decl_name(context) << "* t_" << name << ";\n";
	} else {
	  Type fake_use = make_type_use(0,decl);
	  if (!context) hs << "extern "; else hs << "  ";
	  dump_Type_signature(fake_use,type,hs);
	  if (!context) {
	    dump_Type_signature(fake_use,type,cpps);
	  }
	  hs << "* t_" << name << ";\n";
	  if (!context) {
	    cpps << "* t_" << name << " = ";
	    dump_Type_value(type,cpps);
	    cpps << ";\n";
	  }
	}
      }
    }
    break;
  case KEYconstructor_decl:
    {
      Type ft = constructor_decl_type(decl);
      Declarations formals = function_type_formals(ft);
      Declaration tdecl = constructor_decl_base_type_decl(decl);
      bool is_syntax = false;
      char *base_type_name = decl_name(tdecl);
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
      /* header file */
      hs << " private:\n";
      hs << "  class C_" << name << " : public Constructor {\n";
      hs << "  public:\n";
      hs << "    C_" << decl_name(context) << "* context;\n";
      hs << "    C_" << name << (is_syntax ? "(Phylum*t,C_":"(Type*t,C_") ;
      hs << decl_name(context) << "*c)";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << prefix << "C_" << name << "::C_" << name
	     << (is_syntax ? "(Phylum *t,C_" : "(Type *t,C_");
	cpps << decl_name(context) << "*c)\n";
      }
      cpps << "  : Constructor(t,\""<< name << "\",p_" << name
	   << "), context(c) {}\n";
      hs << "  };\n";

      hs << " public:\n";
      hs << "  struct V_" << name << " : public C_" << base_type_name 
         << " {" << endl;
      for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	hs << "    "; dump_formal(f,"v_",hs); hs << ";\n";
      }
      hs << "    V_" << name << "(C_" << name << "*c";
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
	  << "(C_" << name << "* c";
	for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	  cpps << ", ";
	  dump_Type(formal_type(f),cpps);
	  cpps << " _" << decl_name(f);
	}
	cpps << ")";
      }
      cpps << "\n  : C_" << base_type_name << "(c)";
      for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	cpps << ", v_" << decl_name(f) << "(_" << decl_name(f) << ")";
      }
      cpps << "{";
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
	      cpps << "\n";
	      if (inline_definitions) hs << "    ";
	      cpps << "  _" << decl_name(f) << "->set_parent(this); ";
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
      cpps << "}\n";

      // a print function (defer to constructor context)
      hs << "    string to_string()";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << "\nstring " << prefix << "V_" << name << "::to_string()";
      }
      cpps << " {\n";
      if (inline_definitions) hs << "    ";
      cpps << "  return ((C_" << name << "*)cons)->context->s_" << name << "(this);\n";
      if (inline_definitions) hs << "    ";
      cpps << "}\n";
      hs << "  };\n\n";

      hs << "  C_" << name << "* c_" << name << ";\n  ";
      dump_function_prototype(empty_string,name,ft,hs);
      if (!inline_definitions) {
	hs << ";\n";
	dump_function_prototype(prefix,name,constructor_decl_type(decl),cpps);
      }
      cpps << " {\n";
      if (inline_definitions) hs << "  ";
      cpps << "  return new V_" << name << "(c_" << name;
      for (Declaration f = first_Declaration(function_type_formals(constructor_decl_type(decl)));
	   f != NULL; f = DECL_NEXT(f)) {
	cpps << ",v_" << decl_name(f);
      }
      cpps << ");\n";
      if (inline_definitions) hs << "  ";
      cpps << "}\n";

      // the print function in the context:
      hs << "  string s_" << name << "(V_" << name << "* node)";
      if (!inline_definitions) {
	hs << ";\n" << endl;
	cpps << "\nstring " << prefix
	     << "s_" << name << "(V_" << name << "* node)";
      }
      cpps << " {\n";
      if (inline_definitions) hs << "  ";
      if (is_syntax) {
        cpps << "  return string(\"" << name
	     << "#\")+t_Integer->v_string(node->index)+string(\"(\")";
      } else {
	cpps << "  return string(\"" << name << "(\")";
      }
      bool started = false;
      for (Declaration f = first_Declaration(formals); f; f = DECL_NEXT(f)) {
	if (started) {
	  cpps << "+string(\",\")";
	} else started = true;
	cpps << "\n      ";
	if (inline_definitions) hs << "  ";
	cpps << "+";
	dump_Type_value(formal_type(f),cpps);
	cpps << "->v_string(node->v_" << decl_name(f) << ")";
      }
      cpps << "+string(\")\");\n";
      if (inline_definitions) hs << "  ";
      cpps << "}\n" << endl;
    }
    break;
  case KEYvalue_decl:
    {
      if (context) 
	hs << " public:\n  ";
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
	default:
	  // must be done in handcode version.
	  break;
	}
      }
      // Otherwise initialization is done in the module
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
      bool inh = (ATTR_DECL_IS_INH(decl) != 0);
      
      hs << " private:\n";
      hs << "  class A_" << name << " : public Attribute<";
      dump_Type(at,hs);
      hs << ",";
      dump_Type(rt,hs);
      hs << "> {\n";
      hs << "    C_" << decl_name(context) << "* context;\n";
      hs << "  public:\n";
      hs << "    A_" << name << "(C_" << decl_name(context) << "*c)";
      if (!inline_definitions) {
	hs << ";\n";
	cpps << "\n" << prefix << "A_" << name << "::A_" << name
	     << "(C_" << decl_name(context) << "*c)";
      }
      cpps << " : Attribute<";
      dump_Type(at,cpps);
      cpps << ",";
      dump_Type(rt,cpps);
      cpps << ">(c->"; dump_Type_value(at,cpps); cpps << ",";
      if (!is_global(rt)) cpps << "c->";
      dump_Type_value(rt,cpps);
      cpps << ",\"" << name << "\"), context(c) {}\n";

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
      cpps << "{ return context->c_" << name << "(node); }\n";
      hs << "};\n public:\n";

      hs << "  A_" << name << " *a_" << name << ";\n";

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
      cpps << " {\n";
      if (inh) {
	cpps << "  Node* node=anode->parent;\n";
	cpps << "  if (node != 0) {\n";
      } else {
	cpps << "  Node* node = anode;\n";
      }
      cpps << "  Constructor* cons = node->cons;\n";
      for (Declaration d=first_Declaration(block_body(module_decl_contents(context)));
	   d != NULL; d = DECL_NEXT(d)) {
	switch (Declaration_KEY(d)) {
	case KEYtop_level_match:
	  {
	    Match m = top_level_match_m(d);
	    push_attr_context(m);
	    Block body = matcher_body(top_level_match_m(d));
	    dump_Block(body,dump_attr_assign,decl,cpps);
	    pop_attr_context(cpps);
	  }
	  break;
	default:
	  break;
	}
      }
      if (inh) {
	cpps << "  }\n";
      }
      if (direction_is_collection(attribute_decl_direction(decl))) {
	cpps << "  return collection;\n";
      } else switch (Default_KEY(attribute_decl_default(decl))) {
      default:
	cpps << "  throw UndefinedAttributeException(string(\""
	     << name << " of \")+anode);\n";
	break;
      case KEYsimple:
	cpps << "  return ";
	dump_Expression(simple_value(attribute_decl_default(decl)),cpps);
	cpps << ";\n";
	break;
      }
      cpps << "}\n";


      hs << "  ";
      dump_function_prototype(empty_string,name,attribute_decl_type(decl),hs);
      if (!inline_definitions) {
	hs << ";\n" << endl; 
	dump_function_prototype(prefix,name,attribute_decl_type(decl),cpps);
      }
      cpps << " { return a_" << name << "->evaluate(v_";
      cpps << decl_name(af) << "); }\n" << endl; 
    }
    break;
  case KEYfunction_decl:
    {
      Type fty = function_decl_type(decl);
      Declaration rdecl = first_Declaration(function_type_return_values(fty));
      Block b = function_decl_body(decl);
      if (context) hs << "public:\n  ";
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
	cout << name << " has no body.\n";
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
	hs << "class T_" << decl_name(f);
      }
      hs << ">\nstruct C_" << name << "{\n";
      for (Declaration f=first_Declaration(tfs); f; f=DECL_NEXT(f)) {
	hs << "  ";
	dump_Signature(make_type_use(0,f),some_type_formal_sig(f),hs);
	hs << " *t_" << decl_name(f) << ";\n";
      }
      for (Declaration d=first_Declaration(body); d; d=DECL_NEXT(d)) {
	dump_cpp_Declaration(d,decl,hs,hs);
      }
      hs << "  C_" << name << "(";
      started = false;
      for (Declaration f=first_Declaration(tfs); f; f=DECL_NEXT(f)) {
	if (started) hs << ","; else started = true;
	dump_Signature(make_type_use(0,f),some_type_formal_sig(f),hs);
	hs << " *_" << decl_name(f);
      }
      hs << ") : ";
      started = false;
      for (Declaration f=first_Declaration(tfs); f; f=DECL_NEXT(f)) {
	if (started) hs << ","; else started = true;
	hs << "t_" << decl_name(f) << "(_" << decl_name(f) << ")";
      }
      for (Declaration d=first_Declaration(body); d; d=DECL_NEXT(d)) {
	switch (Declaration_KEY(d)) {
	case KEYvalue_renaming:
	  if (Type_KEY(infer_expr_type(value_renaming_old(d)))
	      == KEYfunction_type)
	    break;
	  hs << ", v_" << def_name(declaration_def(d)) << "(";
	  dump_Expression(value_renaming_old(d),hs);
	  hs << ")";
	  break;
	case KEYfunction_decl:
	case KEYpolymorphic:
	  break;
	case KEYpragma_call:
	  break;
	default:
	  aps_error(d,"not generating code for %s",decl_name(d));
	}
      }
      hs << " {}\n};\n\n";
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
	if (context) hs << " public:\n  ";
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
	if (context) hs << " public:\n  ";
	dump_Type(ty,hs);
	hs << " v_" << name;
	if (context) {
	  // initialization done by module:
	  hs << ";\n" << endl;
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
    /*! hack */
    hs << "#define T_" << name << " ";
    dump_Type(type_renaming_old(decl),hs);
    hs << "\n#define t_" << name << " ";
    dump_Type_value(type_renaming_old(decl),hs);
    hs << "\n" << endl;
    break;
  case KEYpragma_call:
    break;
  default:
    cout << "Not handling declaration " << decl_name(decl) << endl;
  }
}

void dump_Matches(Matches ms, ASSIGNFUNC f, void*arg, ostream&os)
{
  FOR_SEQUENCE
    (Match,m,Matches,ms,
     push_attr_context(m);
     dump_Block(matcher_body(m),f,arg,os);
     pop_attr_context(os));
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
       dump_Matches(case_stmt_matchers(d),f,arg,os);
       dump_Block(case_stmt_default(d),f,arg,os);
       pop_attr_context(os);
       break;
     case KEYfor_stmt:
       push_attr_context(d);
       dump_Matches(for_stmt_matchers(d),f,arg,os);
       pop_attr_context(os);
       break;
     case KEYvalue_decl:
       if (depends_on(arg,d,b)) {
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

void dump_Type(Type t, ostream& o)
{
  switch (Type_KEY(t)) {
  case KEYtype_use:
    {
      Use u = type_use_use(t);
      Declaration tdecl = USE_DECL(u);
      switch (Declaration_KEY(tdecl)) {
      default:
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
      dump_Module(type_inst_module(t),o);
      Declaration mdecl = USE_DECL(module_use_use(type_inst_module(t)));
      TypeActuals tacts = type_inst_type_actuals(t);
      bool started = false;
      FOR_SEQUENCE
	(Type,tact,TypeActuals,tacts,
	 if (started) o << ",";
	 else { o << "<"; started = true; }
	 dump_Type(tact,o));
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
    o << "Node*"; //? not sure
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
      Declaration decl = (Declaration)tnode_parent(t);
      if (ABSTRACT_APS_tnode_phylum(decl) == KEYDeclaration) {
	switch (Declaration_KEY(decl)) {
	case KEYtype_decl:
	  o << "new C_TYPE()";
	  break;
	case KEYphylum_decl:
	  o << "new C_PHYLUM()";
	  break;
	default:
	  o << "new C_TYPE()";
	}
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
    dump_Use(value_use_use(e),"v_",o);
    break;
  case KEYtyped_value:
    dump_Expression(typed_value_expr(e),o);
    break;
  default:
    aps_error(e,"cannot handle this kind of expression");
  }
}

static void dump_poly_inst(Use u, TypeEnvironment type_env, ostream& os)
{
  if (type_env == 0) return;
  if (!type_env->source || Declaration_KEY(type_env->source) != KEYpolymorphic) return;
  dump_poly_inst(u,type_env->outer,os);
  os << "C_";
  os << def_name(polymorphic_def(type_env->source));
  os << "<";
  int n=0;
  for (Declaration f=first_Declaration(type_env->type_formals);
       f; f=DECL_NEXT(f))
    ++n;
  bool started = false;
  for (int i=0; i < n; ++i) {
    Type t = type_env->u.inferred[i];
    if (t == 0) {
      aps_error(u,"incomplete unification");
      os << "UNKNOWN";
      continue;
    }
    if (started) os << ","; else started = true;
    dump_Type(t,os);
  }
  os << ">(";
  started = false;
  Declaration tf = first_Declaration(type_env->type_formals);
  for (int i=0; i < n; ++i) {
    Type t = type_env->u.inferred[i];
    if (t == 0) continue;
    if (started) os << ","; else started = true;
    os << "(";
    dump_Signature(t,sig_subst(u,some_type_formal_sig(tf)),os);
    os << "*)(";
    dump_Type_value(t,os);
    os << ")";
    tf = DECL_NEXT(tf);
  }
  os << ").";
}

void dump_TypeEnvironments(Use u, ostream&os)
{
  TypeEnvironment te = USE_TYPE_ENV(u);
  while (te && Declaration_KEY(te->source) == KEYpolymorphic)
    te = te->outer;
  if (!te) return;
  os << "C_" << decl_name(te->source);
  bool started = false;
  if (Declaration_KEY(te->source) == KEYclass_decl) {
    os << "< ";
    dump_Type(make_type_use(type_envs_nested(u),te->result),os);
    started = true;
  }
  for (Type ta = first_TypeActual(te->u.type_actuals);
       ta ; ta = TYPE_NEXT(ta)) {
    if (started) os << ","; else { os << "< "; started = true;}
    dump_Type(ta,os);
  }
  if (started) os << " >";
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
      os << "((";
      dump_TypeEnvironments(u,os);
      os << "*)";
      dump_Type_value(qual_use_from(u),os);
      os << ")";
      os << "->";
      sym = qual_use_name(u);
      break;
    }
  }
  dump_poly_inst(u,USE_TYPE_ENV(u),os);
  os << prefix << sym;
}
