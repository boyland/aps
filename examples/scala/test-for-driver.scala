object TestForDriver extends App {
  val tree = new M_TINY("Tiny")
  val parser = new TinyParser(tree)
  val root = if (args.length > 0) {
    parser.parseFile(args(0))
  } else {
    tree.v_root(tree.v_branch(tree.v_leaf(3), tree.v_leaf(9)))
  }
  val test = new M_TEST_FOR[tree.T_Result]("Test For", tree)

  tree.finish()
  test.finish()

  println("Results:")
  println(test.v_answer(root.asInstanceOf[test.T_Root]))
}