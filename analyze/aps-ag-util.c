#include <stdio.h>
#include <jbb.h>

#include "aps-ag.h"

/** We require that top-level matches have the form
 *     match ?name:Type=constructor(?arg1,?arg2,...)
 * and provide the following functions for accessing the pieces.
 */

Declaration top_level_match_lhs_decl(Declaration tlm) {
  return match_lhs_decl(top_level_match_m(tlm));
}

Declaration match_lhs_decl(Match m) {
  Pattern pat=matcher_pat(m);
  switch (Pattern_KEY(pat)) {
  default: fatal_error("%d:improper match",
		       tnode_line_number(m));
  case KEYpattern_call:
    return NULL;
    break;
  case KEYand_pattern:
    pat = and_pattern_p1(pat);
    switch (Pattern_KEY(pat)) {
    default:
      fatal_error("%d:improper match",
		  tnode_line_number(m));
      return NULL;
    case KEYpattern_var:
      return pattern_var_formal(pat);
    }
  }
}

Use top_level_match_constructor_use(Declaration tlm) {
  return match_constructor_use(top_level_match_m(tlm));
}

Use match_constructor_use(Match m) {
  Pattern pat=matcher_pat(m);
  switch (Pattern_KEY(pat)) {
  default: fatal_error("%d:improper match",
		       tnode_line_number(m));
  case KEYpattern_call:
    break;
  case KEYand_pattern:
    pat = and_pattern_p2(pat);
    break;
  }
  
  switch (Pattern_KEY(pat)) {
  default: fatal_error("%d:misformed pattern",tnode_line_number(pat));
  case KEYpattern_call:
    switch (Pattern_KEY(pattern_call_func(pat))) {
    default:
      fatal_error("%d:unknown pattern function",tnode_line_number(pat));
      return NULL;
    case KEYpattern_use:
      return pattern_use_use(pattern_call_func(pat));
    }
  }
}

Declaration top_level_match_constructor_decl(Declaration tlm) {
  return match_constructor_decl(top_level_match_m(tlm));
}

Declaration match_constructor_decl(Match m) {
  Use u = match_constructor_use(m);
  Declaration decl = Use_info(u)->use_decl;
  if (decl == NULL) fatal_error("%d:unbound pfunc",
				tnode_line_number(m));
  if (Declaration_KEY(decl) != KEYconstructor_decl)
    fatal_error("%d: can only match on constructors directly",
		tnode_line_number(m));
  return decl;
}

Declaration top_level_match_first_rhs_decl(Declaration tlm) {
  return match_first_rhs_decl(top_level_match_m(tlm));
}

Declaration match_first_rhs_decl(Match m) {
  Pattern pat=matcher_pat(m);
  switch (Pattern_KEY(pat)) {
  default: fatal_error("%d:improper match", tnode_line_number(m));
  case KEYand_pattern:
    pat = and_pattern_p2(pat);
    break;
  case KEYpattern_call:
    break;
  }
  switch (Pattern_KEY(pat)) {
  default: fatal_error("%d:misformed pattern",tnode_line_number(pat));
  case KEYpattern_call:
    pat = first_PatternActual(pattern_call_actuals(pat));
    if (pat == NULL) return NULL;
    switch (Pattern_KEY(pat)) {
    default:
      fatal_error("%d:improper match", tnode_line_number(m));
      return 0;
    case KEYpattern_var:
      return pattern_var_formal(pat);
    }
  }
}

Declaration next_rhs_decl(Declaration rhs_decl) {
  Pattern pat = (Pattern)(tnode_parent(rhs_decl));
  pat = Pattern_info(pat)->next_pattern_actual;
  if (pat == NULL) return NULL;
  switch (Pattern_KEY(pat)) {
  default: fatal_error("%d:improper top-level-match",
		       tnode_line_number(rhs_decl));
  case KEYpattern_var:
    return pattern_var_formal(pat);
  }
}
