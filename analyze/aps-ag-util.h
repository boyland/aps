/** We require that top-level matches have the form
 *     match ?name:Type=constructor(?arg1,?arg2,...)
 * and provide the following functions for accessing the pieces.
 */
extern Declaration top_level_match_lhs_decl(Declaration);
extern Declaration top_level_match_constructor_decl(Declaration);
extern Declaration top_level_match_first_rhs_decl(Declaration);
extern Declaration match_lhs_decl(Match);
extern Declaration match_constructor_decl(Match);
extern Declaration match_first_rhs_decl(Match);
extern Declaration next_rhs_decl(Declaration);



