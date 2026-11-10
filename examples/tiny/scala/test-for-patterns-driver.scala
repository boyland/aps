object TestForPatternsDriver extends App {
  val tree = new M_TINY("Tiny")
  val parser = new TinyParser(tree)
  val root = if (args.length > 0) {
    parser.parseFile(args(0))
  } else {
    tree.v_root(tree.v_branch(tree.v_leaf(3), tree.v_leaf(9)))
  }
  val test = new M_TEST_FOR_PATTERNS[tree.T_Result]("Test For Patterns", tree)

  tree.finish()
  test.finish()

  val testRoot = root.asInstanceOf[test.T_Root]
  val results = Seq(
    test.v_first(testRoot),
    test.v_each(testRoot),
    test.v_last(testRoot),
    test.v_first_last(testRoot),
    test.v_first_each(testRoot),
    test.v_each_last(testRoot),
    test.v_first_each_last(testRoot),
    test.v_separated_span(testRoot),
    test.v_adjacent_span(testRoot),
    test.v_endpoint_middles(testRoot))

  println("Results:")
  results.foreach(println)
}
