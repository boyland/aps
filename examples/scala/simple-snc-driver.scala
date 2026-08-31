object SimpleSncDriver extends App {
  var simple_tree : M_SIMPLE = null;
  var p: Any = _;
  if (args.length == 0) {
    simple_tree = new M_SIMPLE("Simple");
    val t_simple = simple_tree.t_Result;
    val ds = t_simple.v_xcons_decls(t_simple.v_no_decls(),
                t_simple.v_decl("x",t_simple.v_integer_type()));
    val s =	t_simple.v_assign_stmt(t_simple.v_intconstant(3),
                t_simple.v_intconstant(5));
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
  val m_snc = new M_SIMPLE_SNC[m_simple.T_Result]("SimpleSNC", m_simple.t_Result);
  val t_snc = m_snc.t_Result;

  if (args.contains("--debug")) Debug.activate();

  m_simple.finish();
  m_snc.finish();

  println("Results:");
  println("program_total is " + t_snc.v_program_total(t_snc.t_Program.nodes(0)));
}
