/* APS-TRAVERSE.H
 *
 * Code to assist in walking over APS specifications.
 * Each traversal function takes a func with two arguments,
 * and it also takes the first argument to be passed to it.
 * The second argument is the node.  If the function returns NULL
 * then the traversal function returns: essentially:
 *   traverse_XXX(func,arg,node) == func(arg,node)
 * But if a non-null value is returned (typically the same as the second
 * argument, then we continue the traversal using this value
 * as the first argument.
 *
 * For example:
 *    void *count(void *accum, void *node) {
 *      ++ *((int *)accum);
 *      return accum;
 *    }
 *
 *    int count_nodes_in_program(Program p) {
 *      int total;
 *      traverse_Program(count,&total,p);
 *      return total;
 *    }
 */

typedef void *(*TRAVERSEFUNC)(void *,void *);

extern void traverse_Signature(TRAVERSEFUNC,void *,Signature);
extern void traverse_Type(TRAVERSEFUNC,void *,Type);
extern void traverse_Expression(TRAVERSEFUNC,void *,Expression);
extern void traverse_Pattern(TRAVERSEFUNC,void *,Pattern);
extern void traverse_Module(TRAVERSEFUNC,void *,Module);
extern void traverse_Class(TRAVERSEFUNC,void *,Class);
extern void traverse_Def(TRAVERSEFUNC,void *,Def);
extern void traverse_Use(TRAVERSEFUNC,void *,Use);
extern void traverse_Program(TRAVERSEFUNC,void *,Program);
extern void traverse_Unit(TRAVERSEFUNC,void *,Unit);
extern void traverse_Declaration(TRAVERSEFUNC,void *,Declaration);
extern void traverse_Block(TRAVERSEFUNC,void *,Block);
extern void traverse_Match(TRAVERSEFUNC,void *,Match);
extern void traverse_Direction(TRAVERSEFUNC,void *,Direction);
extern void traverse_Default(TRAVERSEFUNC,void *,Default);
extern void traverse_Units(TRAVERSEFUNC,void *,Units);
extern void traverse_Declarations(TRAVERSEFUNC,void *,Declarations);
extern void traverse_Matches(TRAVERSEFUNC,void *,Matches);
extern void traverse_Types(TRAVERSEFUNC,void *,Types);
extern void traverse_Expressions(TRAVERSEFUNC,void *,Expressions);
extern void traverse_Patterns(TRAVERSEFUNC,void *,Patterns);
extern void traverse_Actuals(TRAVERSEFUNC,void *,Actuals);
extern void traverse_TypeActuals(TRAVERSEFUNC,void *,TypeActuals);
extern void traverse_PatternActuals(TRAVERSEFUNC,void *,PatternActuals);

extern void traverse_Signature_skip(TRAVERSEFUNC,void *,Signature);
extern void traverse_Type_skip(TRAVERSEFUNC,void *,Type);
extern void traverse_Expression_skip(TRAVERSEFUNC,void *,Expression);
extern void traverse_Pattern_skip(TRAVERSEFUNC,void *,Pattern);
extern void traverse_Module_skip(TRAVERSEFUNC,void *,Module);
extern void traverse_Class_skip(TRAVERSEFUNC,void *,Class);
extern void traverse_Def_skip(TRAVERSEFUNC,void *,Def);
extern void traverse_Use_skip(TRAVERSEFUNC,void *,Use);
extern void traverse_Program_skip(TRAVERSEFUNC,void *,Program);
extern void traverse_Unit_skip(TRAVERSEFUNC,void *,Unit);
extern void traverse_Declaration_skip(TRAVERSEFUNC,void *,Declaration);
extern void traverse_Block_skip(TRAVERSEFUNC,void *,Block);
extern void traverse_Match_skip(TRAVERSEFUNC,void *,Match);
extern void traverse_Direction_skip(TRAVERSEFUNC,void *,Direction);
extern void traverse_Default_skip(TRAVERSEFUNC,void *,Default);
extern void traverse_Units_skip(TRAVERSEFUNC,void *,Units);
extern void traverse_Declarations_skip(TRAVERSEFUNC,void *,Declarations);
extern void traverse_Matches_skip(TRAVERSEFUNC,void *,Matches);
extern void traverse_Types_skip(TRAVERSEFUNC,void *,Types);
extern void traverse_Expressions_skip(TRAVERSEFUNC,void *,Expressions);
extern void traverse_Patterns_skip(TRAVERSEFUNC,void *,Patterns);
extern void traverse_Actuals_skip(TRAVERSEFUNC,void *,Actuals);
extern void traverse_TypeActuals_skip(TRAVERSEFUNC,void *,TypeActuals);
extern void traverse_PatternActuals_skip(TRAVERSEFUNC,void *,PatternActuals);
