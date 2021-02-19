#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jbb-alloc.h"
#include "aps-ag.h"

int type_debug = FALSE;

static Type Boolean_Type;
static Type Integer_Type;
static Type Real_Type;
static Type Char_Type;
static Type String_Type;
static Type error_type;

int remote_type_p(Type ty);

Type function_type_return_type(Type ft)
{
  return value_decl_type(first_Declaration(function_type_return_values(ft)));
}

Type constructor_return_type(Declaration decl) {
  Type function_type = constructor_decl_type(decl);
  Declaration rd = first_Declaration(function_type_return_values(function_type));
  Type rt = value_decl_type(rd);
  return rt;
}

Declaration current_module = NULL;

static void* do_typechecking(void* ignore, void*node) {
  // find places where Expression, Pattern or Default is used
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYUnit:
    { Unit u=(Unit)node;
      switch (Unit_KEY(u)) {
      case KEYwith_unit:
	{ char *str = (char *)with_unit_name(u);
	  char *prefix = (char *)HALLOC(strlen(str));
	  Program p;
	  strcpy(prefix,str+1);
	  prefix[strlen(prefix)-1] = '\0';
	  p = find_Program(make_string(prefix));
	  type_Program(p);
	}
      default:
	break;
      }
    }
    break;
  case KEYDeclaration:
    {
      Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
        case KEYmodule_decl:
          current_module = (Declaration) node;
          break;
        case KEYconstructor_decl:
        {
          Declarations formals = function_type_formals(constructor_decl_type(decl));
          Declaration formals_ptr1, formals_ptr2;
          int i, j;

          for (formals_ptr1 = first_Declaration(formals), i = 0;
              formals_ptr1 != NULL;
              formals_ptr1 = DECL_NEXT(formals_ptr1), i++) {

              char* formal_name = decl_name(formals_ptr1);

              for (formals_ptr2 = DECL_NEXT(formals_ptr1), j = i + 1;
                formals_ptr2 != NULL;
                formals_ptr2 = DECL_NEXT(formals_ptr2), j++) {
                  if (strcmp(formal_name, decl_name(formals_ptr2)) == 0) {
                    aps_error(decl, "Duplicate constructor formal name: \"%s\" at indices: %i, %i in \"%s\" constructor", formal_name, i, j, decl_name(decl));
                  }
              }
          }

          Type rt = constructor_return_type(decl);
          switch (Type_KEY(rt))
          {
            case KEYremote_type:
              aps_error(decl, "Constructor for remote type is forbidden (constructor %s(...): remote %s)", decl_name(decl), symbol_name(use_name(type_use_use(remote_type_nodetype(rt)))));
              break;
            case KEYtype_use:
              {
                TypeEnvironment type_env = Use_info(type_use_use(rt))->use_type_env;
                while (type_env != NULL) {
                  switch (Declaration_KEY(type_env->source))
                  {
                    case KEYmodule_decl:
                      if (type_env->source != current_module) {
                            aps_error(decl, "Adding a constructor \"%s\" in extending module is forbidden", decl_name(decl));
                      }
                      break;
                    default:
                      break;
                  }

                  type_env = type_env->outer;
                }
                break;
              }
            default:
              break;
          }
          break;
        }
      case KEYvalue_decl:
	check_default_type(value_decl_default(decl),value_decl_type(decl));
	break;
	/* violated all over basic.aps
      case KEYfunction_decl:
	{
	  Type ft = function_decl_type(decl);
	  Declaration f = first_Declaration(function_type_formals(ft));

	  for (; f; f = DECL_NEXT(f)) {
	    if (type_is_phylum(infer_formal_type(f))) {
	      aps_error(decl,"Cannot pass syntactic (phylum) types to functions");
	    }
	  }
	}
	break;
	*/
      case KEYattribute_decl:
	{
	  Type at = attribute_decl_type(decl);
	  Declarations rdecls = function_type_return_values(at);
	  Type nt = infer_formal_type(first_Declaration(function_type_formals(at)));
	  if (!type_is_phylum(nt)) {
	    aps_error(decl,"Can only attribute phyla");
	  }
	  check_default_type(attribute_decl_default(decl),
			     value_decl_type(first_Declaration(rdecls)));
	}
	break;
      case KEYpattern_decl:
	{
	  Declarations rdecls =
	    function_type_return_values(pattern_decl_type(decl));
	  check_pattern_type(pattern_decl_choices(decl),
			     value_decl_type(first_Declaration(rdecls)));
	  return 0;
	}
	break;

      case KEYtype_decl:
	if (type_is_phylum(type_decl_type(decl))) {
	  aps_error(decl,"Cannot rename a phylum to a type");
	}
	break;
      case KEYphylum_decl:
	if (!type_is_phylum(phylum_decl_type(decl))) {
	  aps_error(decl,"Cannot rename a type to a phylum");
	}
	break;

      case KEYtop_level_match:
	{
	  Match m = top_level_match_m(decl);
	  Type ty = infer_pattern_type(matcher_pat(m));
	  /*  otherwise omitted! */
	  if (type_debug) {
	    printf("inferred type of tlm:%d as ",tnode_line_number(m));
	    print_Type(ty,stdout);
	    printf("\n");
	  }
	  traverse_Block(do_typechecking,ignore,matcher_body(m));
	  return 0;
	}
	break;
	  
	/* no need for multi_call since they don't exist yet */
	
      case KEYnormal_assign:
	{
	  Expression lhs = normal_assign_lhs(decl);
	  switch (Expression_KEY(lhs)) {
	  case KEYfuncall:
	    { 
	      Expression n = first_Actual(funcall_actuals(lhs));
	      Type ty = infer_expr_type(n);
	      if (remote_type_p(ty)) {
		aps_error(decl,"assigning normal attribute of remote node");
	      }
	    }
	    break;
	  }
	}
	/* FALL THROUGH */
      case KEYcollect_assign:
      {
        Expression lhs = assign_lhs(decl);
        Declaration lhs_use_decl;
        switch (Expression_KEY(lhs))
        {
          case KEYfuncall:
            lhs_use_decl = Use_info(value_use_use(funcall_f(lhs)))->use_decl;
            if (def_is_constant(declaration_def(lhs_use_decl))) {
              aps_error(lhs_use_decl, "Attribute collection \"%s\" has to be declared as VAR to be assigned", decl_name(lhs_use_decl));
            }
            break;
          case KEYvalue_use:
            lhs_use_decl = Use_info(value_use_use(lhs))->use_decl;
            if (def_is_constant(declaration_def(lhs_use_decl))) {
              aps_error(lhs_use_decl, "Global collection \"%s\" has to be declared as VAR to be assigned", decl_name(lhs_use_decl));
            }
            break;
          default:
            break;
        }
      }
	check_expr_type(assign_rhs(decl),infer_expr_type(assign_lhs(decl)));
	return 0;
	break;

      case KEYif_stmt:
	check_expr_type(if_stmt_cond(decl),Boolean_Type);
	traverse_Block(do_typechecking,ignore,if_stmt_if_true(decl));
	traverse_Block(do_typechecking,ignore,if_stmt_if_false(decl));
	return 0;
	break;

      case KEYfor_stmt:
	check_matchers_type(for_stmt_matchers(decl),
			    infer_expr_type(for_stmt_expr(decl)));
	return 0;
	break;
      case KEYcase_stmt:
	check_matchers_type(case_stmt_matchers(decl),
			    infer_expr_type(case_stmt_expr(decl)));
	traverse_Block(do_typechecking,ignore,case_stmt_default(decl));
	return 0;
	break;

      default:
	break;
      }
    }
    break;

  case KEYExpression:
    infer_expr_type((Expression)node);
    return 0;
    break;
  case KEYPattern:
    infer_pattern_type((Pattern)node);
    return 0;
    break;
    
  case KEYMatch:
  case KEYDefault:
    return 0;
    
  case KEYType:
    {
      Type ty = (Type)node;
      switch (Type_KEY(ty)) {
      case KEYtype_inst:
	{
	  Use mu = module_use_use(type_inst_module(ty));
	  Declaration mdecl = USE_DECL(mu);
	  Declaration rdecl = module_decl_result_type(mdecl);
	  Def rdef = some_type_decl_def(rdecl);
	  TypeEnvironment te =
	    (TypeEnvironment)HALLOC(sizeof(struct TypeContour));
	  extern int aps_yylineno;
	  Declaration tdecl = (Declaration)tnode_parent(ty);
	  Use tu;
	  Use u;
	  Type fty;

	  while (ABSTRACT_APS_tnode_phylum(tdecl) != KEYDeclaration)
	    tdecl = (Declaration)tnode_parent(tdecl);
	  tu = use(def_name(some_type_decl_def(tdecl)));
	  te->outer = USE_TYPE_ENV(mu);
	  te->source = mdecl;
	  te->type_formals = module_decl_type_formals(mdecl);
	  te->result = (Declaration)tnode_parent(ty);
	  te->u.type_actuals = type_inst_type_actuals(ty);
	  aps_yylineno = tnode_line_number(ty);
	  USE_DECL(tu) = tdecl;
	  USE_TYPE_ENV(tu) = 0;
	  u = qual_use(type_use(tu),def_name(rdef));
	  USE_TYPE_ENV(u) = te;
	  USE_DECL(u) = rdecl;
	  aps_yylineno = tnode_line_number(mdecl);
	  check_type_actuals(type_inst_type_actuals(ty),
			     module_decl_type_formals(mdecl),u);
	  fty = function_type
	    (module_decl_value_formals(mdecl),
	     list_Declarations(value_decl(copy_Def(rdef),
					  function_type(nil_Declarations(),
							nil_Declarations()),
					  direction(0,0,0),
					  no_default())));
	  (void)check_actuals(type_inst_actuals(ty),fty,u);
	}
	return 0;
      }
    }
    break;
  default:
    break;
  }
  return ignore;
}

static Symbol Boolean_Symbol;
static Symbol Integer_Symbol;
static Symbol Real_Symbol;
static Symbol Char_Symbol;
static Symbol String_Symbol;

static void *find_types(void *cont, void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYType:
    switch (Type_KEY((Type)node)) {
    case KEYtype_use:
      {
	Use u = type_use_use((Type)node);
	switch (Use_KEY(u)) {
	case KEYuse:
	  if (use_name(u) == Boolean_Symbol) Boolean_Type = (Type)node;
	  else if (use_name(u) == Integer_Symbol) Integer_Type = (Type)node;
	  else if (use_name(u) == Real_Symbol) Real_Type = (Type)node;
	  else if (use_name(u) == Char_Symbol) Char_Type = (Type)node;
	  else if (use_name(u) == String_Symbol) String_Type = (Type)node;
	  else return cont;
	  if (Boolean_Type != 0 && Integer_Type != 0 &&
	      Real_Type != 0 && Char_Type != 0 && String_Type != 0)
	    return 0;
	  break;
	default:
	  break;
	}
      }
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
  return cont;
}

static void init_types() {
  Program basic_program = find_Program(make_string("basic"));
  Boolean_Symbol = intern_symbol("Boolean");
  Integer_Symbol = intern_symbol("Integer");
  Real_Symbol = intern_symbol("IEEEdouble");
  Char_Symbol = intern_symbol("Character");
  String_Symbol = intern_symbol("String");
  traverse_Program(find_types,basic_program,basic_program);
  if (Boolean_Type == 0 ||
      Integer_Type == 0 ||
      Real_Type == 0 ||
      Char_Type == 0 ||
      String_Type == 0)
    fatal_error("Can't find Boolean/Integer/String");
  error_type = Boolean_Type;
}

static char* clean_string_const_token(char* p) {
  p++;
  p[strlen(p)-1] = 0;
  return p;
}

static void* validate_canonicals(void* ignore, void*node) {
  int BUFFER_SIZE = 1000;
  Symbol symb_test_canonical_type = intern_symbol("test_canonical_type");
  Symbol symb_test_canonical_base_type = intern_symbol("test_canonical_base_type");
  Symbol symb_test_canonical_signature = intern_symbol("test_canonical_signature");

  switch (ABSTRACT_APS_tnode_phylum(node))
  {
  case KEYDeclaration:
  {
    Declaration decl = (Declaration) node;
    switch (Declaration_KEY(decl))
    {
    case KEYpragma_call:
    {
      Symbol pragma_value = pragma_call_name(decl);
      if (symb_test_canonical_signature == pragma_value ||
          symb_test_canonical_type == pragma_value ||
          symb_test_canonical_base_type == pragma_value) {
        Expressions exprs = pragma_call_parameters(decl);
        Expression type_expr = first_Expression(exprs);
        Expression result_expr = Expression_info(type_expr)->next_expr;

        Type type = type_value_T(type_expr);
        String expected_string = string_const_token(result_expr);

        char actual_to_string[BUFFER_SIZE];
        FILE* f = fmemopen(actual_to_string, sizeof(actual_to_string), "w");
        if (symb_test_canonical_signature == pragma_value) {
          print_canonical_signature_set(infer_canonical_signatures(canonical_type(type)), f);
        } else if (symb_test_canonical_type == pragma_value) {
          print_canonical_type(canonical_type(type), f);
        } else if (symb_test_canonical_base_type == pragma_value) {
          print_canonical_type(canonical_type_base_type(canonical_type(type)), f);
        }
        fclose(f);

        char expected[BUFFER_SIZE];
        sprintf(expected, "%s", expected_string);

        char* expected_cleaned = clean_string_const_token(&expected);
        
        if (strcmp(actual_to_string, expected_cleaned) != 0) {
          aps_error(type,"Failed: %d  expected `%s` but got `%s`", tnode_line_number(type), expected_cleaned, actual_to_string);
        }
      }
    }
    }
  }
  }
  return node;
}

static Declaration module_TYPE;
static Declaration module_PHYLUM;

static void set_root_phylum(void *ignore, void *node)
{
  switch (ABSTRACT_APS_tnode_phylum(node))
  {
  case KEYDeclaration:
  {
    Declaration d = (Declaration)node;
    switch (Declaration_KEY(d))
    {
    case KEYmodule_decl:
    {
      if (module_TYPE == 0 && streq(decl_name(d), "TYPE"))
      {
        module_TYPE = d;
      }
      else if (module_PHYLUM == 0 && streq(decl_name(d), "PHYLUM"))
      {
        module_PHYLUM = d;
      }

      return NULL;
    }
    }
  }
  }

  return node;
}

void type_Program(Program p)
{
  static int init_type_Program = 0;
  char *saved_filename = aps_yyfilename;
  Program basic_program = find_Program(make_string("basic"));

  if (!init_type_Program) {
    init_type_Program = 1;
    init_types();
  }

  bind_Program(p);
  if (PROGRAM_IS_TYPED(p)) return;
  Program_info(p)->program_flags |= PROGRAM_TYPED_FLAG;

  if (basic_program != p) {
    type_Program(basic_program);
  }
  aps_yyfilename = (char *)program_name(p);
  if (type_debug) printf("Type checking code in \"%s.aps\"\n",aps_yyfilename);
  traverse_Program(do_typechecking,p,p);
  aps_yyfilename = saved_filename;

  traverse_Program(set_root_phylum, p, p);
  initialize_canonical_signature(module_TYPE, module_PHYLUM);

  traverse_Program(validate_canonicals,p,p);
}

Type infer_expr_type(Expression e)
{
  Type ty = Expression_info(e)->expr_type;
  if (ty != 0) return ty;
  switch (Expression_KEY(e)) {
  default:
    aps_error(e,"unknown expression");
    break;
  case KEYvalue_use:
    {
      Use u = value_use_use(e);
      Declaration decl = USE_DECL(u);
      TypeEnvironment type_env = USE_TYPE_ENV(u);
      /* now we set ty and then subst it */
      if (decl) switch (Declaration_KEY(decl)) {
      case KEYvalue_decl:
      case KEYattribute_decl:
      case KEYconstructor_decl:
      case KEYfunction_decl:
      case KEYprocedure_decl:
	ty = some_value_decl_type(decl);
	break;
      case KEYvalue_renaming:
	ty = infer_expr_type(value_renaming_old(decl));
	break;
      case KEYformal:
	ty = infer_formal_type(decl);
	break;
      default:
	aps_error(decl,"unknown expression decl");
	break;
      }
      ty = type_subst(u,ty);
    }
    break;
  case KEYtyped_value:
    ty = typed_value_type(e);
    check_expr_type(typed_value_expr(e),ty);
    break;
  case KEYinteger_const:
    ty = Integer_Type;
    if (type_debug) {
      printf("integer constant %s has type",(char*)(integer_const_token(e)));
      print_Type(ty,stdout);
      printf("\n");
    }
    break;
  case KEYreal_const:
    ty = Real_Type;
    break;
  case KEYstring_const:
    ty = String_Type;
    break;
  case KEYchar_const:
    ty = Char_Type;
    break;
  case KEYfuncall:
    ty = infer_function_return_type(funcall_f(e),funcall_actuals(e));
    break;
  case KEYappend:
    ty = infer_expr_type(append_s1(e));
    check_expr_type(append_s2(e),ty);
    break;
  case KEYrepeat:
    ty = infer_element_type(repeat_expr(e));
    break;
  case KEYguarded:
    ty = infer_expr_type(guarded_expr(e));
    check_expr_type(guarded_cond(e),Boolean_Type);
    break;
  case KEYcontrolled:
    (void)infer_element_type(controlled_set(e));
    ty = infer_expr_type(controlled_expr(e));
    break;
    
  case KEYundefined:
  case KEYno_expr:
  case KEYempty:
    aps_error(e,"cannot infer a type for empty things");
    break;

  case KEYclass_value:
  case KEYmodule_value:
  case KEYsignature_value:
  case KEYtype_value:
    break;

  case KEYpattern_value:
    ty = infer_pattern_type(pattern_value_p(e));
  }
  Expression_info(e)->expr_type = ty;
  return ty;
}

void check_expr_type(Expression e, Type type)
{
  if (type == 0) {
    infer_expr_type(e);
    return;
  }
  if (Expression_info(e)->expr_type) {
    check_type_equal(e,type,Expression_info(e)->expr_type);
    return;
  }
  switch (Expression_KEY(e)) {
  case KEYvalue_use:
    {
      Use u = value_use_use(e);
      Declaration decl = USE_DECL(u);
      TypeEnvironment type_env = USE_TYPE_ENV(u);
      Type ty;
      if (!decl) {
	Expression_info(e)->expr_type = type;
	return;
      }
      /* now we set ty and then check it */
      switch (Declaration_KEY(decl)) {
      case KEYvalue_decl:
      case KEYattribute_decl:
      case KEYconstructor_decl:
      case KEYfunction_decl:
      case KEYprocedure_decl:
	ty = some_value_decl_type(decl);
	break;
      case KEYvalue_renaming:
	ty = infer_expr_type(value_renaming_old(decl));
	break;
      case KEYformal:
	ty = infer_formal_type(decl);
	break;
      default:
	aps_error(decl,"unknown expression decl");
	break;
      }
      check_type_subst(e,type,u,ty);
    }
    break;
  case KEYtyped_value:
  case KEYinteger_const:
  case KEYreal_const:
  case KEYstring_const:
  case KEYchar_const:
    check_type_equal(e,type,infer_expr_type(e));
    break;
  case KEYfuncall:
    check_function_return_type(funcall_f(e),funcall_actuals(e),type);
    break;
  case KEYappend:
    check_expr_type(append_s1(e),type);
    check_expr_type(append_s2(e),type);
    break;
  case KEYrepeat:
    check_element_type(repeat_expr(e),type);
    break;
  case KEYguarded:
    check_expr_type(guarded_expr(e),type);
    check_expr_type(guarded_cond(e),Boolean_Type);
    break;
  case KEYcontrolled:
    (void)infer_element_type(controlled_set(e));
    check_expr_type(controlled_expr(e),type);
    break;
    
  case KEYundefined:
  case KEYno_expr:
  case KEYempty:
    /* do nothing */
    break;

  case KEYclass_value:
  case KEYmodule_value:
  case KEYsignature_value:
  case KEYtype_value:
  case KEYpattern_value:
    aps_error(e,"not allowed in expression");
  }
  Expression_info(e)->expr_type = type;
}

Type sig_element_type(Signature sig)
{
  switch (Signature_KEY(sig)) {
  case KEYsig_inst:
    /*! Hack! */
    return first_TypeActual(sig_inst_actuals(sig));
  case KEYmult_sig:
    {
      Type ty = sig_element_type(mult_sig_sig1(sig));
      if (ty) return ty;
      return sig_element_type(mult_sig_sig2(sig));
    }
  default:
    /*! Hack! */
    return 0;
  }
}

BOOL type_is_phylum(Type ty)
{
  switch (Type_KEY(ty)) {
  case KEYtype_use:
    {
      Declaration td = USE_DECL(type_use_use(ty));
      if (td == NULL) break;
      switch (Declaration_KEY(td)) {
      case KEYtype_decl:
      case KEYtype_formal:
	return FALSE;
      case KEYphylum_decl:
      case KEYphylum_formal:
	return TRUE;
      case KEYtype_renaming:
	return type_is_phylum(type_renaming_old(td));
      default:
	aps_error(td,"internal error: unknown type declaration");
      }
    }
  case KEYtype_inst:
    {
      Declaration md = USE_DECL(module_use_use(type_inst_module(ty)));
      if (md == 0) break;
      switch (Declaration_KEY(module_decl_result_type(md))) {
      case KEYtype_decl:
	return FALSE;
      case KEYphylum_decl:
	return TRUE;
      default:
	aps_error(md,"internal error: unknown result type declaration");
      }
    }
  case KEYno_type:
    {
      Declaration p = (Declaration)tnode_parent(ty);
      switch (Declaration_KEY(p)) {
      case KEYtype_decl:
	return FALSE;
      case KEYphylum_decl:
	return TRUE;
      default:
	aps_error(ty,"internal error: unknown no type type decl");
      }
    }
  case KEYremote_type:
  case KEYfunction_type:
    return FALSE;
  case KEYprivate_type:
    return type_is_phylum(private_type_rep(ty));
  }
  return FALSE;
}

Type type_element_type(Type st)
{
  for (;;) {
    //! horrible hack
    switch (Type_KEY(st)) {
    case KEYtype_use:
      {
	Use u = type_use_use(st);
	Declaration decl = USE_DECL(u);
        Type rt;

	switch (Declaration_KEY(decl)) {
	case KEYtype_renaming:
	  st = type_renaming_old(decl);
	  break;
	case KEYsome_type_decl:
	  st = some_type_decl_type(decl);
	  break;
	case KEYsome_type_formal:
	  {
	    Type ty = sig_element_type(some_type_formal_sig(decl));
	    if (ty != 0)
	      return type_subst(u,ty);
	  }
	  /* FALL THROUGH */
	default:
	  aps_error(decl,"not sure what sort of collection type this is");
	  return error_type;
	}
	rt = type_element_type(st);
	if (rt == error_type) {
	  aps_error(st,"  used here");
	}
	return type_subst(u,rt);
      }
      break;
    case KEYtype_inst:
      {
	Type ta = first_TypeActual(type_inst_type_actuals(st));
	Declaration mdecl = USE_DECL(module_use_use(type_inst_module(st)));
	Declaration rdecl = module_decl_result_type(mdecl);
        Type rt;

	if (ta) return ta; /*! Hack: assume first parameter is type */
	rt =  type_element_type(some_type_decl_type(rdecl));
	if (rt == error_type) {
	  aps_error(st,"  used here");
	}
	return rt;
      }
      break;
    case KEYremote_type:
      return type_element_type(remote_type_nodetype(st));
    default:
      aps_error(st,"not sure what sort of collection type this is");
      return error_type;
    }
  }
}

Type infer_element_type(Expression e)
{
  Type st = infer_expr_type(e);
  return type_element_type(st);
}

void check_element_type(Expression e, Type type)
{
  /* For now, just be lazy */
  check_type_equal(e,type,infer_element_type(e));
}

void check_type_actuals(TypeActuals tacts, Declarations tfs, Use type_envs)
{
  Type tact;
  Declaration tf;
  for (tact = first_TypeActual(tacts), tf = first_Declaration(tfs);
       tact != 0 && tf != 0; tact = TYPE_NEXT(tact), tf = DECL_NEXT(tf)) {
    check_type_signatures(tact,tact,type_envs,some_type_formal_sig(tf));
  }
  if (tf != 0) {
    aps_error(tacts,"too few type actuals");
  } else if (tact != 0) {
    aps_error(tact,"too many type actuals");
  }
}

/* check actuals, possibly side-effecting type_env.
   Handles sequence formals.
   return return type */
Type check_actuals(Actuals args, Type ftype, Use type_envs)
{
  Declarations formals;
  Type rty;
  Declaration formal;
  Expression arg;

  ftype = base_type(ftype);

  if (type_debug) {
    printf("checking actuals for ");
    print_Type(ftype,stdout);
    printf(" using/for ");
    print_Use(type_envs,stdout);
    printf("\n");
  }

  switch (Type_KEY(ftype)) {
  case KEYfunction_type:
    formals = function_type_formals(ftype);
    rty = function_type_return_type(ftype);
    break;
  default:
    aps_error(tnode_parent(args),"not a function being called");
    return error_type;
  }

  formal = first_Declaration(formals);
  for (arg = first_Actual(args); arg != 0; arg = EXPR_NEXT(arg)) {
    if (formal == 0) {
      aps_error(args,"too many parameters to function");
      break;
    }
    /* As soon as type environment is complete, we
     * no longer use inference
     */
    if (type_envs == 0 || is_complete(USE_TYPE_ENV(type_envs))) {
      check_expr_type(arg,type_subst(type_envs,formal_type(formal)));
    } else {
      check_type_subst(arg,infer_expr_type(arg),type_envs,formal_type(formal));
    }
    if (Declaration_KEY(formal) != KEYseq_formal)
      formal = DECL_NEXT(formal);
  }
  if (formal != 0 && Declaration_KEY(formal) != KEYseq_formal) {
    aps_error(args,"too few parameters to function");
  }
  rty = type_subst(type_envs,rty);
  if (type_debug) {
    printf("return type is ");
    print_Type(rty,stdout);
    printf("\n");
  }
  return rty;
}

Type infer_function_return_type(Expression f, Actuals args) {
  switch (Expression_KEY(f)) {
  case KEYvalue_use:
    {
      Use u = value_use_use(f);
      Declaration decl = USE_DECL(u);
      Type ty;
      Declarations formals;
      Type rty;

      if (!decl) {
	aps_error(u,"No binding!");
	return error_type;
      }

      /* now we set ty and then check it */
      if (decl) switch (Declaration_KEY(decl)) {
      case KEYvalue_decl:
	ty = value_decl_type(decl);
	break;
      case KEYattribute_decl:
	ty = attribute_decl_type(decl);
	break;
      case KEYsome_function_decl:
	ty = some_function_decl_type(decl);
	break;
      case KEYconstructor_decl:
	ty = constructor_decl_type(decl);
	break;
      case KEYvalue_renaming:
	/* This will end up with warnings frequently */
	ty = infer_expr_type(value_renaming_old(decl));
	break;
      case KEYformal:
	ty = infer_formal_type(decl);
	break;
      default:
	aps_error(decl,"unknown expression decl");
	return error_type;
	break;
      }
      
      return check_actuals(args,ty,u);
    }

  case KEYtyped_value:
    check_expr_type(f,typed_value_type(f));
    return check_actuals(args,typed_value_type(f),0);

  default:
    return check_actuals(args,infer_expr_type(f),0);
  }
}

void check_function_return_type(Expression f, Actuals args, Type type) {
  switch (Expression_KEY(f)) {
  case KEYvalue_use:
    {
      Use u = value_use_use(f);
      Declaration decl = USE_DECL(u);
      Type ty;
      Declarations formals;
      Type rty;

      if (!decl) return;

      /* now we set ty and then check it */
      if (decl) switch (Declaration_KEY(decl)) {
      case KEYvalue_decl:
	ty = value_decl_type(decl);
	break;
      case KEYattribute_decl:
	ty = attribute_decl_type(decl);
	break;
      case KEYsome_function_decl:
	ty = some_function_decl_type(decl);
	break;
      case KEYconstructor_decl:
	ty = constructor_decl_type(decl);
	break;
      case KEYvalue_renaming:
	/* This will end up with warnings frequently */
	ty = infer_expr_type(value_renaming_old(decl));
	break;
      case KEYformal:
	ty = infer_formal_type(decl);
      default:
	aps_error(decl,"unknown expression decl");
	return;
      }
      ty = base_type(ty);
      if (Type_KEY(ty) != KEYfunction_type) {
	aps_error(f,"function called does not have function type");
        return;
      }
      {
	Type rty = function_type_return_type(ty);
	if (type_debug) {
	  printf("checking ");
	  print_Type(type,stdout);
	  printf(" for return type ");
	  print_Type(rty,stdout);
	  printf(" using ");
	  print_Use(u,stdout);
	  printf("\n");
	}
	check_type_subst(tnode_parent(f),type,u,rty);
      }
      check_actuals(args,ty,u);
    }
    break;

  case KEYtyped_value:
    check_expr_type(f,typed_value_type(f));
    check_type_equal(f,type,check_actuals(args,typed_value_type(f),0));
    break;

  default:
    check_type_equal(f,type,check_actuals(args,infer_expr_type(f),0));
    break;
  }
}

Type infer_pattern_type(Pattern p)
{
  Type ty = Pattern_info(p)->pat_type;
  if (ty != 0) return ty;
  switch(Pattern_KEY(p)) {
  case KEYpattern_use:
    {
      Use u = pattern_use_use(p);
      Declaration d = USE_DECL(u);
      
      if (!d) return 0;
      switch (Declaration_KEY(d)) {
      case KEYconstructor_decl:
	ty = constructor_decl_type(d);
	break;
      case KEYpattern_decl:
	ty = pattern_decl_type(d);
	break;
      case KEYpattern_renaming:
	/* This will end up with warnings frequently */
	ty = infer_pattern_type(pattern_renaming_old(d));
	break;
      default:
	aps_error(d,"unknown pattern decl");
	return error_type;
	break;
      }
      ty = type_subst(u,ty);
    }
    break;
  case KEYtyped_pattern:
    ty = typed_pattern_type(p);
    check_pattern_type(typed_pattern_pat(p),ty);
    break;
  case KEYpattern_call:
    ty = infer_pfunction_return_type(pattern_call_func(p),
				     pattern_call_actuals(p));
    break;
  case KEYrest_pattern:
    ty = infer_pattern_type(rest_pattern_constraint(p));
    break;
  case KEYand_pattern:
    ty = infer_pattern_type(and_pattern_p2(p));
    check_pattern_type(and_pattern_p1(p),ty);
    break;
  case KEYpattern_var:
    switch (Type_KEY(formal_type(pattern_var_formal(p)))) {
    case KEYno_type:
      aps_error(p,"cannot infer type of untyped pattern variable");
      break;
    default:
      break;
    }
    break;
  case KEYcondition:
    aps_error(p,"cannot infer type of condition pattern");
    check_expr_type(condition_e(p),Boolean_Type);
    break;
  default:
    aps_error(p,"cannot infer type of unusual pattern");
  }
  Pattern_info(p)->pat_type = ty;
  return ty;
}

void check_pattern_type(Pattern p, Type type)
{
  Type ty = Pattern_info(p)->pat_type;
  if (type == 0) {
    (void)infer_pattern_type(p);
    return;
  }
  if (ty != 0) {
    check_type_equal(p,type,ty);
    return;
  }
  Pattern_info(p)->pat_type = type;
  switch(Pattern_KEY(p)) {
  case KEYpattern_use:
    {
      Use u = pattern_use_use(p);
      Declaration d = USE_DECL(u);
      Type ty;
      
      if (!d) return;
      switch (Declaration_KEY(d)) {
      case KEYconstructor_decl:
	ty = constructor_decl_type(d);
	break;
      case KEYpattern_decl:
	ty = pattern_decl_type(d);
	break;
      case KEYpattern_renaming:
	/* This will end up with warnings frequently */
	ty = infer_pattern_type(pattern_renaming_old(d));
	break;
      default:
	aps_error(d,"unknown pattern decl");
	return;
	break;
      }
      check_type_subst(p,type,u,ty);
    }
    break;
  case KEYtyped_pattern:
    check_type_equal(p,type,infer_pattern_type(p));
    break;
  case KEYpattern_call:
    check_pfunction_return_type(pattern_call_func(p),
				pattern_call_actuals(p),type);
    break;
  case KEYrest_pattern:
    check_pattern_type(rest_pattern_constraint(p),type);
    break;
  case KEYand_pattern:
    check_pattern_type(and_pattern_p2(p),type);
    check_pattern_type(and_pattern_p1(p),type);
    break;
  case KEYpattern_var:
    switch (Type_KEY(formal_type(pattern_var_formal(p)))) {
    case KEYno_type:
      break;
    default:
      check_type_equal(p,formal_type(pattern_var_formal(p)),type);
      break;
    }
    break;
  case KEYcondition:
    check_expr_type(condition_e(p),Boolean_Type);
    break;
  case KEYno_pattern:
    break;
  default:
    aps_error(p,"cannot check type of unusual pattern");
    break;
  }    
}


/* check actuals, possibly side-effecting type_env.
   Handles sequence formals.
   return return type */
Type check_pattern_actuals(PatternActuals args, Type ftype, Use type_envs)
{
  Declarations formals;
  Type rty;
  Declaration formal;
  Pattern arg;

  switch (Type_KEY(ftype)) {
    /*! change to handle renamings */
  case KEYfunction_type:
    formals = function_type_formals(ftype);
    rty = function_type_return_type(ftype);
    break;
  default:
    aps_error(tnode_parent(args),"not a pattern function being called");
    return error_type;
  }

  formal = first_Declaration(formals);
  for (arg = first_PatternActual(args); arg != 0; arg = PAT_NEXT(arg)) {
    if (formal == 0) {
      aps_error(args,"too many parameters to pattern");
      break;
    }
    /* As soon as type environment is complete, we
     * no longer use inference
     */
    if (type_envs == 0 || is_complete(USE_TYPE_ENV(type_envs))) {
      check_pattern_type(arg,type_subst(type_envs,formal_type(formal)));
    } else {
      check_type_subst(arg,infer_pattern_type(arg),
		       type_envs,formal_type(formal));
    }
    if (Declaration_KEY(formal) != KEYseq_formal)
      formal = DECL_NEXT(formal);
  }
  if (formal != 0 && Declaration_KEY(formal) != KEYseq_formal) {
    aps_error(args,"too few parameters to function");
  }
  return type_subst(type_envs,rty);
}

Type infer_pfunction_return_type(Pattern f, PatternActuals args) {
  switch (Pattern_KEY(f)) {
  case KEYpattern_use:    
    {
      Use u = pattern_use_use(f);
      Declaration d = USE_DECL(u);
      Type ty;

      if (!d) return error_type;
      switch (Declaration_KEY(d)) {
      case KEYconstructor_decl:
	ty = constructor_decl_type(d);
	break;
      case KEYpattern_decl:
	ty = pattern_decl_type(d);
	break;
      case KEYpattern_renaming:
	/* This will end up with warnings frequently */
	ty = infer_pattern_type(pattern_renaming_old(d));
	break;
      default:
	aps_error(d,"unknown pattern decl");
	return error_type;
	break;
      }
      return check_pattern_actuals(args,type_subst(u,ty),u);
    }

  default:
    return check_pattern_actuals(args,infer_pattern_type(f),0);
  }
}

void check_pfunction_return_type(Pattern f, PatternActuals args, Type type) {
  switch (Pattern_KEY(f)) {
  case KEYpattern_use:
    {
      Use u = pattern_use_use(f);
      Declaration decl = USE_DECL(u);
      Type ty;
      Declarations formals;
      Type rty;

      if (!decl) return;

      /* now we set ty and then check it */
      switch (Declaration_KEY(decl)) {
      case KEYconstructor_decl:
	ty = constructor_decl_type(decl);
	break;
      case KEYpattern_decl:
	ty = pattern_decl_type(decl);
	break;
      case KEYpattern_renaming:
	/* This will end up with warnings frequently */
	ty = infer_pattern_type(pattern_renaming_old(decl));
	break;
      default:
	aps_error(decl,"unknown pattern decl");
	return;
	break;
      }

      ty = base_type(ty);
      if (Type_KEY(ty) != KEYfunction_type) {
	aps_error(f,"pattern call function does not have functional type");
	return;
      }

      check_type_subst(tnode_parent(f),type,u,function_type_return_type(ty));
      check_pattern_actuals(args,ty,u);
    }
    break;

  default:
    check_type_equal(f,type,check_pattern_actuals(args,infer_pattern_type(f),0));
    break;
  }
}



void check_default_type(Default d, Type t) {
  static Symbol underscore_symbol;
  static Def anon_def;
  static Direction dir;
  static Default deft;
  if (underscore_symbol == 0) {
    underscore_symbol = intern_symbol("_");
    anon_def = def(underscore_symbol,TRUE,FALSE);
    dir = direction(FALSE,FALSE,FALSE);
    deft = no_default();
  }

  switch (Default_KEY(d)) {
  case KEYno_default:
    break;
  case KEYsimple:
    check_expr_type(simple_value(d),t);
    break;
  case KEYcomposite:
    check_expr_type(composite_initial(d),t);
    {
      // We create a function type and then check the combiner:
      Declaration f1 = normal_formal(anon_def,t);
      Declaration f2 = normal_formal(anon_def,t);
      Declarations fs = append_Declarations(list_Declarations(f1),
					    list_Declarations(f2));
      Declaration rd = value_decl(anon_def,t,dir,deft);
      Type ft = function_type(fs,list_Declarations(rd));

      DECL_NEXT(f1) = f2;
      DECL_NEXT(f2) = 0;

      check_expr_type(composite_combiner(d),ft);
    }
    break;
  default:
    aps_error(d,"warning: not checking more complicated defaults");
    break;
  }
}

void check_matchers_type(Matches ms, Type t)
{
  switch (Matches_KEY(ms)) {
  case KEYlist_Matches:
    {
      Match m = list_Matches_elem(ms);
      check_pattern_type(matcher_pat(m),t);
      traverse_Block(do_typechecking,matcher_body(m),matcher_body(m));
    }
    break;
  case KEYappend_Matches:
    check_matchers_type(append_Matches_l1(ms),t);
    check_matchers_type(append_Matches_l2(ms),t);
    break;
  case KEYnil_Matches:
    break;
  }
}

Type infer_formal_type(Declaration decl)
{
  Type ty = formal_type(decl);
  if (Type_KEY(ty) == KEYno_type) {
    /* try context */
    void *parent = tnode_parent(decl);
    switch (ABSTRACT_APS_tnode_phylum(parent)) {
    case KEYPattern:
      /* must be pattern_var */
      ty = infer_pattern_type((Pattern)parent);
      break;
    case KEYDeclaration:
      /* must be for-in */
      ty = infer_element_type(for_in_stmt_seq(parent));
      break;
    case KEYExpression:
      /* must be "controlled" */
      ty = infer_element_type(controlled_set(parent));
      break;
    default:
      aps_error(decl,"Untyped formal!");
      break;
    }
    if (type_debug) {
      printf("inferred formal's type as ");
      print_Type(ty,stdout);
      printf("\n");
    }
  }
  return ty;
}

void print_TypeEnvironment(TypeEnvironment te, FILE *f)
{
  if (te) {
    Declarations tfs = te->type_formals;
    Declaration tf = first_Declaration(tfs);
    print_TypeEnvironment(te->outer,f);
    if (Declaration_KEY(te->source) == KEYpolymorphic) {
      Type *inferred = te->u.inferred;
      int started = FALSE;
    
      fputc('[',f);
      for (; tf; tf = DECL_NEXT(tf), ++inferred) {
        if (started) fputc(',',f); else started = TRUE;
        fprintf(f,"%s=",decl_name(tf));
        if (*inferred)
	  print_Type(*inferred,f);
        else
	  fputc('?',f);
      }
    } else {
      TypeActuals tacts = te->u.type_actuals;
      Type a;
      int started = FALSE;
      fprintf(f,"%s[",decl_name(te->source));
      for (a = first_TypeActual(tacts); a; a=TYPE_NEXT(a)) {
        if (started) fputc(',',f); else started = TRUE;
	print_Type(a,f);
      }
    }
    fputs("].",f);
  }
}

void print_Use(Use u, FILE *f) 
{ 
  Symbol sym;
  if (f == 0) f = stdout;
  switch (Use_KEY(u)) {
  case KEYuse: 
    sym = use_name(u);
    break;
  case KEYqual_use:
    print_Type(qual_use_from(u),f);
    fputc('$',f);
    sym = qual_use_name(u);
    break;
  }
  print_TypeEnvironment(USE_TYPE_ENV(u),f);
  fprintf(f,"%s",symbol_name(sym));
}

void print_value_decl(Declaration d, FILE *f)
{
  if (f == 0) f = stdout;
  fprintf(f,"%s:", decl_name(d));
  switch (Declaration_KEY(d)) {
  case KEYnormal_formal:
    print_Type(formal_type(d),f);
    break;
  case KEYseq_formal:
    print_Type(formal_type(d),f);
    fprintf(f,"...");
    break;
  default:
    print_Type(some_value_decl_type(d),f);
    break;
  }
}

void print_Signature(Signature s, FILE *f)
{
  if (f == 0) f = stdout;
  if (s == 0) {
    fprintf(f,"<null>");
    return;
  }
  switch (Signature_KEY(s)) {
  case KEYsig_use:
    print_Use(sig_use_use(s),f);
    break;
  case KEYsig_inst:
    print_Use(class_use_use(sig_inst_class(s)),f);
    fputc('[',f);
    {
      TypeActuals as = sig_inst_actuals(s);
      Type a;
      int started = FALSE;
      for (a = first_TypeActual(as); a; a=TYPE_NEXT(a)) {
	if (started) fputc(',',f); else started = TRUE;
	print_Type(a,f);
      }
    }
    fputc(']',f);
    break;
  case KEYno_sig:
    fprintf(f,"<nosig>");
    break;
  case KEYmult_sig:
    print_Signature(mult_sig_sig1(s),f);
    print_Signature(mult_sig_sig2(s),f);
    break;
  case KEYfixed_sig:
    fprintf(f,"{...}");
    /*! need to print the individual types:
     *! binding needs to put in TYPE_NEXT first.  No rush.
     **/
    break;
  }
}

void print_Type(Type t, FILE *f)
{
  if (f == 0) f = stdout;
  if (t == 0) {
    fprintf(f,"<null>");
    return;
  }
  switch (Type_KEY(t)) {
  case KEYtype_use:
    print_Use(type_use_use(t),f);
    break;
  case KEYprivate_type:
    fprintf(f,"private ");
    print_Type(private_type_rep(t),f);
    break;
  case KEYremote_type:
    fprintf(f,"remote ");
    print_Type(remote_type_nodetype(t),f);
    break;    
  case KEYfunction_type:
    fprintf(f,"function (");
    {
      Declarations fs = function_type_formals(t);
      Declarations rs = function_type_return_values(t);
      Declaration a, r;
      int started = FALSE;
      for (a = first_Declaration(fs); a; a=DECL_NEXT(a)) {
	if (started) fputc(',',f); else started = TRUE;
	print_value_decl(a,f);
      }
      fputc(')',f);
      started = FALSE;
      for (r = first_Declaration(rs); r; r=DECL_NEXT(r)) {
	if (started) fputc(',',f); else started = TRUE;
	print_value_decl(r,f);
      }
    }
    break;
  case KEYtype_inst:
    print_Use(module_use_use(type_inst_module(t)),f);
    fputc('[',f);
    {
      TypeActuals as = type_inst_type_actuals(t);
      Type a;
      int started = FALSE;
      for (a = first_TypeActual(as); a; a=TYPE_NEXT(a)) {
	if (started) fputc(',',f); else started = TRUE;
	print_Type(a,f);
      }
    }
    fputc(']',f);
    /*! need to print value_actuals too! */
    break;
  case KEYno_type:
    fprintf(f,"<notype>");
    break;
  default:
    fprintf(f,"????");
    break;
  }
}

int is_complete(TypeEnvironment type_env)
{
  if (type_env == 0) return TRUE;
  if (Declaration_KEY(type_env->source) == KEYpolymorphic) {
    int i;
    Declaration tformal = first_Declaration(type_env->type_formals);
    for (i=0; tformal !=0; ++i, tformal = DECL_NEXT(tformal))
      if (type_env->u.inferred[i] == 0) return FALSE;
  }
  return is_complete(type_env->outer);
}

Type use_from(Use u)
{
  switch (Use_KEY(u)) {
  case KEYqual_use:
    return qual_use_from(u);
  case KEYuse:
    {
      TypeEnvironment type_env = USE_TYPE_ENV(u);
      while (type_env && Declaration_KEY(type_env->source) == KEYpolymorphic)
	type_env = type_env->outer;
      if (type_env == 0) return 0;
      return make_type_use(0,type_env->result);
    }
  }
}

Type make_type_use(Use type_envs,Declaration d)
{
  Use u;
  Type fr = (type_envs != 0) ? use_from(type_envs) : 0;
  if (fr == 0) {
    u = use(def_name(declaration_def(d)));
    USE_DECL(u) = d;
    USE_TYPE_ENV(u) = 0;
  } else {
    u = qual_use(fr,def_name(declaration_def(d)));
    USE_DECL(u) = d;
    USE_TYPE_ENV(u) = USE_TYPE_ENV(type_envs);
  }
  return type_use(u);
}

Use type_envs_nested(Use type_envs)
{
  Type from = use_from(type_envs);
  if (from == 0) return 0;
  return type_use_use(from);
}

Signature sig_subst(Use type_envs, Signature sig)
{
  TypeEnvironment type_env = type_envs ? USE_TYPE_ENV(type_envs) : 0;
  if (type_envs == 0) return sig;
  if (type_env == 0) {
    switch (Use_KEY(type_envs)) {
    case KEYuse:
      return sig;
    case KEYqual_use:
      aps_error(type_envs,"very strange: a qual use without an environment!");
      return sig;
    }
  }
  if (type_debug) {
    printf("sig_subst(");
    print_Use(type_envs,stdout);
    printf(",");
    print_Signature(sig,stdout);
    printf(")\n");
  }
  switch (Signature_KEY(sig)) {
  case KEYsig_use:
    {
      Use u = sig_use_use(sig);
      Declaration decl = USE_DECL(u);
      if (!decl) return sig; /* an error actually */
      switch (Declaration_KEY(decl)) {
      case KEYsignature_renaming:
	sig = sig_subst(u,signature_renaming_old(decl));
	break;
      case KEYsignature_decl:
	sig = sig_subst(u,signature_decl_sig(decl));
	break;
      case KEYclass_decl:
	aps_error(sig,"neglected to provide actual types to class");
	break;
      default:
	aps_error(sig,"What sort of signature?");
	return sig;
      }
      return sig_subst(type_envs,sig);
    }
  case KEYno_sig :
    return sig;
  case KEYmult_sig :
    {
      Signature sig1 = mult_sig_sig1(sig);
      Signature sig2 = mult_sig_sig2(sig);
      Signature nsig1 = sig_subst(type_envs,sig1);
      Signature nsig2 = sig_subst(type_envs,sig2);
      if (sig1 == nsig1 && sig2 == nsig2) return sig;
      return mult_sig(nsig1,nsig2);
    }
  case KEYsig_inst :
    {
      TypeActuals tacts = sig_inst_actuals(sig);
      TypeActuals nacts = nil_TypeActuals();
      Type tact = first_TypeActual(tacts);
      if (!tact) return sig; /*! we might need to class_subst the class */
      nacts = nil_TypeActuals();
      while (tact) {
	nacts = xcons_TypeActuals(nacts,type_subst(type_envs,tact));
	tact = TYPE_NEXT(tact);
      }
      return sig_inst(sig_inst_is_input(sig),
		      sig_inst_is_var(sig),
		      sig_inst_class(sig),
		      nacts);
    }
  case KEYfixed_sig:
    /*!! can't handle yet. */
    return sig;
  }
}

/* We take a use as a holder of multiple type environments. */
Type type_subst(Use type_envs, Type ty)
{
  TypeEnvironment type_env = type_envs ? USE_TYPE_ENV(type_envs) : 0;
  if (type_envs == 0) return ty;
  if (type_env == 0) {
    switch (Use_KEY(type_envs)) {
    case KEYuse:
      return ty;
    case KEYqual_use:
      aps_error(type_envs,"very strange: a qual use without an environment!");
      return ty;
    }
  }
  if (type_debug) {
    printf("type_subst(");
    print_Use(type_envs,stdout);
    printf(",");
    print_Type(ty,stdout);
    printf(")\n");
  }
  switch (Type_KEY(ty)) {
  case KEYno_type:
  case KEYtype_inst:
  case KEYprivate_type:
    aps_error(ty,"new/private/inst types are not copyable (internal error)");
    return ty;
  case KEYremote_type:
    ty =  remote_type(type_subst(type_envs,remote_type_nodetype(ty)));
    break;
  case KEYfunction_type:
    {
      Declarations formals = function_type_formals(ty);
      Declarations new_formals = nil_Declarations();
      Declarations rdecls = function_type_return_values(ty);
      Declarations new_rdecls = nil_Declarations();
      Declaration f,r, sf=0;
      for (f = first_Declaration(formals); f; f=DECL_NEXT(f)) {
	Type fty = formal_type(f);
	Type new_fty = type_subst(type_envs,fty);
	Declaration new_f;
	/* We have to copy formals because of
	 * NEXT_DECL
	 */
	int is_seq = (Declaration_KEY(f) == KEYseq_formal);
	Def def = formal_def(f);
	new_f = is_seq ? seq_formal(def,new_fty)
	  : normal_formal(def,new_fty);
	new_formals = xcons_Declarations(new_formals,new_f);
	if (sf != 0) DECL_NEXT(sf) = new_f;
	sf = new_f;
      }
      if (sf != 0) DECL_NEXT(sf) = 0;
      /*! Fix for DECL_NEXT for multiple return values */
      for (r = first_Declaration(rdecls); r; r=DECL_NEXT(r)) {
	Type rty = value_decl_type(r);
	Type new_rty = type_subst(type_envs,rty);
	Declaration new_r;
	if (rty == new_rty) {
	  new_r = r;
	} else {
	  Def def = value_decl_def(r);
	  Direction dir = value_decl_direction(r);
	  Default init = value_decl_default(r);
	  new_r = value_decl(def,new_rty,dir,init);
	}
	new_rdecls = xcons_Declarations(new_rdecls,new_r);
      }
      ty = function_type(new_formals,new_rdecls);
    }
    break;
  case KEYtype_use:
    {
      Use u = type_use_use(ty);
      Declaration tdecl = USE_DECL(u);
      TypeEnvironment type_env2 = USE_TYPE_ENV(u);
      void *parent = tdecl ? tnode_parent(tdecl) : 0;
      Declarations formals;
      TypeActuals actuals;
      Declaration f;
      Type a;
      if (tdecl == 0) return ty;
      if (type_env2 != 0) {
	Type from = type_subst(type_envs,use_from(u));
	type_envs = type_use_use(from);
	type_env = USE_TYPE_ENV(type_envs);
      }
      while (parent != 0 &&
	     ABSTRACT_APS_tnode_phylum(parent) != KEYDeclaration)
	parent = tnode_parent(parent);
      if (parent == 0) {
	if (type_env2 != 0)
	  aps_error(ty,"global type with environment? (internal error)");
	return ty;
      }
      while (type_env != 0 && type_env->source != parent)
	type_env = type_env->outer;
      if (type_env == 0) {
	/*
	 * ? It seems that returning 'ty' is correct here.
	print_Use(type_envs,stdout);
	printf(" used for ");
	print_Type(ty,stdout);
	printf("\n");
	aps_error(ty,"qualification fetches outer type ? (internal error)");
	*/
	return ty;
      }
      switch (Declaration_KEY(tdecl)) {
      case KEYsome_type_formal:
	if (Declaration_KEY(type_env->source) != KEYclass_decl ||
	    tdecl != class_decl_result_type(type_env->source))
	{
	  int i = Declaration_info(tdecl)->instance_index;
	  if (Declaration_KEY((Declaration)parent) == KEYpolymorphic) {
	    Type t = type_env->u.inferred[i];
	    if (t == 0) {
	      aps_error(ty,"type not fully inferred");
	      aps_error(type_envs,"or perhaps here");
	    }
	    if (type_debug) {
	      printf("(I)=> ");
	      print_Type(t,stdout);
              printf("\n");
	    }
	    return t;
	  }
	  actuals = type_env->u.type_actuals;
	  for (a = first_TypeActual(actuals); a!=0 && i; a=TYPE_NEXT(a), --i)
	    ;
	  if (!a) {
	    aps_error(type_env->result,"too few type parameters");
	    printf("looking for actual for %s on line %d\n",
		   decl_name(tdecl), tnode_line_number(tdecl));
	  } else {
	    ty = type_subst(type_envs_nested(type_envs),a);
	    if (type_debug) {
	      printf("(F)=> ");
	      print_Type(ty,stdout);
              printf("\n");
	    }
	    return ty;
	  }
	  break;
	}
        /* FALL THROUGH */
      case KEYsome_type_decl:
	switch (Declaration_KEY(type_env->source)) {
	case KEYsome_class_decl:
	  if (some_class_decl_result_type((Declaration)parent) == tdecl) {
	    ty = type_subst(type_envs_nested(type_envs),
			    make_type_use(0,type_env->result));
	    if (type_debug) {
	      printf("(R)=> ");
	      print_Type(ty,stdout);
              printf("\n");
	    }
            return ty;
          }
	  break;
	default:
	  aps_error(parent,"not a class decl? (internal error)");
	  break;
	}
	/* FALL THROUGH */
      default:
	break;
      }
      ty = make_type_use(type_envs,tdecl);
      break;
    }
  }
  if (type_debug) {
    printf("=> ");
    print_Type(ty,stdout);
    printf("\n");
  }
  return ty;
}

int module_decl_generating(Declaration m)
{
  struct Declaration_info *info = Declaration_info(m);
  if (info->decl_flags & MODULE_DECL_GENERATING_VALID) {
    /*
    printf("%s %s\n",
	   decl_name(m),
	   (info->decl_flags & MODULE_DECL_GENERATING) ? 
	   "is generating" : "is not generating");
    */
    return !!(info->decl_flags & MODULE_DECL_GENERATING);
  }
  // avoid infinite recursion for recursive modules
  info->decl_flags |= MODULE_DECL_GENERATING|MODULE_DECL_GENERATING_VALID;
  Declaration rd = module_decl_result_type(m);
  Type ty = some_type_decl_type(rd);

  for (;;) {
    switch (Type_KEY(ty)) {
    case KEYremote_type:
      ty = remote_type_nodetype(ty);
      break;
    case KEYno_type:
    case KEYprivate_type:
      return 1;
    case KEYtype_inst:
      {
	Declaration m2 = USE_DECL(module_use_use(type_inst_module(ty)));
	if (module_decl_generating(m2)) return 1;
	Declaration rd2 = module_decl_result_type(m2);
	Declarations tfs2 = module_decl_type_formals(m2);
	Type ty2 = some_type_decl_type(rd2);
	int loop_again = 0;
	switch (Type_KEY(ty2)) {
	default: break;
	case KEYtype_use:
	  {
	    Declaration td = USE_DECL(type_use_use(ty2));
	    Declaration tf2 = first_Declaration(tfs2);
	    Type ta2 = first_TypeActual(type_inst_type_actuals(ty));
	    while (tf2 && !loop_again) {
	      if (tf2 == td) {
		ty = ta2;
		loop_again = 1;
	      }
	      tf2 = DECL_NEXT(tf2);
	      ta2 = TYPE_NEXT(ta2);
	    }
	  }
	}
	if (loop_again) break;
      }
      /*FALLTHROUGH*/
    case KEYfunction_type:
    case KEYtype_use:
      info->decl_flags &= ~MODULE_DECL_GENERATING;
      return 0;
    }
  }
}

Type type_inst_base(Type t)
{
  Use mu = module_use_use(type_inst_module(t));
  Declaration mdecl = USE_DECL(mu);
  Declaration etd = module_decl_result_type(mdecl);
  Type et = some_type_decl_type(etd);
  Declarations fs = module_decl_type_formals(mdecl);
  TypeActuals as = type_inst_type_actuals(t);

  while (Type_KEY(et) == KEYremote_type) {
    et = remote_type_nodetype(et);
  }
  if (Type_KEY(et) == KEYtype_inst) {
    et = type_inst_base(et);
  }

  switch (Type_KEY(et)) {
  case KEYno_type:
  case KEYprivate_type:
  case KEYfunction_type:
  case KEYremote_type: // shouldn't happen
  case KEYtype_inst: // shouldn't happen
    return et;
  case KEYtype_use:
    if (Use_KEY(type_use_use(et)) == KEYqual_use) return et; //TODO
    {
      Declaration etud = USE_DECL(type_use_use(et));
      Declaration f;
      Type a;
      for (f = first_Declaration(fs), a = first_TypeActual(as);
	   f && a; f = DECL_NEXT(f), a = TYPE_NEXT(a))
	if (f == etud) return a;
      return et;
    }
    break;
  }
}
   
Use type_inst_envs(Type ty)
{
  Use mu = module_use_use(type_inst_module(ty));
  Declaration mdecl = USE_DECL(mu);
  Declaration rdecl = module_decl_result_type(mdecl);
  Def rdef = some_type_decl_def(rdecl);
  TypeEnvironment te =
    (TypeEnvironment)HALLOC(sizeof(struct TypeContour));
  extern int aps_yylineno;
  Declaration tdecl = (Declaration)tnode_parent(ty);
  Use tu;
  Use u;

  while (ABSTRACT_APS_tnode_phylum(tdecl) != KEYDeclaration)
    tdecl = (Declaration)tnode_parent(tdecl);
  tu = use(def_name(some_type_decl_def(tdecl)));
  te->outer = USE_TYPE_ENV(mu);
  te->source = mdecl;
  te->type_formals = module_decl_type_formals(mdecl);
  te->result = (Declaration)tnode_parent(ty);
  te->u.type_actuals = type_inst_type_actuals(ty);
  aps_yylineno = tnode_line_number(ty);
  USE_DECL(tu) = tdecl;
  USE_TYPE_ENV(tu) = 0;
  u = qual_use(type_use(tu),def_name(rdef));
  USE_TYPE_ENV(u) = te;
  USE_DECL(u) = rdecl;
  return u;
}

Type base_type(Type ty)
{
  switch (Type_KEY(ty)) {
  case KEYfunction_type:
  case KEYprivate_type:
  case KEYno_type:
    break;
  case KEYremote_type:
    ty = base_type(remote_type_nodetype(ty));
    break;
  case KEYtype_use:
    {
      Use u = type_use_use(ty);
      Declaration tdecl = USE_DECL(u);
      if (tdecl) switch (Declaration_KEY(tdecl)) {
      case KEYsome_type_formal:
	break;
      case KEYtype_renaming:
	ty = base_type(type_subst(u,type_renaming_old(tdecl)));
	break;
      case KEYsome_type_decl:
	{
	  Type t = some_type_decl_type(tdecl);
	  switch (Type_KEY(t)) {
	  case KEYno_type:
	  case KEYprivate_type:
	    return ty;
	    break;
	  case KEYtype_inst:
	    {
	      Use mu = module_use_use(type_inst_module(t));
	      Declaration mdecl = USE_DECL(mu);
	      if (module_decl_generating(mdecl)) return ty;
	      return base_type(type_subst(u,base_type(t)));
	    }
	    break;
	  default:
	    ty = base_type(type_subst(u,t));
	    break;
	  }
	}
	break;
      default:
	aps_error(tdecl,"unknown type decl");
      }
    }
    break;
  case KEYtype_inst:
    {
      Declaration mdecl = USE_DECL(module_use_use(type_inst_module(ty)));
      if (module_decl_generating(mdecl)) {
	return ty;
      }
      Declaration etd = module_decl_result_type(mdecl);
      Type et = type_subst(type_inst_envs(ty),
			   base_type(some_type_decl_type(etd)));
      return base_type(et);
    }
  }
  return ty;
}

int remote_type_p(Type ty)
{
  switch (Type_KEY(ty)) {
  case KEYfunction_type:
  case KEYprivate_type:
  case KEYtype_inst:
  case KEYno_type:
    return FALSE;
    break;
  case KEYremote_type:
    return TRUE;
    break;
  case KEYtype_use:
    {
      Use u = type_use_use(ty);
      Declaration tdecl = USE_DECL(u);
      if (tdecl) switch (Declaration_KEY(tdecl)) {
      case KEYtype_formal:
	return TRUE;
      case KEYphylum_formal:
	return FALSE;
      case KEYtype_renaming:
	return remote_type_p(type_renaming_old(tdecl));
	break;
      case KEYsome_type_decl:
	return remote_type_p(some_type_decl_type(tdecl));
      default:
	aps_error(tdecl,"unknown type decl");
      }
    }
    break;
  }
  return FALSE;
}

static int declarations_type_equal(Declarations ds1, Declarations ds2)
{
  Declaration d1, d2;
  for (d1 = first_Declaration(ds1), d2 = first_Declaration(ds2);
       d1 && d2; d1 = DECL_NEXT(d1), d2 = DECL_NEXT(d2)) {
    if (Declaration_KEY(d1) != Declaration_KEY(d2))
      return FALSE;
    if (!base_type_equal(base_type(some_value_decl_type(d1)),
			 base_type(some_value_decl_type(d2))))
      return FALSE;
  }
  return d1 == 0 && d2 == 0;
}

/*! This function will fail to terminate in the case of recursive types */
int base_type_equal(Type b1, Type b2) {
  if (b1 == b2) return TRUE;
  if (Type_KEY(b1) != Type_KEY(b2)) return FALSE;
  switch (Type_KEY(b1)) {
  case KEYprivate_type:
  case KEYremote_type:
  case KEYtype_inst:
  case KEYno_type:
    return FALSE;
  case KEYfunction_type:
    return declarations_type_equal(function_type_formals(b1),
				   function_type_formals(b2)) &&
	   declarations_type_equal(function_type_return_values(b1),
				   function_type_return_values(b2));
  case KEYtype_use:
    {
      Use u1 = type_use_use(b1);
      Use u2 = type_use_use(b2);
      return (USE_DECL(u1) == USE_DECL(u2) &&
	      /*? perhaps too strict */
	      USE_TYPE_ENV(u1) == USE_TYPE_ENV(u2));
    }
  }
}

void check_type_equal(void *node, Type t1, Type t2) {
  if (t1 == t2) return; /* easy case */
  if (t1 == 0 || t2 == 0) return;
  {
    Type b1 = base_type(t1);
    Type b2 = base_type(t2);
    if (!base_type_equal(b1,b2)) {
      print_Type(b1,stderr); fprintf(stderr, " != ");
      print_Type(b2,stderr); fprintf(stderr, "\n");
      
      aps_error(node,"type mismatch");
      fprintf(stderr,"  ");
      print_Type(t1,stderr);
      fprintf(stderr," != ");
      print_Type(t2,stderr);
      fprintf(stderr,"\n");
    }
  }
}

static int check_declarations_type_subst
  (void *node, Declarations ds1, Use type_envs, Declarations ds2)
{
  Declaration d1, d2;
  for (d1 = first_Declaration(ds1), d2 = first_Declaration(ds2);
       d1 && d2; d1 = DECL_NEXT(d1), d2 = DECL_NEXT(d2)) {
    if (Declaration_KEY(d1) != Declaration_KEY(d2)) {
      aps_error(node,"declaration kind mismatch");
      return -1;
    }
    check_type_subst(node,some_value_decl_type(d1),
		     type_envs,some_value_decl_type(d2));
  }
  if (d1 != 0 || d2 != 0)
    aps_error(node,"type mismatch: function formal number"); /* probably */
  return 0;
}

void check_type_signatures(void *node, Type t, Use type_envs, Signature sig)
{
  /*! Horrible hack!  -- only used for element type signatures*/
  switch (Signature_KEY(sig)) {
  case KEYno_sig:
    break;
  case KEYfixed_sig:
    /*! check membership! */
    break;
  case KEYmult_sig:
    check_type_signatures(node,t,type_envs,mult_sig_sig1(sig));
    check_type_signatures(node,t,type_envs,mult_sig_sig2(sig));
    break;
  case KEYsig_inst:
    /*! Avert your eyes! */
    if (first_TypeActual(sig_inst_actuals(sig)))
    {
      Type t1 = type_element_type(t);
      Type t2 = sig_element_type(sig);
      if (t1 == error_type) {
        aps_error(node,"  used here");
      }
      check_type_subst(node,t1,type_envs,t2);
    }
    break;
  case KEYsig_use:
    /*! check membership! */
    break;
  }
}

void check_type_subst(void *node, Type t1, Use type_envs, Type t2)
{
  TypeEnvironment type_env;
  if (type_envs == 0 || is_complete(type_env = USE_TYPE_ENV(type_envs))) {
    check_type_equal(node,t1,type_subst(type_envs,t2));
    return;
  }
  if (type_debug) {
    printf("unify("); print_Type(t1,stdout); printf(",");
    print_Use(type_envs,stdout); printf(" "); print_Type(t2,stdout);
    printf(")\n");
  }
  switch (Type_KEY(t2)) {
  case KEYremote_type:
    check_type_subst(node,t1,type_envs,remote_type_nodetype(t2));
    break;
  case KEYprivate_type:
  case KEYno_type:
  case KEYtype_inst:
    aps_error(node,"cannot match private/new/inst type");
    break;
  case KEYfunction_type:
    t1 = base_type(t1);
    if (Type_KEY(t1) != KEYfunction_type) {
      aps_error(node,"function type mismatch");
    } else {
      check_declarations_type_subst(node,function_type_formals(t1),
				    type_envs,function_type_formals(t2));
      check_declarations_type_subst(node,function_type_return_values(t1),
				    type_envs,function_type_return_values(t2));
    }
    break;
  case KEYtype_use:
    {
      Use u = type_use_use(t2);
      Declaration tdecl = USE_DECL(u);
      TypeEnvironment type_env2 = USE_TYPE_ENV(u);
      void *parent = tdecl ? tnode_parent(tdecl) : 0;
      Declarations formals;
      TypeActuals actuals;
      Declaration f;
      Type a;
      if (type_env2 != 0) {	
	Type from = type_subst(type_envs,use_from(u));
	type_envs = type_use_use(from);
	type_env = USE_TYPE_ENV(type_envs);
      }
      while (parent && ABSTRACT_APS_tnode_phylum(parent) != KEYDeclaration)
	parent = tnode_parent(parent);
      if (parent == 0) {
	if (type_env2 != 0)
	  aps_error(t2,"global type with environment? (internal error)");
	check_type_equal(node,t1,t2);
	return;
      }
      while (type_env != 0 && type_env->source != parent)
	type_env = type_env->outer;
      if (type_env == 0) {
	/*
	 * See comment in type_subst
	aps_error(t2,"qualification fetches outer type ? (internal error)");
	*/
	check_type_equal(node,t1,t2);
      }
      switch (Declaration_KEY(tdecl)) {
      case KEYsome_type_formal:
	if (Declaration_KEY(type_env->source) != KEYclass_decl ||
	    tdecl != class_decl_result_type(type_env->source))
	{
	  int i = Declaration_info(tdecl)->instance_index;
	  if (Declaration_KEY((Declaration)parent) == KEYpolymorphic) {
	    Type t = type_env->u.inferred[i];
	    if (t == 0) {
	      type_env->u.inferred[i] = t1;
	      check_type_signatures(node,t1,type_envs,some_type_formal_sig(tdecl));
	    } else {
	      check_type_equal(node,t1,type_subst(type_envs_nested(type_envs),t));
	    }
	  } else {
	    actuals = type_env->u.type_actuals;
	    for (a = first_TypeActual(actuals); a!=0 && i; a=TYPE_NEXT(a), --i)
	      ;
	    if (!a) {
	      aps_error(type_env->result,"too few type parameters");
	      printf("looking for actual for %s on line %d\n",
		     decl_name(tdecl), tnode_line_number(tdecl));
	    } else {
	      check_type_equal(node,t1,type_subst(type_envs_nested(type_envs),a));
	    }
	  }
	  return;
	}
	/* FALL THROUGH */
      case KEYsome_type_decl:
	switch (Declaration_KEY((Declaration)parent)) {
	KEYsome_class_decl:
	  if (some_class_decl_result_type((Declaration)parent) == tdecl) {
	    check_type_equal(node,t1,type_subst(type_envs_nested(type_envs),
						make_type_use(0,type_env->result)));
	    return;
	  }
	  break;
	default:
	  break;
	}
	/* FALL THROUGH */
      default:
	break;
      }
      check_type_equal(node,t1,make_type_use(type_envs,tdecl));
      break;
    }
    break;
  }
}


