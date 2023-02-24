// Implementation for 790-2 (Program Analysis) project
// Dec. 1999
// Yu Wang

#include <stdio.h>
#include "jbb.h"
#include "jbb-alloc.h"
#include "aps-ag.h"

CALLSITE_SET next_cs = 1;
int CallSiteNum = 0;
VECTOR(CALLSITE_SET) refs;
  
int SHOW_AI_INFO = 1;


void nothing() {}

int callset_AI(Declaration module, STATE *s) {
   int changed = 1;
   int ref_num = 0;
   int i = 0;
      
   traverse_Declaration(count_things, &ref_num, module);
   DEBUG_INFO("%d\t%d\n", CallSiteNum, ref_num);

   VECTORALLOC(refs, CALLSITE_SET, ref_num);
   for(i=0;i<refs.length;i++) refs.array[i] = 0;
   i = 0;
   traverse_Declaration(init_things, &i, module);

   while(changed) {
     changed = 0;  
     traverse_Declaration(traverser, &changed, module);
   }

   traverse_Declaration(call_sites_report, &i, module);
   
   DEBUG_INFO("Mission accomplished!\n");
   exit(0);
}


/* wrap up the manipulations of CALLSITE_SET */
int callsite_set_empty_p(CALLSITE_SET css) {
  return (css==0);
}

void INCLUDE(CALLSITE_SET* pDest, CALLSITE_SET source) {
  if(pDest != NULL && source != 0)
    *pDest |= source;
}

int assign_sets(CALLSITE_SET site, void* node, CALLSITE_SET source) {
  int dirty = 0;
  VECTOR(CALLSITE_SET)* pCS = Declaration_info(field_ref_p(node))->call_sites;
  int i, dist;
  CALLSITE_SET oldSite;
  for(i = 1, dist=0; i<next_cs; i<<=1,dist++)
    if(site&i) {
      oldSite = pCS->array[dist];
      pCS->array[dist] |= source;
      if(oldSite != pCS->array[dist]) dirty = 1;
    }
  return dirty;
}
  
/* go through all assignments iteratively */
void* traverser(void *changed, void *node) {
  int * dirty_sign = (int*)changed;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      
      switch (Declaration_KEY(decl)) {
      default: break;
      /* only inspect the assignments */
      case KEYassign:
nothing();
        DEBUG_INFO("#%d", tnode_line_number(decl));
        {
        Expression lhs = assign_lhs(decl);
        Expression rhs = assign_rhs(decl);
        Declaration fdecl = field_ref_p(lhs);
        CALLSITE_SET RhsSites = interpret(rhs);

        DEBUG_INFO("\t\tRHS sites = %d\n", RhsSites);
        if(callsite_set_empty_p(RhsSites)) return changed;

        if(fdecl != NULL) 
          {
          Actuals actuals = funcall_actuals(lhs);
          Expression actual = first_Actual(actuals);
          CALLSITE_SET Sites = interpret(actual);
          DEBUG_INFO("LHS %s of #%d %p %p\n", 
                      decl_name(fdecl), tnode_line_number(fdecl), 
                      fdecl, Declaration_info(fdecl)->call_sites);
          if(callsite_set_empty_p(Sites)) 
            {
              DEBUG_INFO("%dUnidentified field selection\n",
                tnode_line_number(lhs));
              return changed;
            }
          else
            {
              if( assign_sets(Sites, lhs, RhsSites) ) 
                *dirty_sign = 1;
            }
          }
        else 
          {
          CALLSITE_SET* pLhsSites = locater(lhs);
          CALLSITE_SET old_sites = *pLhsSites;
          if(pLhsSites == NULL)
            {
            DEBUG_INFO("%dAssignment aborted.\n", tnode_line_number(lhs));
            }
          else
            {
            INCLUDE(pLhsSites, RhsSites);
            if(*pLhsSites != old_sites) *dirty_sign = 1;
            }
          }
        }
      }
    }
  }
  return changed;
}

/* find the location of call_sites storage, not used for fields */
void* locater(void *node) {
  Expression expr = (Expression)node;
  Declaration attr_ref = attr_ref_p(expr);

  if (attr_ref != NULL) 
    return Declaration_info(attr_ref)->call_sites;
  else {
  Declaration udecl = sth_use_p(expr);
  if (udecl!=NULL) 
    return Declaration_info(udecl)->call_sites;
  }

  return NULL;
}

/* main interpretation function */
CALLSITE_SET interpret(void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  /* only inspect RHS expressions */
  case KEYExpression:
    { Expression expr = (Expression)node;
    
      Declaration proc = proc_call_p(expr);
      Declaration constr = constructor_call_p(expr);
      Declaration attr = attr_ref_p(expr);
      Declaration udecl = sth_use_p(expr);
      Declaration field = field_ref_p(expr);
      CALLSITE_SET* pSite;
      
      if(attr != NULL) 
        {  
        pSite = Declaration_info(attr)->call_sites;
        return *(pSite);
        }
      if(udecl != NULL) 
        { 
          /* first check if it is declared in case statement */
	  Declaration case_stmt;
	  if ((case_stmt = formal_in_case_p(udecl)) != NULL) {
	    Expression expr = case_stmt_expr(case_stmt);
	    CALLSITE_SET case_expr_sites = interpret(expr);
	    DEBUG_INFO("\t\tin case statement [%d]\n", tnode_line_number(case_stmt));
	    return case_expr_sites;
	  }

        pSite = Declaration_info(udecl)->call_sites;
        if(pSite == NULL) {
          DEBUG_INFO("\tValue use not declared locally: %s\n",
                     decl_name(udecl));
          return NULL;
        }  
        return *(pSite);
        }
      else if(constr != NULL) 
        {
        DEBUG_INFO("%dUnexpected Constructor call\n", 
                tnode_line_number(expr));
        exit(-1);
        }
      else if(proc != NULL) 
        { Declarations formals =
              function_type_formals(some_function_decl_type(proc));
          Declaration result = some_function_decl_result(proc);
          Actuals actuals = funcall_actuals(expr);
          Declaration formal;
          Expression actual;

          for (actual = first_Actual(actuals),
               formal = first_Declaration(formals);
               actual != NULL && formal != NULL;
               actual = Expression_info(actual)->next_actual,
               formal = Declaration_info(formal)->next_decl) 
            {
            CALLSITE_SET* pFormalSites = 
                      (CALLSITE_SET*)Declaration_info(formal)->call_sites;
            CALLSITE_SET ActualSites = interpret(actual);
            CALLSITE_SET old_sites = *pFormalSites;
            if(pFormalSites == NULL || callsite_set_empty_p(ActualSites))
              {
              DEBUG_INFO("%dAssignment aborted.\n", tnode_line_number(expr));
              }
            else
              {
              INCLUDE(pFormalSites, ActualSites);
              }
            }
            
          pSite = Declaration_info(result)->call_sites;
          return *(pSite);
        }
      
      switch(Expression_KEY(expr)) {
      default:
        return NULL;
      case KEYfuncall:
        { Expression func = funcall_f(expr);
          Actuals actuals = funcall_actuals(expr);
          Expression actual;
          switch (Expression_KEY(func)) {
          case KEYvalue_use:
            { 
              Declaration attr = USE_DECL(value_use_use(func));
              actual = first_Actual(actuals);
              if(attr != NULL)  DEBUG_INFO("func is %s\n", decl_name(attr));
              if (attr == NULL) aps_error(func,"unbound function");
              else if (DECL_IS_LOCAL(attr) && FIELD_DECL_P(attr)) 
              /* field selection : e.f */
  	      {
                if(actual == NULL)
                {
                  DEBUG_INFO("%dnull field selection.\n", tnode_line_number(func));
                  exit(-1);
                }
                else
                {
	          CALLSITE_SET sites = interpret(actual);
                  VECTOR(CALLSITE_SET)* pCS = Declaration_info(attr)->call_sites;
                  int i, dist;
                  CALLSITE_SET newSites = 0;
                  for(i = 1, dist=0; i<next_cs; i<<=1,dist++)
                    if(sites&i) newSites |= pCS->array[dist];
                  return newSites;
	        }
	      } 
	      else
	      /* func call : f(e1,...,en) */
	      {
                CALLSITE_SET tmp_set = 0;
                for (actual = first_Actual(actuals);
                  actual != NULL;
                  actual = Expression_info(actual)->next_actual)
                  {
                  INCLUDE(&tmp_set, interpret(actual)); 
                  }
               return tmp_set;
	      }
            }
            break;
          } //switch (Expression_KEY(func))
        } //case KEYfuncall
        break;
  
      }

    }	//case KEYExpression
  }	//switch
  return NULL;
}

/* preparation phase, count callsites, etc, in order to allocate mem */
void *count_things(void *ref_num, void *node) {
  int * num = (int*)ref_num;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      Declaration cdecl = object_decl_p(decl);
      if(cdecl != NULL ) 
        { int base = 1;
          CallSiteNum++;
          if(CallSiteNum > MAX_CALLSITE ) {
            printf("Can't handle more than %d callsites! Abort.\n", MAX_CALLSITE);
            exit(0);
            }
          printf("[%d]\tCallsite #0x%x\t: %s\n", 
                 tnode_line_number(decl), base<<(CallSiteNum-1), decl_name(cdecl));
        }   
      
      switch (Declaration_KEY(decl)) {
      default: break;
      case KEYvalue_decl:
      case KEYattribute_decl:
      case KEYprocedure_decl:	
      case KEYnormal_formal:	
        ++(*num);	
        break;
      }
    }
    break;
  default:  break;
  }
  return ref_num;
}

/* initialize memory for storing callsites info
 * each field is associated with a vector of length #callsites
 * others just one CALLSITE_SET cell from a global vector
 */
void *init_things(void *dist, void *node) {
  int *loc = (int*)dist;
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
      default: break;
      case KEYvalue_decl:
        {
        Declaration cdecl = object_decl_p(decl);
        Declaration_info(decl)->call_sites = &(refs.array[*loc]);
        ++(*loc);
        if(cdecl != NULL) 
          {
          INCLUDE(Declaration_info(decl)->call_sites, next_cs);
          DEBUG_INFO("#%d callsite\t%d\n",tnode_line_number(decl), 
                 *((CALLSITE_SET*)Declaration_info(decl)->call_sites));
          next_cs <<= 1;
          }
        DEBUG_INFO("#%dKEYvalue_decl %s array: %x\n",
               tnode_line_number(decl), decl_name(decl), &(refs.array[*loc]));
	}
	break;	   
      case KEYprocedure_decl:	
        {
        Declaration_info(decl)->call_sites = &(refs.array[*loc]);
        ++(*loc);
        DEBUG_INFO("#%dKEYprocedure_decl %s array: %x\n",
               tnode_line_number(decl), decl_name(decl), &(refs.array[*loc]));
	}
	break;	   
      case KEYnormal_formal:	
        {
        Declaration_info(decl)->call_sites = &(refs.array[*loc]);
        ++(*loc);
        DEBUG_INFO("#%dKEYnormal_formal %s array: %x\n",
               tnode_line_number(decl), decl_name(decl), &(refs.array[*loc]));
	}
	break;	   
      case KEYattribute_decl:
        {
        if(FIELD_DECL_P(decl) && DECL_IS_LOCAL(decl)) 
          {
          VECTOR(CALLSITE_SET) * sites = HALLOC(sizeof(VECTOR(CALLSITE_SET)));
          VECTORALLOC(*sites, CALLSITE_SET, CallSiteNum);
          Declaration_info(decl)->call_sites = sites;
          DEBUG_INFO("#%d\tfield declaration\t%s %p %p\n",
                  tnode_line_number(decl), decl_name(decl), decl, sites);
            {
            int i;
            for(i=0; i<sites->length; i++)
            sites->array[i] = 0;
            }
          }
        else 
          {
          Declaration_info(decl)->call_sites = &(refs.array[*loc]);
          ++(*loc);
          DEBUG_INFO("#%dKEYattribute_decl %s array: %x\n",
               tnode_line_number(decl), decl_name(decl), &(refs.array[*loc]));
          }        
        }
      }
    }
  }
  return dist;
}

void *call_sites_report(void *nouse, void *node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
      case KEYattribute_decl:
        if(DECL_IS_LOCAL(decl) && !FIELD_DECL_P(decl)) {
          printf("[%d]\t%s\t", tnode_line_number(decl), decl_name(decl));
          if(strlen(decl_name(decl)) < 8) printf("\t");
          printf("0x%x\n", *((int*)Declaration_info(decl)->call_sites));
          }

        if(DECL_IS_LOCAL(decl) && FIELD_DECL_P(decl)) {
          int i;
          VECTOR(CALLSITE_SET)* pCS = Declaration_info(decl)->call_sites;
          printf("[%d]\t%s\t", tnode_line_number(decl), decl_name(decl));
          if(strlen(decl_name(decl)) < 8) printf("\t");
          for(i=0;i<pCS->length;i++) printf("0x%x\t", pCS->array[i]);
          printf("\n");
          }

        break;
      case KEYvalue_decl:
        if(strcmp(decl_name(decl), "_") != 0 
           && strcmp(decl_name(decl), "result") != 0) {
          printf("[%d]\t%s\t", tnode_line_number(decl), decl_name(decl));
          if(strlen(decl_name(decl)) < 8) printf("\t");
          printf("0x%x\n", *((int*)Declaration_info(decl)->call_sites));
        }
      }
    }
    break;
  default:  break;
  }
  return nouse;
}

Declaration sth_use_p(Expression expr) {
  if (responsible_node_declaration(expr) == NULL) return NULL;
  switch (Expression_KEY(expr)) {
  case KEYvalue_use:
    { Declaration decl = USE_DECL(value_use_use(expr));
      if (decl == NULL) return NULL;
      switch (Declaration_KEY(decl)) {
      case KEYvalue_decl:
      case KEYnormal_formal:
	return decl;
      }
    }
    break;
  }
  return NULL;
}


/* 
 * following are a bunch of debug fuctions, 
 * totally deletable if there will be no other aps novice like yw
 */
void *check_all_decls(void *nouse, void * node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration d = (Declaration)node;
      decl_type(d); 
    }
    break;
  }
  return nouse;
}

void *check_all_assign(void *nouse, void * node) {
  switch (ABSTRACT_APS_tnode_phylum(node)) {
  case KEYDeclaration:
    { Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
      case KEYassign:
        DEBUG_INFO("#%d\t",
                tnode_line_number(decl));
        {
        Expression lhs = assign_lhs(decl);
        Expression rhs = assign_rhs(decl);
        DEBUG_INFO("LHS=");
        expr_type(lhs);
        DEBUG_INFO("\tRHS=");
        expr_type(rhs);
        DEBUG_INFO("\n");
        }
      default: break;
      }
    }
    break;
  }
  return nouse;
}

void decl_type(Declaration d) {
  switch (Declaration_KEY(d)) {
  case KEYprocedure_decl:	
  	DEBUG_INFO("%d\tKEYprocedure_decl\t%s\n", tnode_line_number(d),decl_name(d)); break;
  case KEYnormal_formal:	
  	DEBUG_INFO("%d\tKEYnormal_formal\t%s\n", tnode_line_number(d),decl_name(d)); break;
  case KEYno_decl:	
  	DEBUG_INFO("%d\tKEYno_decl\n", tnode_line_number(d)); break;
  case KEYclass_decl:	
  	DEBUG_INFO("%d\tKEYclass_decl\n", tnode_line_number(d)); break;
  case KEYmodule_decl:	
  	DEBUG_INFO("%d\tKEYmodule_decl\n", tnode_line_number(d)); break;
  case KEYphylum_decl:	
  	DEBUG_INFO("%d\tKEYphylum_decl\n", tnode_line_number(d)); break;
  case KEYtype_decl:	
  	DEBUG_INFO("%d\tKEYtype_decl\n", tnode_line_number(d)); break;
  case KEYvalue_decl:	
  	DEBUG_INFO("%d\tKEYvalue_decl\t%s\n", tnode_line_number(d),decl_name(d)); break;
  case KEYattribute_decl:	
  	DEBUG_INFO("%d\tKEYattribute_decl\t%s\n", tnode_line_number(d), decl_name(d)); break;
  case KEYfunction_decl:	
  	DEBUG_INFO("%d\tKEYfunction_decl\t%s\n", tnode_line_number(d),decl_name(d)); break;
  case KEYconstructor_decl:	
  	DEBUG_INFO("%d\tKEYconstructor_decl\t%s\n", tnode_line_number(d),decl_name(d)); break;
  case KEYpattern_decl:	
  	DEBUG_INFO("%d\tKEYpattern_decl\n", tnode_line_number(d)); break;
  case KEYnormal_assign:	
  	DEBUG_INFO("%d\tKEYnormal_assign\t\n", tnode_line_number(d)); break;
  case KEYcollect_assign:	
  	DEBUG_INFO("%d\tcollect_assign\n", tnode_line_number(d)); break;
  case KEYif_stmt:	
  	DEBUG_INFO("%d\tKEYif_stmt\n", tnode_line_number(d)); break;
  case KEYcase_stmt:	
  	DEBUG_INFO("%d\tKEYcase_stmt\n", tnode_line_number(d)); break;
  case KEYpragma_call:	
  	DEBUG_INFO("%d\tKEYpragma_call\n", tnode_line_number(d)); break;
  default:	break;
  }
}

void value_use_decl_type(Expression e) {
  switch (Expression_KEY(e)) {
  case KEYvalue_use:
    { Declaration decl = USE_DECL(value_use_use(e));
      switch (Declaration_KEY(decl)) {
	case KEYprocedure_decl:	DEBUG_INFO("KEYprocedure_decl\n"); break;
	case KEYnormal_formal:	DEBUG_INFO("KEYnormal_formal\n"); break;
        default:		DEBUG_INFO("other decl type\n"); break;
      }
    }
  }
}

void expr_type(Expression e) {
  switch (Expression_KEY(e)) {
  case KEYvalue_use:		DEBUG_INFO("KEYvalue_use"); break;
  case KEYtyped_value:		DEBUG_INFO("KEYtyped_value"); break;
  case KEYfuncall:		DEBUG_INFO("KEYfuncall"); break;
  case KEYpattern_value:	DEBUG_INFO("KEYpattern_value"); break;
  case KEYinteger_const: 	DEBUG_INFO("KEYinteger_const"); break;
  case KEYreal_const:		DEBUG_INFO("KEYreal_const"); break;
  case KEYstring_const: 	DEBUG_INFO("KEYstring_const");break;
  case KEYchar_const: 		DEBUG_INFO("KEYchar_const");break;
  case KEYundefined: 		DEBUG_INFO("KEYundefined"); break;
  case KEYno_expr: 		DEBUG_INFO("KEYno_expr"); break;
  case KEYappend:		DEBUG_INFO("KEYappend"); break;
  case KEYempty:		DEBUG_INFO("KEYempty"); break;
  case KEYclass_value:		DEBUG_INFO("KEYclass_value"); break;
  case KEYmodule_value:		DEBUG_INFO("KEYmodule_value"); break;
  default:			DEBUG_INFO("Other expression type"); break;
  }	        
}
