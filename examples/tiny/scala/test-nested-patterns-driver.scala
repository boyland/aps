object TestNestedPatternsDriver extends App {
  val tree = new M_TINY("Tiny")
  val parser = new TinyParser(tree)
  val root = if (args.length > 0) {
    parser.parseFile(args(0))
  } else {
    tree.v_root(tree.v_branch(tree.v_leaf(3), tree.v_leaf(9)))
  }
  val test = new M_TEST_NESTED_PATTERNS[tree.T_Result](
    "Test Nested Patterns",
    tree
  )

  tree.finish()
  test.finish()

  val testRoot = root.asInstanceOf[test.T_Root]
  val results = Seq(
    test.v_case_in_for(testRoot),
    test.v_for_in_case(testRoot))

  println("Results:")
  results.foreach(println)
}
