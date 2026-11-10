object FirstDriver extends App {

  val symb = new M_SYMBOL("Symbol")
  symb.finish()

  val ss = new GrammarScanner(new java.io.FileReader(args(0)))
  val sp = new GrammarParser()
  sp.reset(ss, args(0))
  if (!sp.yyparse()) {
    println("Errors found.\n")
    System.exit(1)
  }
  val grammarTree = sp.getTree()

  val grammar = grammarTree
  val first = new M_FIRST[grammar.T_Result]("First", grammar.t_Result)

  grammar.finish()
  first.finish()

  val results = first.v_firstTable.toSeq
    .map { case (key, value) => (key.name, value.map(_.name).toSeq.sorted) }
    .sortBy(_._1)

  println("Results:")
  results
    .foreach { case (key, value) => println(s"$key: ${value.mkString(", ")}") }
}