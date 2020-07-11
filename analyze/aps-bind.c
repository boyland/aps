#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jbb-alloc.h"
#include "aps-ag.h"

int bind_debug;

int decl_namespaces(Declaration d) {
  switch (Declaration_KEY(d)) {
  case KEYclass_decl : /* fall through */
  case KEYclass_renaming: return NAME_SIGNATURE;
  case KEYmodule_decl : /* fall through */
  case KEYmodule_renaming: return NAME_TYPE|NAME_SIGNATURE;
  case KEYsignature_decl : /* fall through */
  case KEYsignature_renaming: return NAME_SIGNATURE;
  case KEYtype_decl: /* fall through */
  case KEYphylum_decl: /* fall through */
  case KEYtype_formal: /* fall through */
  case KEYphylum_formal: /* fall through */
  case KEYtype_renaming: return NAME_TYPE;
  case KEYpattern_decl: /* fall through */
  case KEYpattern_renaming: return NAME_PATTERN;
  case KEYconstructor_decl: return NAME_PATTERN|NAME_VALUE;
  case KEYattribute_decl: /* fall through */
  case KEYfunction_decl: /* fall through */
  case KEYprocedure_decl: /* fall through */
  case KEYvalue_decl: /* fall through */
  case KEYvalue_renaming: /* fall through */
  case KEYformal: return NAME_VALUE;
  default:
  return 0;
  }
}

struct env_item {
  struct env_item *next;
  Declaration decl;
  TypeEnvironment type_env;
  Symbol name;
  int namespaces;
};
typedef struct env_item *SCOPE;

static Declaration module_TYPE;
static Declaration module_PHYLUM;

static TypeEnvironment current_type_env = 0;

static void push_type_contour(Declaration d, TypeActuals tacts, Declaration tdecl) {
  TypeEnvironment new_type_env =
    (TypeEnvironment)HALLOC(sizeof(struct TypeContour));
  new_type_env->outer = current_type_env;
  new_type_env->source = d;
  new_type_env->result = tdecl;
  switch (Declaration_KEY(d)) {
  case KEYsome_class_decl:
    new_type_env->type_formals = some_class_decl_type_formals(d);
    new_type_env->u.type_actuals = tacts;
    break;
  case KEYpolymorphic:
    new_type_env->type_formals = polymorphic_type_formals(d);
    new_type_env->u.inferred = 0; /* instantiate at each use */
    break;
  default:
    fatal_error("push_type_contour called with bad declaration");
  }
  current_type_env = new_type_env;
}

static void set_instance_index(Declarations ds)
{
  int i;
  Declaration formal = first_Declaration(ds);
  for (i=0; formal != 0; ++i, formal=DECL_NEXT(formal)) {
    // printf("Indx of %s = %d\n",decl_name(formal),i);
    Declaration_info(formal)->instance_index = i;
  }
}


static void pop_type_contour()
{
  if (current_type_env != 0)
    current_type_env = current_type_env->outer;
  else
    fatal_error("too many pop_type_contour");
}

/* instantiate polymorphic uses with space for inferred types */
TypeEnvironment instantiate_type_env(TypeEnvironment form)
{
  if (form != 0 && Declaration_KEY(form->source) == KEYpolymorphic) {
    TypeEnvironment ins =
      (TypeEnvironment)HALLOC(sizeof(struct TypeContour));
    Declarations tfs = form->type_formals;
    Declaration tf;
    int n=0;
    int i;

    ins->outer = instantiate_type_env(form->outer);
    ins->source = form->source;
    ins->type_formals = tfs;
    for (tf=first_Declaration(tfs); tf != NULL; tf=DECL_NEXT(tf))
      ++n;
    /* allocate one more than necessary */
    ins->u.inferred = (Type*)HALLOC((n+1)*sizeof(Type));
    for (i=0; i <= n; ++i)
      ins->u.inferred[i] = 0;
    return ins;
  }
  return form;
}

static SCOPE add_env_item(SCOPE old, Declaration d) {
  static Symbol underscore_symbol = 0;
  if (underscore_symbol == 0) {
    underscore_symbol = intern_symbol("_");
  }
  switch (Declaration_KEY(d)) {
  case KEYdeclaration:
    {
      Symbol name = def_name(declaration_def(d));
      if (name == NULL || name == underscore_symbol) {
	return old;
      } else {
	SCOPE new_entry=(SCOPE)HALLOC(sizeof(struct env_item));
	new_entry->next = old;
	new_entry->decl = d;
	new_entry->name = name;
	new_entry->namespaces = decl_namespaces(d);
	new_entry->type_env = current_type_env;
	/* check to see if already declared */
	while (old && old->type_env == current_type_env) {
	  if (old->name == name &&
	      (old->namespaces & new_entry->namespaces) != 0) {
	    aps_error(d,"Duplicate declaration (previously seen line %d)",
		      tnode_line_number(old->decl));
	  }
	  old = old->next;
	}
	return new_entry;
      }
    }
  }
  return old;
}

static void *get_bindings(void *, void *);
static void *get_public_bindings(void *, void *);
static void *do_bind(void *, void *);
static void *set_next_fields(void *, void *);
static void *activate_pragmas(void *, void *);

static SCOPE add_ext_sig(SCOPE old, Declaration tdecl, Signature sig) {
  switch (Signature_KEY(sig)) {
  case KEYno_sig: return old;
  case KEYsig_use:
  {
    Declaration sig_use_decl = USE_DECL(sig_use_use(sig));
    switch (Declaration_KEY(sig_use_decl))
    {
    case KEYsome_class_decl:
      aps_error(sig, "Missing actuals for %s in the signature", decl_name(sig_use_decl));
      break;
    }
    break;
  }
  case KEYmult_sig:
    return add_ext_sig(add_ext_sig(old,tdecl,mult_sig_sig1(sig)),tdecl,
		       mult_sig_sig2(sig));
  case KEYsig_inst:
    /*! ignore is_input and is_var */
    { Class cl = sig_inst_class(sig);
      switch (Class_KEY(cl)) {
      case KEYclass_use:
	{ Declaration d = Use_info(class_use_use(cl))->use_decl;
	  if (d == NULL) fatal_error("%d: class not found",
				     tnode_line_number(cl));
	  switch (Declaration_KEY(d)) {
	  case KEYsome_class_decl:
	    { SCOPE new_scope = old;
	      TypeEnvironment saved = current_type_env;
	      current_type_env = USE_TYPE_ENV(class_use_use(cl));
	      push_type_contour(d,sig_inst_actuals(sig),tdecl);
	      traverse_Block(get_public_bindings,&new_scope,
			     some_class_decl_contents(d));
	      pop_type_contour(); /* not actually necessary */
	      current_type_env = saved;
	      return new_scope; }
	  default:
	    /*! eventually handle renamings */
	    fatal_error("%d: complicated binding for class",
			tnode_line_number(cl));
	  }
	}
      default:
	fatal_error("%d: bad binding for class",
		    tnode_line_number(cl));
      }
    }
  default:
    fatal_error("%d: signature too complicated",tnode_line_number(sig));
  }
  return old;
}

void bind_Program(Program p) {
  void *top_mark = SALLOC(0);
  SCOPE scope=NULL;
  char *saved_filename = aps_yyfilename;
  Program basic_program = find_Program(make_string("basic"));
  TypeEnvironment saved_type_env = current_type_env;

  if (PROGRAM_IS_BOUND(p)) return;
  Program_info(p)->program_flags |= PROGRAM_BOUND_FLAG;

  current_type_env = 0;
  set_tnode_parent(p);
  traverse_Program(set_next_fields,/* any nonnull value */p,p);
  
  if (basic_program != p) {
    bind_Program(basic_program);
    traverse_Program(get_public_bindings,&scope,basic_program);
  }
  traverse_Program(get_bindings,&scope,p);
  aps_yyfilename = (char *)program_name(p);
  traverse_Program(do_bind,scope,p);
  release(top_mark);
  traverse_Program(activate_pragmas,/* any nonnull value */p,p);
  aps_yyfilename = saved_filename;
  current_type_env = saved_type_env;
}

static SCOPE bind_Declaration(SCOPE scope, Declaration decl) {
  SCOPE new_scope = scope;
  traverse_Declaration(get_bindings,&new_scope,decl);
  traverse_Declaration(do_bind,new_scope,decl);
  return new_scope;
}

static SCOPE bind_Declarations(SCOPE scope, Declarations decls) {
  SCOPE new_scope = scope;
  traverse_Declarations(get_bindings,&new_scope,decls);
  traverse_Declarations(do_bind,new_scope,decls);
  return new_scope;
}

static SCOPE signature_services(Declaration tdecl, Signature sig, SCOPE services);

static SCOPE inst_services(TypeEnvironment use_type_env,
			   Declaration class_decl,
			   Declaration tdecl, /* type declaration */
			   TypeActuals tacts,
			   SCOPE services)
{
  TypeEnvironment saved = current_type_env;
  Declaration tf;
  /* Signature psig = some_class_decl_parent(class_decl); */
  current_type_env = use_type_env;
  push_type_contour(class_decl,tacts,tdecl);
  /*!! HACK: needs to be fixed to handle extension correctly */
  for (tf = first_Declaration(some_class_decl_type_formals(class_decl));
       tf ; tf = DECL_NEXT(tf)) {
    if (TYPE_FORMAL_IS_EXTENSION(tf)) {
      /*! This won't work -- the environment is lost */
      services = signature_services(tdecl,type_formal_sig(tf),services);
    }
  }
  /*! Don't do this:
   *! the services don't work correctly anyway (see above)
   *! it's better to see what is actually available.
   *
   * services = signature_services(tdecl,psig,services); 
   */
  traverse_Block(get_public_bindings,&services,
		 some_class_decl_contents(class_decl));  
  pop_type_contour(); /* not actually necessary */
  current_type_env = saved;
  return services;
}

static SCOPE signature_services(Declaration tdecl, Signature sig, SCOPE services)
{
  switch (Signature_KEY(sig)) {
  case KEYsig_use:
     /*! ignoring the use_type_env is wrong */
    {
      Declaration sdecl = USE_DECL(sig_use_use(sig));
      switch (Declaration_KEY(sdecl)) {
      case KEYsignature_decl:
	return signature_services(tdecl,signature_decl_sig(sdecl),services);
      case KEYsignature_renaming:
	return signature_services(tdecl,signature_renaming_old(sdecl),services);
      default:
	aps_error(sig,"not sure what sort of sig this is");
	break;
      }
    }
    break;
  case KEYsig_inst:
    {
      Use u = class_use_use(sig_inst_class(sig));
      Declaration cdecl = USE_DECL(u);
      if (cdecl)
	return inst_services(USE_TYPE_ENV(u),cdecl,tdecl,sig_inst_actuals(sig),
			     services);
    }
  case KEYmult_sig:
    return signature_services(tdecl,mult_sig_sig2(sig),
			      signature_services(tdecl,mult_sig_sig1(sig),
						 services));
  case KEYfixed_sig:
  case KEYno_sig:
    break;
  }
  return services;
}

/* return list of things visible to this type */
static SCOPE type_services(Type t)
{
  SCOPE services = (SCOPE)Type_info(t)->binding_temporary;
  if (services != 0) return services;
  switch (Type_KEY(t)) {
  case KEYtype_use:
    /*? I think ignore USE_TYEP_ENV is OK
     *? since we will keep this information around
     *? for later
     */
    {
      Declaration tdecl = USE_DECL(type_use_use(t));
      if (tdecl == 0) {
	return 0;
      }
      switch (Declaration_KEY(tdecl)) {
      case KEYtype_decl:
	{
	  Type ty = type_decl_type(tdecl);
	  if (Type_KEY(ty) == KEYno_type &&
	      !Type_info(ty)->binding_temporary) {
	    services = inst_services(0,module_TYPE,tdecl,0,services);
	    Type_info(ty)->binding_temporary = services;
	  } else if (Type_KEY(ty) == KEYtype_inst &&
		     !Type_info(ty)->binding_temporary) {
	    Use u = module_use_use(type_inst_module(ty));
	    Declaration mdecl = USE_DECL(u);
	    if (mdecl)
	      services = inst_services(USE_TYPE_ENV(u),mdecl,tdecl,
				       type_inst_type_actuals(ty),services);
	    Type_info(ty)->binding_temporary = services;
	  } else {
	    services = type_services(ty);
	  }
	}
	break;
      case KEYphylum_decl:
	{
	  Type ty = phylum_decl_type(tdecl);
	  if (Type_KEY(ty) == KEYno_type &&
	      !Type_info(ty)->binding_temporary) {
	    services = inst_services(0,module_PHYLUM,tdecl,0,services);
	    Type_info(ty)->binding_temporary = services;
	  } else if (Type_KEY(ty) == KEYtype_inst &&
		     !Type_info(ty)->binding_temporary) {
	    Use u = module_use_use(type_inst_module(ty));
	    Declaration mdecl = USE_DECL(u);
	    services = inst_services(USE_TYPE_ENV(u),mdecl,tdecl,
				     type_inst_type_actuals(ty),services);
	    Type_info(ty)->binding_temporary = services;
	  } else {
	    services = type_services(ty);
	  }
	}
	break;
      case KEYtype_renaming:
	services = type_services(type_renaming_old(tdecl));
	break;
      case KEYsome_type_formal:
	services = signature_services(tdecl,some_type_formal_sig(tdecl),services);
	break;
      default:
	aps_error(t,"not sure what sort of type this is");
	break;
      }
    }
    break;
  case KEYtype_inst:
    aps_error(t,"MODULE[...] must be := assigned in a type declaration");
    break;
  case KEYremote_type:
    services = type_services(remote_type_nodetype(t));
    break;
  case KEYprivate_type:
    {
      extern void print_Type(Type,FILE*);
      printf("%d: getting services of ",tnode_line_number(t));
      print_Type(t,stdout);
      putc('\n',stdout);
    }
    services = type_services(private_type_rep(t));
    break;
  case KEYfunction_type:
  case KEYno_type:
    break;
  }
  Type_info(t)->binding_temporary = services;
  return services;
}

static void *do_bind(void *vscope, void *node);

static void bind_Use_by_name(Use u, Symbol name, int namespaces, SCOPE scope)
{
  while (scope != NULL) {
    if (scope->name == name && (scope->namespaces&namespaces) != 0) {
      Use_info(u)->use_decl = scope->decl;
      Use_info(u)->use_type_env = instantiate_type_env(scope->type_env);
      return;
    }
    scope=scope->next;
  }
}

static void bind_Use(Use u, int namespaces, SCOPE scope) {
  switch (Use_KEY(u)) {
  case KEYuse:
    bind_Use_by_name(u,use_name(u),namespaces,scope);
    break;
  case KEYqual_use:
    do_bind(scope,qual_use_from(u));
    {
      SCOPE s = type_services(qual_use_from(u));
      bind_Use_by_name(u,qual_use_name(u),namespaces,s);
      if (USE_DECL(u) == 0) {
	SCOPE t;
	int started = FALSE;
	fprintf(stderr,"  services = [");
	for (t=s; t != NULL; t=t->next) {
	  if (started) fputc(',',stderr); else started = TRUE;
	  fprintf(stderr,"%s",symbol_name(t->name));
	}
	fprintf(stderr,"]\n");
      }
    }
    break;
  }
}

static void *get_bindings(void *scopep, void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration d=(Declaration)node;
      *(SCOPE *)scopep = add_env_item(*(SCOPE *)scopep,d);
      if (Declaration_KEY(d) == KEYpolymorphic) {
	push_type_contour(d,0,0);
	traverse_Block(get_bindings,scopep,polymorphic_body(d));
	pop_type_contour();
      }
      return NULL; }
  case KEYUnit:
    { Unit u=(Unit)node;
      switch (Unit_KEY(u)) {
      case KEYwith_unit:
	{ char *str = (char *)with_unit_name(u);
	  char *prefix = HALLOC(strlen(str));
	  Program p;
	  strcpy(prefix,str+1);
	  prefix[strlen(prefix)-1] = '\0';
	  p = find_Program(make_string(prefix));
	  bind_Program(p);
	  traverse_Program(get_public_bindings,scopep,p);
	}
      }
    }
  }
  return scopep;
}

static void *get_public_bindings(void *scopep, void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration d=(Declaration)node;
      switch (Declaration_KEY(d)) {
      case KEYdeclaration:
	if (def_is_public(declaration_def(d))) {
	  *(SCOPE *)scopep = add_env_item(*(SCOPE *)scopep,d);
	  if (Declaration_KEY(d) == KEYpolymorphic) {
	    push_type_contour(d,0,0);
	    traverse_Block(get_public_bindings,scopep,polymorphic_body(d));
	    pop_type_contour();
	  }
	}
	return NULL;
      }
    }
  }
  return scopep;
}

static void *do_bind(void *vscope, void *node) {
  SCOPE scope=(SCOPE)vscope;
  /* sanity check: */
  if (scope != NULL) Declaration_KEY(scope->decl);
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYBlock:
    { void *top_mark = SALLOC(0);
      /* printf("vscope = 0x%x, scope = 0x%x\n",vscope,scope); */
      if (vscope != scope) fatal_error("corruption!");
      (void)bind_Declarations(scope,block_body((Block)node));
      release(top_mark);
      return NULL; }
  case KEYMatch:
    { SCOPE new_scope = scope;
      Match m = (Match)node;
      Pattern p = matcher_pat(m);
      void *top_mark = SALLOC(0);
      traverse_Pattern(get_bindings,&new_scope,p);
      traverse_Match_skip(do_bind,new_scope,m);
      release(top_mark);
      return NULL; }
  case KEYDeclaration:
    { SCOPE new_scope = scope;
      Declaration d = (Declaration)node;
      void *top_mark = SALLOC(0);
      switch (Declaration_KEY(d)) {
      case KEYclass_decl:
	set_instance_index(class_decl_type_formals(d));
	{ new_scope = bind_Declarations(new_scope,
					class_decl_type_formals(d));
	  new_scope = add_env_item(new_scope,class_decl_result_type(d));
	  traverse_Declaration_skip(do_bind,new_scope,d); }
	break;
      case KEYmodule_decl:
	if (module_TYPE == 0 && streq(decl_name(d),"TYPE"))
	  module_TYPE = d;
	else if (module_PHYLUM == 0 && streq(decl_name(d),"PHYLUM"))
	  module_PHYLUM = d;
	set_instance_index(module_decl_type_formals(d));
	{ new_scope = bind_Declarations(new_scope,
					module_decl_type_formals(d));
	  traverse_Signature(do_bind,new_scope,module_decl_parent(d));
	  new_scope = bind_Declarations(new_scope,
					module_decl_value_formals(d));
	  new_scope=add_env_item(new_scope,module_decl_result_type(d));
	  traverse_Declaration(do_bind,new_scope,
			       module_decl_result_type(d));
	  /* handle extension: common case only */
	  { Declaration rdecl = module_decl_result_type(d);
	    Type ext=some_type_decl_type(rdecl);
	    switch (Type_KEY(ext)) {
	    case KEYtype_use:
	      { Declaration tf = Use_info(type_use_use(ext))->use_decl;
		if (tf != NULL) {
		  switch (Declaration_KEY(tf)) {
		  case KEYsome_type_formal:
		    Declaration_info(tf)->decl_flags |=
		      TYPE_FORMAL_EXTENSION_FLAG;
		    new_scope=add_ext_sig(new_scope,rdecl,some_type_formal_sig(tf));
		    break;
		  }
		}
	      }
	      break;
	    }
	  }
	  traverse_Block(do_bind,new_scope,module_decl_contents(d)); }
    {
      Declaration type_formal = first_Declaration(module_decl_type_formals(d));
      for (;type_formal != NULL; type_formal = DECL_NEXT(type_formal))
      {
        Signature sig = some_type_formal_sig(type_formal);
        switch (Signature_KEY(sig))
        {
        case KEYsig_use:
          {
            Declaration use_decl = USE_DECL(sig_use_use(sig));
            switch (Declaration_KEY(use_decl))
            {
            case KEYsome_class_decl:
              aps_error(sig, "Missing actuals for %s in the signature", decl_name(use_decl));
              break;
            }
            break;
          }
        }
      }
    }
	break;
      case KEYattribute_decl:
	{ Type ftype = attribute_decl_type(d);
	  new_scope = bind_Declarations(new_scope,
					function_type_formals(ftype));
	  new_scope = bind_Declarations(new_scope,
					function_type_return_values(ftype));
	  traverse_Declaration_skip(do_bind,new_scope,d); }
	break;
      case KEYfunction_decl:
      case KEYprocedure_decl:
	{ Type ftype = some_function_decl_type(d);
	  new_scope = bind_Declarations(new_scope,
					function_type_formals(ftype));
	  new_scope = bind_Declarations(new_scope,
					function_type_return_values(ftype));
	  traverse_Declaration_skip(do_bind,new_scope,d); }
	break;
      case KEYpolymorphic:
	set_instance_index(polymorphic_type_formals(d));
	{ new_scope = bind_Declarations(new_scope,
					polymorphic_type_formals(d));
	  traverse_Declaration_skip(do_bind,new_scope,d); }
	break;
      case KEYfor_in_stmt:
	traverse_Expression(do_bind,new_scope,for_in_stmt_seq(d));
	new_scope = bind_Declaration(new_scope,for_in_stmt_formal(d));
	traverse_Block(do_bind,new_scope,for_in_stmt_body(d)); 
	break;
      default: return vscope; /* continue as normal */
      }
      release(top_mark);
      return NULL;
    }
    break;
  case KEYClass:
    switch (Class_KEY((Class)node)) {
    case KEYclass_use:
      bind_Use(class_use_use((Class)node),NAME_SIGNATURE,scope);
    }
    break;
  case KEYModule:
    switch (Module_KEY((Module)node)) {
    case KEYmodule_use:
      bind_Use(module_use_use((Module)node),NAME_SIGNATURE,scope);
    }
    break;
  case KEYSignature:
    switch (Signature_KEY((Signature)node)) {
    case KEYsig_use:
      bind_Use(sig_use_use((Signature)node),NAME_SIGNATURE,scope);
    }
    break;
  case KEYType:
    switch (Type_KEY((Type)node)) {
    case KEYtype_use:
      bind_Use(type_use_use((Type)node),NAME_TYPE,scope);
    }
    break;
  case KEYPattern:
    switch (Pattern_KEY((Pattern)node)) {
    case KEYpattern_use:
      bind_Use(pattern_use_use((Pattern)node),NAME_PATTERN,scope);
    }
    break;
  case KEYExpression:
    {
      SCOPE new_scope = scope;
      Expression e = (Expression)node;
      switch (Expression_KEY(e)) {
      case KEYvalue_use:
	bind_Use(value_use_use(e),NAME_VALUE,scope);
	break;
      case KEYcontrolled:
	traverse_Expression(do_bind,new_scope,controlled_set(e));
	new_scope = bind_Declaration(new_scope,controlled_formal(e));
	traverse_Expression(do_bind,new_scope,controlled_expr(e));
	return NULL;
      }
    }
    break;
  case KEYUse:
    {
      Use u = (Use)node;
      if (Use_info(u)->use_decl == NULL) {
	Symbol name =
	  (Use_KEY(u) == KEYqual_use) ? qual_use_name(u) : use_name(u);
	aps_error(u,"no binding for %s",symbol_name(name));
      }
    }
    break;
  }
  return vscope;
}

/************* setting the next_decl field **************/

static Unit units_set_next_unit(Units units, Unit next) {
  switch (Units_KEY(units)) {
  case KEYnil_Units: return next;
  case KEYlist_Units:
    { Unit prev = list_Units_elem(units);
      Unit_info(prev)->next_unit = next;
      return prev; }
  case KEYappend_Units:
    { Units some = append_Units_l1(units);
      Units more = append_Units_l2(units);
      Unit middle = units_set_next_unit(more,next);
      return units_set_next_unit(some,middle); }
  }
  fatal_error("control reached end of units_set_next_unit");
  return NULL;
}

static Declaration decls_set_next_decl(Declarations decls, Declaration next) {
  switch (Declarations_KEY(decls)) {
  case KEYnil_Declarations: return next;
  case KEYlist_Declarations:
    { Declaration prev = list_Declarations_elem(decls);
      Declaration_info(prev)->next_decl = next;
#ifdef DEBUG
      switch (Declaration_KEY(prev)) {
      case KEYdeclaration:
	{ char *name=symbol_name(def_name(declaration_def(prev)));
	  char *nextname="NULL";
	  if (next != NULL) {
	    switch (Declaration_KEY(next)) {
	    case KEYdeclaration:
	      nextname=symbol_name(def_name(declaration_def(next)));
	      break;
	    default:
	      nextname="<something>";
	      break;
	    }
	  }
	  fprintf(stderr,"setting decl_next(%s) to %s\n",name,nextname);
	}
      }
#endif
      return prev; }
  case KEYappend_Declarations:
    { Declarations some = append_Declarations_l1(decls);
      Declarations more = append_Declarations_l2(decls);
      Declaration middle = decls_set_next_decl(more,next);
      return decls_set_next_decl(some,middle); }
  }
  fatal_error("control reached end of decls_set_next_decl");
  return NULL;
}

static Expression actuals_set_next_actual(Actuals actuals, Expression next) {
  switch (Actuals_KEY(actuals)) {
  case KEYnil_Actuals: return next;
  case KEYlist_Actuals:
    { Expression prev = list_Actuals_elem(actuals);
      Expression_info(prev)->next_actual = next;
      return prev; }
  case KEYappend_Actuals:
    { Actuals some = append_Actuals_l1(actuals);
      Actuals more = append_Actuals_l2(actuals);
      Expression middle = actuals_set_next_actual(more,next);
      return actuals_set_next_actual(some,middle); }
  }
  fatal_error("control reached end of actuals_set_next_actual");
  return NULL;
}

static Expression exprs_set_next_expr(Expressions exprs, Expression next) {
  switch (Expressions_KEY(exprs)) {
  case KEYnil_Expressions: return next;
  case KEYlist_Expressions:
    { Expression prev = list_Expressions_elem(exprs);
      Expression_info(prev)->next_expr = next;
      return prev; }
  case KEYappend_Expressions:
    { Expressions some = append_Expressions_l1(exprs);
      Expressions more = append_Expressions_l2(exprs);
      Expression middle = exprs_set_next_expr(more,next);
      return exprs_set_next_expr(some,middle); }
  }
  fatal_error("control reached end of exprs_set_next_expr");
  return NULL;
}

static Pattern pattern_actuals_set_next_pattern_actual(PatternActuals pattern_actuals, Pattern next) {
  switch (PatternActuals_KEY(pattern_actuals)) {
  case KEYnil_PatternActuals: return next;
  case KEYlist_PatternActuals:
    { Pattern prev = list_PatternActuals_elem(pattern_actuals);
      Pattern_info(prev)->next_pattern_actual = next;
      return prev; }
  case KEYappend_PatternActuals:
    { PatternActuals some = append_PatternActuals_l1(pattern_actuals);
      PatternActuals more = append_PatternActuals_l2(pattern_actuals);
      Pattern middle = pattern_actuals_set_next_pattern_actual(more,next);
      return pattern_actuals_set_next_pattern_actual(some,middle); }
  }
  fatal_error("control reached end of pattern_actuals_set_next_pattern_actual");
  return NULL;
}

static Pattern patterns_set_next_pattern(Patterns patterns, Pattern next) {
  switch (Patterns_KEY(patterns)) {
  case KEYnil_Patterns: return next;
  case KEYlist_Patterns:
    { Pattern prev = list_Patterns_elem(patterns);
      Pattern_info(prev)->next_pattern = next;
      return prev; }
  case KEYappend_Patterns:
    { Patterns some = append_Patterns_l1(patterns);
      Patterns more = append_Patterns_l2(patterns);
      Pattern middle = patterns_set_next_pattern(more,next);
      return patterns_set_next_pattern(some,middle); }
  }
  fatal_error("control reached end of patterns_set_next_pattern");
  return NULL;
}

static Type type_actuals_set_next_type_actual(TypeActuals type_actuals, Type next) {
  switch (TypeActuals_KEY(type_actuals)) {
  case KEYnil_TypeActuals: return next;
  case KEYlist_TypeActuals:
    { Type prev = list_TypeActuals_elem(type_actuals);
      Type_info(prev)->next_type_actual = next;
      return prev; }
  case KEYappend_TypeActuals:
    { TypeActuals some = append_TypeActuals_l1(type_actuals);
      TypeActuals more = append_TypeActuals_l2(type_actuals);
      Type middle = type_actuals_set_next_type_actual(more,next);
      return type_actuals_set_next_type_actual(some,middle); }
  }
  fatal_error("control reached end of type_actuals_set_next_type_actual");
  return NULL;
}

static Match matches_set_next_match(Matches matches, Match next) {
  switch (Matches_KEY(matches)) {
  case KEYnil_Matches: return next;
  case KEYlist_Matches:
    { Match prev = list_Matches_elem(matches);
      Match_info(prev)->next_match = next;
      return prev; }
  case KEYappend_Matches:
    { Matches some = append_Matches_l1(matches);
      Matches more = append_Matches_l2(matches);
      Match middle = matches_set_next_match(more,next);
      return matches_set_next_match(some,middle); }
  }
  fatal_error("control reached end of matches_set_next_match");
  return NULL;
}

static void *set_next_fields(void *ignore, void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYUnits:
    { Unit unit = units_set_next_unit((Units)node,NULL);
      for (; unit != NULL; unit=Unit_info(unit)->next_unit) {
	traverse_Unit(set_next_fields,ignore,unit);
      }
    }
    return NULL;
  case KEYDeclarations:
    { Declaration decl = decls_set_next_decl((Declarations)node,NULL);
      for (; decl != NULL; decl=Declaration_info(decl)->next_decl) {
	traverse_Declaration(set_next_fields,ignore,decl);
      }
    }
    return NULL;
  case KEYActuals:
    { Expression actual = actuals_set_next_actual((Actuals)node,NULL);
      for (; actual != NULL; actual=Expression_info(actual)->next_actual) {
	traverse_Expression(set_next_fields,ignore,actual);
      }
    }
    return NULL;
  case KEYExpressions:
    { Expression expr = exprs_set_next_expr((Expressions)node,NULL);
      for (; expr != NULL; expr=Expression_info(expr)->next_expr) {
	traverse_Expression(set_next_fields,ignore,expr);
      }
    }
    return NULL;
  case KEYPatterns:
    { Pattern pattern = patterns_set_next_pattern((Patterns)node,NULL);
      for (; pattern != NULL; pattern=Pattern_info(pattern)->next_pattern) {
	traverse_Pattern(set_next_fields,ignore,pattern);
      }
    }
    return NULL;
  case KEYPatternActuals:
    { Pattern pattern_actual = pattern_actuals_set_next_pattern_actual((PatternActuals)node,NULL);
      for (; pattern_actual != NULL; pattern_actual=Pattern_info(pattern_actual)->next_pattern_actual) {
	traverse_Pattern(set_next_fields,ignore,pattern_actual);
      }
    }
    return NULL;
  case KEYTypeActuals:
    { Type actual = type_actuals_set_next_type_actual((TypeActuals)node,NULL);
      for (; actual != NULL; actual=Type_info(actual)->next_type_actual) {
	traverse_Type(set_next_fields,ignore,actual);
      }
    }
    return NULL;
  case KEYMatches:
    { Match match = matches_set_next_match((Matches)node,NULL);
      Declaration parent = (Declaration)tnode_parent(node);
      (void)Declaration_KEY(parent);
      for (; match != NULL; match=Match_info(match)->next_match) {
	Match_info(match)->header = parent;
	traverse_Match(set_next_fields,ignore,match);
      }
    }
    return NULL;
  case KEYDeclaration:
    if (Declaration_KEY((Declaration)node) == KEYtop_level_match) {
      Declaration tlm = (Declaration)node;
      Match_info(top_level_match_m(tlm))->header = tlm;
    }
    /* Fall through */
  default: break;
  }
  return ignore;
}

Unit first_Unit(Units l) {
  switch (Units_KEY(l)) {
  case KEYlist_Units:
    return list_Units_elem(l);
  case KEYappend_Units:
    { Unit first = first_Unit(append_Units_l1(l));
      if (first != NULL) return first;
      return first_Unit(append_Units_l2(l)); }
  default:
    return NULL;
  }
}

/* potentially useful later */
Declaration first_Declaration(Declarations l) {
  switch (Declarations_KEY(l)) {
  case KEYlist_Declarations:
    return list_Declarations_elem(l);
  case KEYappend_Declarations:
    { Declaration first = first_Declaration(append_Declarations_l1(l));
      if (first != NULL) return first;
      return first_Declaration(append_Declarations_l2(l)); }
  default:
    return NULL;
  }
}

Expression first_Actual(Actuals l) {
  switch (Actuals_KEY(l)) {
  case KEYlist_Actuals:
    return list_Actuals_elem(l);
  case KEYappend_Actuals:
    { Expression first = first_Actual(append_Actuals_l1(l));
      if (first != NULL) return first;
      return first_Actual(append_Actuals_l2(l)); }
  default:
    return NULL;
  }
}

Expression first_Expression(Expressions l) {
  switch (Expressions_KEY(l)) {
  case KEYlist_Expressions:
    return list_Expressions_elem(l);
  case KEYappend_Expressions:
    { Expression first = first_Expression(append_Expressions_l1(l));
      if (first != NULL) return first;
      return first_Expression(append_Expressions_l2(l)); }
  default:
    return NULL;
  }
}

Pattern first_PatternActual(PatternActuals l) {
  switch (PatternActuals_KEY(l)) {
  case KEYlist_PatternActuals:
    return list_PatternActuals_elem(l);
  case KEYappend_PatternActuals:
    { Pattern first = first_PatternActual(append_PatternActuals_l1(l));
      if (first != NULL) return first;
      return first_PatternActual(append_PatternActuals_l2(l)); }
  default:
    return NULL;
  }
}

Pattern first_Pattern(Patterns l) {
  switch (Patterns_KEY(l)) {
  case KEYlist_Patterns:
    return list_Patterns_elem(l);
  case KEYappend_Patterns:
    { Pattern first = first_Pattern(append_Patterns_l1(l));
      if (first != NULL) return first;
      return first_Pattern(append_Patterns_l2(l)); }
  default:
    return NULL;
  }
}

Type first_TypeActual(TypeActuals l) {
  switch (TypeActuals_KEY(l)) {
  case KEYlist_TypeActuals:
    return list_TypeActuals_elem(l);
  case KEYappend_TypeActuals:
    { Type first = first_TypeActual(append_TypeActuals_l1(l));
      if (first != NULL) return first;
      return first_TypeActual(append_TypeActuals_l2(l)); }
  default:
    return NULL;
  }
}

Match first_Match(Matches l) {
  switch (Matches_KEY(l)) {
  case KEYlist_Matches:
    return list_Matches_elem(l);
  case KEYappend_Matches:
    { Match first = first_Match(append_Matches_l1(l));
      if (first != NULL) return first;
      return first_Match(append_Matches_l2(l)); }
  default:
    return NULL;
  }
}



/*** PRAGMA ACTIVATION ***/

static void *activate_pragmas(void *ignore, void *node) {
  static Symbol synthesized_symbol=NULL;
  static Symbol inherited_symbol=NULL;
  static Symbol field_strict_symbol=NULL;
  static Symbol fiber_untracked_symbol=NULL;
  static Symbol fiber_cyclic_symbol=NULL;
  static Symbol start_phylum_symbol=NULL;
  static Symbol self_managed_symbol=NULL;
  if (synthesized_symbol == NULL) {
    synthesized_symbol=intern_symbol("synthesized");
    inherited_symbol=intern_symbol("inherited");
    field_strict_symbol=intern_symbol("field_strict");
    fiber_untracked_symbol=intern_symbol("fiber_untracked");
    fiber_cyclic_symbol=intern_symbol("fiber_cyclic");
    start_phylum_symbol=intern_symbol("root_phylum");
    self_managed_symbol=intern_symbol("self_managed");
  }
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration decl=(Declaration)node;
    switch (Declaration_KEY(decl)) {
    case KEYpragma_call:
      { Symbol pragma_sym = pragma_call_name(decl);
	Expressions exprs = pragma_call_parameters(decl);
	Expression expr = first_Expression(exprs);
	for (; expr != NULL; expr=Expression_info(expr)->next_expr) {
	  switch (Expression_KEY(expr)) {
	  case KEYvalue_use:
	    { Declaration d = Use_info(value_use_use(expr))->use_decl;
	      if (d != NULL) {
		if (pragma_sym == synthesized_symbol) {
		  if (bind_debug & PRAGMA_ACTIVATION)
		    printf("%s is synthesized\n",
			   symbol_name(def_name(declaration_def(d))));
		  Declaration_info(d)->decl_flags |= ATTR_DECL_SYN_FLAG;
		} else if (pragma_sym == inherited_symbol) {
		  if (bind_debug & PRAGMA_ACTIVATION)
		    printf("%s is inherited\n",
			   symbol_name(def_name(declaration_def(d))));
		  Declaration_info(d)->decl_flags |= ATTR_DECL_INH_FLAG;
		} else if (pragma_sym == field_strict_symbol) {
		  if (bind_debug & PRAGMA_ACTIVATION)
		    printf("%s is strict for incrementality\n",
			   symbol_name(def_name(declaration_def(d))));
		  Declaration_info(d)->decl_flags |= FIELD_DECL_STRICT_FLAG;
		} else if (pragma_sym == fiber_untracked_symbol) {
		  if (bind_debug & PRAGMA_ACTIVATION)
		    printf("%s is untracked for fibering\n",
			   symbol_name(def_name(declaration_def(d))));
		  Declaration_info(d)->decl_flags |=
		    FIELD_DECL_UNTRACKED_FLAG;
		} else if (pragma_sym == fiber_cyclic_symbol) {
		  if (bind_debug & PRAGMA_ACTIVATION)
		    printf("%s is cyclic for fibering\n",
			   symbol_name(def_name(declaration_def(d))));
		  Declaration_info(d)->decl_flags |= FIELD_DECL_CYCLIC_FLAG;
		}
	      }
	    }
	    break;
	  case KEYtype_value:
	    switch (Type_KEY(type_value_t(expr))) {
	    case KEYtype_use:
	      { Declaration d = USE_DECL(type_use_use(type_value_t(expr)));
		if (pragma_sym == start_phylum_symbol) {
		  if (bind_debug & PRAGMA_ACTIVATION)
		    printf("%s is start phylum\n",
			   symbol_name(def_name(declaration_def(d))));
		  Declaration_info(d)->decl_flags |= START_PHYLUM_FLAG;
		} else if (pragma_sym == self_managed_symbol) {
		  if (bind_debug & PRAGMA_ACTIVATION)
		    printf("%s is self managed\n",
			   symbol_name(def_name(declaration_def(d))));
		  Declaration_info(d)->decl_flags |= SELF_MANAGED_FLAG;
		}
	      }
	    }
	    break;
	  }
	}
      }
      break; 
    }
  }
  return ignore;
}
