#include <iostream>
extern "C" {
#include <stdio.h>
#include "aps-ag.h"
}
#include "dump.h"
#include "implement.h"

static unsigned global_match_index = 0;

unsigned get_match_index(Match match)
{
  if (Match_info(match)->match_index == 0) {
    Match_info(match)->match_index = ++global_match_index;
  }
  return Match_info(match)->match_index;
}

Implementation::ModuleInfo::ModuleInfo(Declaration module)
  : module_decl(module) 
{}

void Implementation::ModuleInfo::note_top_level_match(Declaration tlm,
						 GEN_OUTPUT&)
{
  top_level_matches.push_back(tlm);
}

void Implementation::ModuleInfo::note_var_value_decl(Declaration vd,
						GEN_OUTPUT&)
{
  var_value_decls.push_back(vd);
}

void Implementation::ModuleInfo::note_local_attribute(Declaration ld,
						 GEN_OUTPUT&)
{
  local_attributes.push_back(ld);
}

void Implementation::ModuleInfo::note_attribute_decl(Declaration ad,
						     GEN_OUTPUT&)
{
  attribute_decls.push_back(ad);
}

static void *clear_impl_marks(void *ignore, void *node) {
  if (ABSTRACT_APS_tnode_phylum(node) == KEYDeclaration) {
    Declaration_info((Declaration)node)->decl_flags &= ~IMPLEMENTATION_MARKS;
  }
  return ignore;
}

void clear_implementation_marks(Declaration d) {
  int nothing;
  traverse_Declaration(clear_impl_marks,&nothing,d);
}

static bool unconstrained_rest_pattern(Pattern pattern)
{
  return pattern && Pattern_KEY(pattern) == KEYrest_pattern &&
      Pattern_KEY(rest_pattern_constraint(pattern)) == KEYno_pattern;
}

static bool sequence_for_pattern_recursive(Pattern pattern,
                                           SequenceForPattern *result)
{
  if (!pattern) return !result->elements.empty();
  if (Pattern_KEY(pattern) == KEYrest_pattern) {
    if (!unconstrained_rest_pattern(pattern)) return false;
    result->rests[result->elements.size()] = true;
  } else {
    result->elements.push_back(pattern);
    result->rests.resize(result->elements.size()+1,false);
  }
  return sequence_for_pattern_recursive(PAT_NEXT(pattern),result);
}

bool sequence_for_pattern(Pattern pattern, SequenceForPattern *result)
{
  Symbol sequence_symbol = intern_symbol("{}");
  if (Pattern_KEY(pattern) != KEYpattern_call) return false;

  Pattern function = pattern_call_func(pattern);
  if (Pattern_KEY(function) != KEYpattern_use) return false;
  Declaration function_decl = USE_DECL(pattern_use_use(function));
  if (!function_decl ||
      def_name(declaration_def(function_decl)) != sequence_symbol) return false;

  SequenceForPattern found = {};
  found.rests.resize(1,false);
  Pattern actual = first_PatternActual(pattern_call_actuals(pattern));
  if (!sequence_for_pattern_recursive(actual,&found)) return false;
  bool has_rest = false;
  for (bool rest : found.rests) has_rest |= rest;
  if (!has_rest) return false;

  unsigned endpoints = 0;
  if (!found.rests.front()) {
    found.positions |= SEQUENCE_FOR_FIRST;
    ++endpoints;
  }
  if (!found.rests.back()) {
    found.positions |= SEQUENCE_FOR_LAST;
    ++endpoints;
  }
  if (found.elements.size() > endpoints)
    found.positions |= SEQUENCE_FOR_EACH;
  *result = found;
  return true;
}

bool sequence_search_pattern(Pattern p, Pattern *middle)
{
  SequenceForPattern result;
  if (!sequence_for_pattern(p,&result) ||
      result.positions != SEQUENCE_FOR_EACH || result.elements.size() != 1)
    return false;
  *middle = result.elements.front();
  return true;
}

bool sequence_search_matcher(Declaration decl, Match *match, Pattern *middle)
{
  Matches matchers;
  switch (Declaration_KEY(decl)) {
  case KEYcase_stmt:
    matchers = case_stmt_matchers(decl);
    break;
  case KEYfor_stmt:
    matchers = for_stmt_matchers(decl);
    break;
  default:
    return false;
  }
  Match first = first_Match(matchers);
  if (!first || MATCH_NEXT(first)) return false;
  if (!sequence_search_pattern(matcher_pat(first),middle)) return false;
  if (match) *match = first;
  return true;
}

bool block_assigns_to(Block b, void *vdecl)
{
  for (Declaration d = first_Declaration(block_body(b)); d; d = DECL_NEXT(d)) {
    switch (Declaration_KEY(d)) {
    case KEYassign:
      {
	Expression lhs = assign_lhs(d);
	if (Expression_KEY(lhs) == KEYvalue_use &&
	    USE_DECL(value_use_use(lhs)) == vdecl) return true;
	if (Expression_KEY(lhs) == KEYfuncall &&
	    USE_DECL(value_use_use(funcall_f(lhs))) == vdecl) return true;
      }
      break;
    case KEYblock_stmt:
      if (block_assigns_to(block_stmt_body(d),vdecl)) return true;
      break;
    case KEYif_stmt:
      if (block_assigns_to(if_stmt_if_true(d),vdecl) ||
	  block_assigns_to(if_stmt_if_false(d),vdecl)) return true;
      break;
    case KEYcase_stmt:
      for (Match m = first_Match(case_stmt_matchers(d)); m; m = MATCH_NEXT(m)) {
	if (block_assigns_to(matcher_body(m),vdecl)) return true;
      }
      if (block_assigns_to(case_stmt_default(d),vdecl)) return true;
      break;
    case KEYfor_stmt:
      for (Match m = first_Match(for_stmt_matchers(d)); m; m = MATCH_NEXT(m)) {
	if (block_assigns_to(matcher_body(m),vdecl)) return true;
      }
      break;
    case KEYfor_in_stmt:
      if (block_assigns_to(for_in_stmt_body(d),vdecl)) return true;
      break;
    default:
      break;
    }
  }
  return false;
}
