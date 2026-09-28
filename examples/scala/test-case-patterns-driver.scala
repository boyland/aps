object TestCasePatternsDriver extends App {
  val tree = new M_TINY("Tiny")
  val parser = new TinyParser(tree)
  val root = if (args.length > 0) {
    parser.parseFile(args(0))
  } else {
    tree.v_root(tree.v_branch(tree.v_leaf(3), tree.v_leaf(9)))
  }
  tree.finish()

  val first = new M_TEST_CASE_FIRST[tree.T_Result]("Case First", tree)
  val each = new M_TEST_CASE_EACH[tree.T_Result]("Case Each", tree)
  val last = new M_TEST_CASE_LAST[tree.T_Result]("Case Last", tree)
  val firstLast = new M_TEST_CASE_FIRST_LAST[tree.T_Result]("Case First Last", tree)
  val firstEach = new M_TEST_CASE_FIRST_EACH[tree.T_Result]("Case First Each", tree)
  val eachLast = new M_TEST_CASE_EACH_LAST[tree.T_Result]("Case Each Last", tree)
  val firstEachLast = new M_TEST_CASE_FIRST_EACH_LAST[tree.T_Result]("Case First Each Last", tree)
  val separatedSpan = new M_TEST_CASE_SEPARATED_SPAN[tree.T_Result]("Case Separated Span", tree)
  val adjacentSpan = new M_TEST_CASE_ADJACENT_SPAN[tree.T_Result]("Case Adjacent Span", tree)
  val endpointMiddles = new M_TEST_CASE_ENDPOINT_MIDDLES[tree.T_Result]("Case Endpoint Middles", tree)

  first.finish()
  each.finish()
  last.finish()
  firstLast.finish()
  firstEach.finish()
  eachLast.finish()
  firstEachLast.finish()
  separatedSpan.finish()
  adjacentSpan.finish()
  endpointMiddles.finish()

  println("Results:")
  println(first.v_result(root.asInstanceOf[first.T_Root]))
  println(each.v_result(root.asInstanceOf[each.T_Root]))
  println(last.v_result(root.asInstanceOf[last.T_Root]))
  println(firstLast.v_result(root.asInstanceOf[firstLast.T_Root]))
  println(firstEach.v_result(root.asInstanceOf[firstEach.T_Root]))
  println(eachLast.v_result(root.asInstanceOf[eachLast.T_Root]))
  println(firstEachLast.v_result(root.asInstanceOf[firstEachLast.T_Root]))
  println(separatedSpan.v_result(root.asInstanceOf[separatedSpan.T_Root]))
  println(adjacentSpan.v_result(root.asInstanceOf[adjacentSpan.T_Root]))
  println(endpointMiddles.v_result(root.asInstanceOf[endpointMiddles.T_Root]))
}
