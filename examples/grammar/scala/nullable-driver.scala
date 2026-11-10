object NullableDriver extends App {

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
  val nullable = new M_NULLABLE[grammar.T_Result]("Nullable", grammar.t_Result)

  grammar.finish()
  nullable.finish()

  val results = nullable.v_nullableTable.toSeq
    .map { case (key, value) => (key.name, value) }
    .sortBy(_._1)

  println("Results:")
  results
    .foreach { case (key, value) => println(s"$key: $value") }
}