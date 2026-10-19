object BiggerBindingDriver extends App {
  if (args.length == 0) {
    Console.err.println("usage: BiggerBindingDriver <program> [--debug]")
    System.exit(2)
  }

  val scanner = new BiggerScanner(new java.io.FileReader(args(0)))
  val parser = new BiggerParser()
  parser.reset(scanner, args(0))
  if (!parser.yyparse()) {
    Console.err.println("Errors found.")
    System.exit(1)
  }

  val biggerTree = parser.getTree()
  val codeGeneration =
    new M_CODE_GENERATION[biggerTree.T_Result](
      "CodeGeneration", biggerTree.t_Result)

  if (args.contains("--debug")) Debug.activate()

  biggerTree.finish()
  codeGeneration.finish()

  println("Results:")
  codeGeneration.v_msgs.toSeq.sorted.foreach(println)
  println("size is " + codeGeneration.v_size)
}
