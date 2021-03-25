object GrammarDriver extends App {
  Debug.activate();

  val symb = new M_SYMBOL("Symbol")
  symb.finish()

  val grammar = new M_GRAMMAR("Grammar")
  // Z -> Y X
  // Y -> y
  // X -> x
  // X -> Z z
  val t_Y = grammar.v_nonterminal(symb.v_create("Y"))
  val t_X = grammar.v_nonterminal(symb.v_create("X"))
  val t_Z = grammar.v_nonterminal(symb.v_create("Z"))

  val t_y = grammar.v_terminal(symb.v_create("y"))
  val t_x = grammar.v_terminal(symb.v_create("x"))
  val t_z = grammar.v_terminal(symb.v_create("z"))

  val prod1 = grammar.v_prod(
    symb.v_create("Z"),
    grammar.t_Items.v_append(grammar.t_Items.v_single(t_Y), grammar.t_Items.v_single(t_X))
  )
  val prod2 = grammar.v_prod(symb.v_create("Y"), grammar.t_Items.v_single(t_y))
  val prod3 = grammar.v_prod(symb.v_create("X"), grammar.t_Items.v_single(t_x))
  val prod4 = grammar.v_prod(
    symb.v_create("X"),
    grammar.t_Items.v_append(grammar.t_Items.v_single(t_Z), grammar.t_Items.v_single(t_z))
  )

  val program = grammar.t_Productions.v_append(
    grammar.t_Productions.v_append(
      grammar.t_Productions.v_single(prod1),
      grammar.t_Productions.v_single(prod2)
    ),
    grammar.t_Productions.v_append(
      grammar.t_Productions.v_single(prod3),
      grammar.t_Productions.v_single(prod4)
    )
  )

  val root = grammar.v_grammar(program)

  grammar.finish()

  val first = new M_FIRST("First", grammar)
  val follow = new M_FOLLOW("Follow", grammar)

  first.finish()
  follow.finish()
}
