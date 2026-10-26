object SimpleBindingDriver extends App {
  var tree: M_SIMPLE = null
  if (args.length == 0) {
    tree = new M_SIMPLE("Simple")
    val simple = tree.t_Result
    val declarations = simple.v_xcons_decls(
      simple.v_no_decls(),
      simple.v_decl("x", simple.v_integer_type()))
    val statement = simple.v_assign_stmt(
      simple.v_variable("x"),
      simple.v_variable("y"))
    val statements = simple.v_xcons_stmts(simple.v_no_stmts(), statement)
    simple.v_program(simple.v_block(declarations, statements))
  } else {
    val scanner = new SimpleScanner(new java.io.FileReader(args(0)))
    val parser = new SimpleParser()
    parser.reset(scanner, args(0))
    if (!parser.yyparse()) {
      println("Errors found.")
      System.exit(1)
    }
    tree = parser.getTree()
  }

  val simpleTree = tree
  val binding = new M_NAME_RESOLUTION[simpleTree.T_Result]("Binding", simpleTree.t_Result)

  simpleTree.finish()
  binding.finish()

  println("Results:")
  binding.v_msgs.toSeq.sorted.foreach(println)
}