#include <iostream>
#include <sstream>
#include <stack>
#include <vector>
extern "C" {
#include <stdio.h>
#include "aps-ag.h"
}
#include "dump-scala.h"
#include "implement.h"

// Dynamic implementation of attribute grammars in APS syntax
// Many many known bugs.
// In particular:
// case is not implemented exclusively:
//    z : Integer := 0;
//    case foo() of
//      match foo() begin
//        y := 2;
//      end;
//    else
//      z := 4;
//    end;
// It will say z is 4, when it should be zero.

using namespace std;

struct ContextRecordNode {
  Declaration context;
  void *extra; /* branch in context (if any) */
  ContextRecordNode* outer;
  ContextRecordNode(Declaration d, void* e, ContextRecordNode* o)
    : context(d), extra(e), outer(o) {}
};

typedef void (*ASSIGNFUNC)(void *,Declaration,ostream&);
void dump_Block(Block,ASSIGNFUNC,void*arg,ostream&);

static const int MAXDEPTH = 200;

static void *attr_context[MAXDEPTH];
static int attr_context_depth = 0;   /* current depth of attribute assigns */
static int attr_context_started = 0; /* depth of last activation */

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
	os << indent() << "{ val cond = ";
	++nesting_level;
	dump_Expression(if_stmt_cond(decl),os);
	os << ";\n";
	return;
      case KEYcase_stmt:
	{
	  os << indent() << case_stmt_expr(decl) << " match {\n";
	  ++nesting_level;
	}
	return;
      case KEYfor_stmt:
	{
	  //!! Doesn't work.
	  Type ty = infer_expr_type(for_stmt_expr(decl));
	  os << indent() << "{ ";
	  ++nesting_level;
	  dump_Type(ty,os);
	  os << " node = ";
	  dump_Expression(for_stmt_expr(decl),os);
	  os << ";\n";
	  os << indent() << "Constructor* cons = node->cons;\n";
	}
	return;
      case KEYfor_in_stmt:
	{
	  aps_error(decl,"Still generating C++ here...");
	  Declaration f = for_in_stmt_formal(decl);
	  Type ty = infer_formal_type(f);
	  os << indent() << "for (CollectionIterator<";
	  dump_Type(ty,os);
	  os << "> ci = CollectionIterator<";
	  dump_Type(ty,os);
	  os << ">(";
	  dump_Expression(for_in_stmt_seq(decl),os);
	  os << "); ci.has_item(); ci.advance()) {";
	  ++nesting_level;
	  os << indent() << ty
	     << " v_" << decl_name(f) << " = ci.item();\n";
	}
	return;
      case KEYtop_level_match:
	{
	  Type ty = infer_pattern_type(matcher_pat(top_level_match_m(decl)));
	  static int unique = 0;
	  int i = ++unique;
	  os << indent() << "val phy"<< i << " = " << as_val(ty)
	     << ".asInstanceOf[I_PHYLUM[" << ty << "]];\n";
	  os << indent() << "for (i <- 0 until phy" << i << ".size) {\n";
	  ++nesting_level;
	  os << indent() << ty << " anchor = phy" << i << ".get(i);\n";
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
      Declaration parent = (Declaration)tnode_parent(b);
      switch (Declaration_KEY(parent)) {
      case KEYif_stmt:
	if (b == if_stmt_if_true(parent)) {
	  os << indent() << "if (cond) {\n";
	} else {
	  os << indent() << "if (!cond) {\n";
	}
	++nesting_level;
	break;
      case KEYcase_stmt:
	os << indent() << "case _ => {\n";
	++nesting_level;
	break;
      default:
	fatal_error("%d: cannot determine why we have this attr_context",
		    tnode_line_number(b));
	break;
      }
      return;
    }
  case KEYMatch:
    {
      Match m = (Match)c;
      Pattern p = matcher_pat(m);
      // Declaration header = Match_info(m)->header;
      // bool is_exclusive = Declaration_KEY(header) == KEYcase_stmt;
      os << indent() << "case " << p << " => {\n";
      ++nesting_level;
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
    --nesting_level;
    os << indent() << "}\n";
  }
}

static void dump_init_collection(Declaration vd, ostream& os)
{
  os << indent() << "var collection : " << value_decl_type(vd) << " = ";
  dump_vd_Default(vd,os);
  os << ";\n";
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
    if (Declaration_info(decl)->decl_flags & VAR_VALUE_DECL_FLAG &&
	!direction_is_collection(dir)) {
      os << indent() << "s_" << decl_name(decl) << " = EVALUATED;\n";
    }
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
  o << indent() << "var v_" << decl_name(local)
    << " : " << value_decl_type(local);
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


void dump_Matches(Matches ms, bool exclusive, ASSIGNFUNC f, void*arg, ostream&os)
{
  FOR_SEQUENCE
    (Match,m,Matches,ms,
     push_attr_context(m);
     dump_Block(matcher_body(m),f,arg,os);
     pop_attr_context(os);
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
       push_attr_context(case_stmt_default(d));
       dump_Block(case_stmt_default(d),f,arg,os);
       pop_attr_context(os);
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

void dump_default_return(Default deft, Direction dir, string ns, ostream& os)
{
  if (direction_is_collection(dir)) {
    os << indent() << "return collection;\n";
  } else switch (Default_KEY(deft)) {
  default:
    os << indent() << "throw UndefinedAttributeException(" << ns << ");\n";
    break;
  case KEYsimple:
    os << indent() << "return " << simple_value(deft) << ";\n";
    break;
  }
}

Block local_attribute_block(Declaration d)
{
  void *p = d;
  while ((p = tnode_parent(p)) != 0 &&
	 ABSTRACT_APS_tnode_phylum(p) != KEYBlock)
    ;
  return (Block)p;
}

static void dump_local_context_begin(void *p, Declaration d, ostream& os)
{
  if (p == 0) aps_error(d,"internal error: reached null parent!");
  else switch (ABSTRACT_APS_tnode_phylum(p)) {
    case KEYDeclaration:
      if (Declaration_KEY((Declaration)p) == KEYtop_level_match) return;
      /* FALL THROUGH */
    default:
      dump_local_context_begin(tnode_parent(p),d,os);
      break;
    case KEYMatch: 
      dump_local_context_begin(tnode_parent(p),d,os);
      {
	Match m = (Match)p;
	Declaration header = Match_info(m)->header;
	switch (Declaration_KEY(header)) {
	case KEYtop_level_match:
	  os << indent() << "anchor";
	  break;
	case KEYcase_stmt:
	  os << indent() << case_stmt_expr(header);
	  break;
	default:
	  aps_error(header,"Cannot handle this header for Match");
	  break;
	}
	os << " match {\n";
	++nesting_level;
	os << indent() << "case " << matcher_pat(m) << " => {\n";
	++nesting_level;
      }
      break;
    }
}

static void dump_local_context_end(void *p, Declaration d, ostream& os)
{
  if (p == 0) aps_error(d,"internal error: reached null parent!");
  else switch (ABSTRACT_APS_tnode_phylum(p)) {
    case KEYDeclaration:
      if (Declaration_KEY((Declaration)p) == KEYtop_level_match) return;
      /* FALL THROUGH */
    default:
      dump_local_context_end(tnode_parent(p),d,os);
      break;
    case KEYMatch: 
      --nesting_level;
      os << indent() << "}\n";
      --nesting_level;
      os << indent() << "}\n";
      dump_local_context_end(tnode_parent(p),d,os);
      break;
    }
}

void implement_local_attributes(vector<Declaration>& local_attributes,
				ostream& oss)
{
  int n = local_attributes.size();
  for (int j=0; j <n; ++j) {
    Declaration d = local_attributes[j];
    Block b = local_attribute_block(d);
    int i = LOCAL_UNIQUE_PREFIX(d);
    const char *name = decl_name(d);
    bool is_col = direction_is_collection(value_decl_direction(d));
    
    oss << indent() << "def c" << i << "_" << name << "(anchor : Any) : "
	<< value_decl_type(d) << " = {\n";
    ++nesting_level;
    dump_local_context_begin(d,d,oss);
    if (is_col) {
      dump_init_collection(d,oss);
    }
    dump_Block(b,dump_attr_assign,d,oss);
    dump_default_return(value_decl_default(d),value_decl_direction(d),
			string("\"local ")+name+"\"",oss);
    dump_local_context_end(d,d,oss);
    --nesting_level;
    oss << indent() << "}\n";
  }
}

void implement_attributes(const vector<Declaration>& attrs,
			  const vector<Declaration>& tlms,
			  ostream& oss)
{
  int n = attrs.size();
  for (int i=0; i <n; ++i) {
    Declaration ad = attrs[i];
    const char *name = decl_name(ad);
    Declarations afs = function_type_formals(attribute_decl_type(ad));
    Declaration af = first_Declaration(afs);
    Type at = formal_type(af);
    // Declaration pd = USE_DECL(type_use_use(at));
    Declarations rdecls = function_type_return_values(attribute_decl_type(ad));
    Declaration rdecl = first_Declaration(rdecls);
    Type rt = value_decl_type(rdecl);
    bool inh = (ATTR_DECL_IS_INH(ad) != 0);
    bool is_col = direction_is_collection(attribute_decl_direction(ad));

    oss << indent() << "def c_" << name << "(anode : " << at << ") : "
	<< rt << " = {\n";
    ++nesting_level;
    if (is_col) {
      dump_init_collection(rdecl,oss);
    }
    if (inh) {
      oss << indent() << "val anchor = anode.asInstanceOf[Phylum].parent;\n";
    } else {
      oss << indent() << "val anchor = anode;\n";
    }
    oss << indent() << "anchor match {\n";
    ++nesting_level;
    for (vector<Declaration>::const_iterator i = tlms.begin();
	 i != tlms.end(); ++i) {
      if (is_col) push_attr_context(*i);
      Match m = top_level_match_m(*i);
      push_attr_context(m);
      Block body = matcher_body(m);
      dump_Block(body,dump_attr_assign,ad,oss);
      pop_attr_context(oss);
      if (is_col) pop_attr_context(oss);
    }
    oss << indent() << "case _ => {};\n";
    --nesting_level;
    oss << indent() << "}\n";
    dump_default_return(attribute_decl_default(ad),
			attribute_decl_direction(ad),
			string("anode.toString()+\".") + name + "\"", oss);
    --nesting_level;
    oss << indent() << "}\n";
  }
}

void implement_var_value_decls(const vector<Declaration>& vvds,
			       const vector<Declaration>& tlms,
			       ostream& oss)
{
  int n = vvds.size();
  for (int i=0; i <n; ++i) {
    Declaration vvd = vvds[i];
    const char *name = decl_name(vvd);
    Type vt = value_decl_type(vvd);
    bool is_col = direction_is_collection(value_decl_direction(vvd));

    oss << indent() << "def c_" << name << "() : " << vt << " = {\n";
    ++nesting_level;
    oss << indent() << "if (s_" << name << " >= EVALUATED) "
	<< "return v_" << name << ";\n";
    oss << indent() << "if (s_" << name << " == CYCLE) "
	<< "throw CyclicAttributeException(\"" << name << "\");\n";
    if (debug) {
      oss << indent() << "Debug.start(\"" << name << "\");\n";
      oss << indent() << "try {\n";
      ++nesting_level;
    }
    oss << indent() << "s_" << name << " = CYCLE;\n";
    if (is_col) {
      dump_init_collection(vvd,oss);
    }
    for (vector<Declaration>::const_iterator i = tlms.begin();
	 i != tlms.end(); ++i) {
      push_attr_context(*i);
      Match m = top_level_match_m(*i);
      push_attr_context(m);
      Block body = matcher_body(m);
      dump_Block(body,dump_attr_assign,vvd,oss);
      pop_attr_context(oss);
      pop_attr_context(oss);
    }
    if (is_col) {
      if (debug) {
	oss << indent() << "Debug.returns(collection.toString());\n";
      }
      oss << indent() << "v_" << name << " = collection;\n";
    }
    oss << indent() << "s_" << name << " = EVALUATED;\n";
    dump_default_return(value_decl_default(vvd),
			value_decl_direction(vvd),
			string("\"") + name + "\"", oss);
    if (debug) {
      dump_debug_end(oss);
    }
    --nesting_level;
    oss << indent() << "}\n";
  }
}

class Dynamic : public Implementation
{
public:
  typedef Implementation::ModuleInfo Super;
  class ModuleInfo : public Super {
  public:
    ModuleInfo(Declaration mdecl) : Implementation::ModuleInfo(mdecl) {}

    void note_top_level_match(Declaration tlm, ostream& oss) {
      Super::note_top_level_match(tlm,oss);
    }

    void dump_compute(string cfname, ostream& oss) {
      oss << indent() << "override def compute(node : NodeType) : ValueType = "
	  << cfname << "(node);\n";
    }
    
    void note_local_attribute(Declaration ld, ostream& oss) {
      Super::note_local_attribute(ld,oss);
      Declaration_info(ld)->decl_flags |= LOCAL_ATTRIBUTE_FLAG;
      int i = LOCAL_UNIQUE_PREFIX(ld);
      dump_compute(string("c")+i+"_"+decl_name(ld),oss);
    }
    
    void note_attribute_decl(Declaration ad, ostream& oss) {
      Super::note_attribute_decl(ad,oss);
      dump_compute(string("c_")+decl_name(ad),oss);
    }
    
    void note_var_value_decl(Declaration vd, ostream& oss) {
      Super::note_var_value_decl(vd,oss);
      Declaration_info(vd)->decl_flags |= VAR_VALUE_DECL_FLAG;
      const char *name = decl_name(vd);
      oss << indent() << "var s_" << name << ": EvalStatus = " 
	  << "UNEVALUATED;\n";
    }

    void implement(ostream& oss) {
      implement_local_attributes(local_attributes,oss);
      implement_attributes(attribute_decls,top_level_matches,oss);
      implement_var_value_decls(var_value_decls,top_level_matches,oss);

      // const char *name = decl_name(module_decl);

      Declarations ds = block_body(module_decl_contents(module_decl));
      
      // Implement finish routine:
      oss << indent() << "def finish() : Unit = {\n";
      ++nesting_level;
      //? not sure how to do this:
      //? we don't want to force these to finish in the wrong order.
      for (Declaration d = first_Declaration(ds); d; d = DECL_NEXT(d)) {
	switch(Declaration_KEY(d)) {
	default:
	  break;
	case KEYvalue_decl:
	  if (!def_is_constant(value_decl_def(d))) {
	    oss << indent() << "c_" << decl_name(d) << "();\n";
	  }
	  break;
	}
      }
      // oss << indent() << "super.finish();\n";
      --nesting_level;
      oss << indent() << "}\n\n";
      clear_implementation_marks(module_decl);
    }
  };

  Super* get_module_info(Declaration m) {
    return new ModuleInfo(m);
  }

  void implement_function_body(Declaration f, ostream& os) {
    Type fty = function_decl_type(f);
    Declaration rdecl = first_Declaration(function_type_return_values(fty));
    Block b = function_decl_body(f);
    bool is_col = direction_is_collection(value_decl_direction(rdecl));
    const char *name = decl_name(f);
    
    if (is_col) {
      dump_init_collection(rdecl,os);
    }
    os << "\n"; // blank line

    dump_Block(b,dump_attr_assign,rdecl,os);
    dump_default_return(value_decl_default(rdecl),
			value_decl_direction(rdecl),
			string("\"local ")+name+"\"",os);
  }

  void implement_value_use(Declaration vd, ostream& os) {
    int flags = Declaration_info(vd)->decl_flags;
    if (flags & LOCAL_ATTRIBUTE_FLAG) {
      os << "a" << LOCAL_UNIQUE_PREFIX(vd) << "_"
	 << decl_name(vd) << ".evaluate(anchor)"; 
    } else if (flags & VAR_VALUE_DECL_FLAG) {
      os << "c_" << decl_name(vd) << "()";
    } else if (flags & ATTRIBUTE_DECL_FLAG) {
      // not currently active:
      // (but should work just fine)
      os << "a" << "_" << decl_name(vd) << ".evaluate";
    } else {
      aps_error(vd,"internal_error: What is special about this?");
    }
  }
};

Implementation *dynamic_impl = new Dynamic();
