/* Driver to run classic driver code */

object Classic extends Application {
  val m_simple = new M_SIMPLE("Simple");
  val m_binding = new M_NAME_RESOLUTION[m_simple.T_Result]("Binding",m_simple.t_Result);
  val t_binding = m_binding.t_Result;
  val t_simple = t_binding; // = m_simple.t_Result; // scala type problems
  val ds =
    t_simple.v_xcons_decls(t_simple.v_no_decls(),
			   t_simple.v_decl("x",t_simple.v_integer_type()));
  val s =
    t_simple.v_assign_stmt(t_simple.v_variable("x"),t_simple.v_variable("y"));
  val ss = t_simple.v_xcons_stmts(t_simple.v_no_stmts(),s);
					    
  val p = t_simple.v_program(t_simple.v_block(ds,ss));

  Debug.activate();

  m_simple.finish();
  m_binding.finish();
  println("Messages:");
  for (m <- t_binding.v_prog_msgs(p)) {
    println(m);
  }
}
