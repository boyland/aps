/* Driver to run classic driver code */

object Classic extends App {
  var simple_tree : M_SIMPLE = null;
  var p: Any = _;
  if (args.length == 0) {
    simple_tree = new M_SIMPLE("Simple");
    val t_simple = simple_tree.t_Result;
    val ds =
	t_simple.v_xcons_decls(t_simple.v_no_decls(),
			       t_simple.v_decl("x",t_simple.v_integer_type()));
    val s =
	t_simple.v_assign_stmt(t_simple.v_variable("x"),
			       t_simple.v_variable("y"));
    val ss = t_simple.v_xcons_stmts(t_simple.v_no_stmts(),s);
    p = t_simple.v_program(t_simple.v_block(ds,ss));
  } else {
    val ss = new SimpleScanner(new java.io.FileReader(args(0)));
    val sp = new SimpleParser();
    sp.reset(ss, args(0));
    if (!sp.yyparse()) {
      println("Errors found.\n");
      System.exit(1);
    }
    simple_tree = sp.getTree();
    p = simple_tree.t_Program;
  }
  
  val m_simple = simple_tree;
  val m_binding = new M_NAME_RESOLUTION[m_simple.T_Result]("Binding",m_simple.t_Result);
  val t_binding = m_binding.t_Result;

  Debug.activate();

  m_simple.finish();
  m_binding.finish();
  println("Messages:");

  var program: t_binding.T_Program = p.asInstanceOf[t_binding.T_Program];
  for (m <- t_binding.v_prog_msgs(program)) {
    println(m);
  }
}
