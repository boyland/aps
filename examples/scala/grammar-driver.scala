object GrammarDriver extends App {

  val symb = new M_SYMBOL("Symbol")
  symb.finish()

  var grammar_tree : M_GRAMMAR = null;
  var p: Any = _;
  if (args.length == 0) {
    grammar_tree = new M_GRAMMAR("Grammar");
    val t_grammar = grammar_tree.t_Result;

    // Z -> Y X
    // Y -> y
    // X -> x
    // X -> Z z
    val t_Y = t_grammar.v_nonterminal(symb.v_create("Y"))
    val t_X = t_grammar.v_nonterminal(symb.v_create("X"))
    val t_Z = t_grammar.v_nonterminal(symb.v_create("Z"))

    val t_y = t_grammar.v_terminal(symb.v_create("y"))
    val t_x = t_grammar.v_terminal(symb.v_create("x"))
    val t_z = t_grammar.v_terminal(symb.v_create("z"))

    val prod1 = t_grammar.v_prod(
      symb.v_create("Z"),
      t_grammar.t_Items.v_append(t_grammar.t_Items.v_single(t_Y), t_grammar.t_Items.v_single(t_X))
    )
    val prod2 = t_grammar.v_prod(symb.v_create("Y"), t_grammar.t_Items.v_single(t_y))
    val prod3 = t_grammar.v_prod(symb.v_create("X"), t_grammar.t_Items.v_single(t_x))
    val prod4 = t_grammar.v_prod(
      symb.v_create("X"),
      t_grammar.t_Items.v_append(t_grammar.t_Items.v_single(t_Z), t_grammar.t_Items.v_single(t_z))
    )

    val program = t_grammar.t_Productions.v_append(
      t_grammar.t_Productions.v_append(
        t_grammar.t_Productions.v_single(prod1),
        t_grammar.t_Productions.v_single(prod2)
      ),
      t_grammar.t_Productions.v_append(
        t_grammar.t_Productions.v_single(prod3),
        t_grammar.t_Productions.v_single(prod4)
      )
    )

    p = t_grammar.v_grammar(program)
  } else {
    var ss = new GrammarScanner(new java.io.FileReader(args(0)));

    while(ss.hasNext()) println(ss.next())

    ss = new GrammarScanner(new java.io.FileReader(args(0)));
    val sp = new GrammarParser();
    sp.reset(ss, args(0));
    if (!sp.yyparse()) {
      println("Errors found.\n");
      System.exit(1);
    }
    grammar_tree = sp.getTree();
    p = grammar_tree.t_Grammar;
    println(grammar_tree.t_Grammar.nodes(0))
  }
  
  val m_grammar = grammar_tree;
  val m_first_binding = new M_FIRST[m_grammar.T_Result]("First",m_grammar.t_Result);
  val m_follow_binding = new M_FOLLOW[m_grammar.T_Result]("Follow",m_grammar.t_Result);
  val m_nullable_binding = new M_NULLABLE[m_grammar.T_Result]("Nullable",m_grammar.t_Result);

  val t_first_binding = m_first_binding.t_Result;
  val t_follow_binding = m_follow_binding.t_Result;
  val t_nullable_binding = m_nullable_binding.t_Result;

  Debug.activate();

  m_grammar.finish();
  m_first_binding.finish();
  m_follow_binding.finish();
  m_nullable_binding.finish();

}
