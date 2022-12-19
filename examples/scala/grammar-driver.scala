object GrammarDriver extends App {

  val symb = new M_SYMBOL("Symbol")
  symb.finish()

  val ss = new GrammarScanner(new java.io.FileReader(args(0)));
  val sp = new GrammarParser();
  sp.reset(ss, args(0));
  if (!sp.yyparse()) {
    println("Errors found.\n");
    System.exit(1);
  }
  var grammar_tree = sp.getTree();
  val p = grammar_tree.t_Grammar;
  
  val m_grammar = grammar_tree;
  val m_first = new M_FIRST[m_grammar.T_Result]("First", m_grammar.t_Result);
  val m_follow = new M_FOLLOW[m_grammar.T_Result]("Follow", m_grammar.t_Result);
  val m_nullable = new M_NULLABLE[m_grammar.T_Result]("Nullable", m_grammar.t_Result);

  Debug.activate();

  m_grammar.finish();
  m_first.finish();
  m_follow.finish();
  m_nullable.finish();

}
