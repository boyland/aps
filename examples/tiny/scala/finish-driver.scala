object FinishDriver extends App {
  val tree = new M_TINY("Tiny")
  val t = tree.t_Result;
  val branch = t.v_branch(t.v_leaf(1), t.v_leaf(2))
  t.v_root(branch)

  val module = new M_TEST_FINISH[tree.T_Result]("Finish", tree.t_Result)
  tree.finish()
  Debug.activate()
  module.finish()

  println("Results:")
  println("result is " + module.v_result(module.t_Root.nodes(0)))
  println(
    "unreachable is " +
      module.v_unreachable(branch.asInstanceOf[module.T_Wood]))
}
