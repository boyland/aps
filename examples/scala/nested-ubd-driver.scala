object NestedUbdDriver extends App {

  val symb = new M_SYMBOL("Symbol")
  symb.finish()

  val ss = new NestedUbdScanner(new java.io.FileReader(args(0)));
  val sp = new NestedUbdParser();
  sp.reset(ss, args(0));
  if (!sp.yyparse()) {
    println("Errors found.\n");
    System.exit(1);
  }
  var nested_ubd_tree = sp.getTree();
  val p = nested_ubd_tree.t_Program;
  
  val m_nested_ubd_tree = nested_ubd_tree;
  val m_nested_ubd = new M_NESTED_UBD[m_nested_ubd_tree.T_Result]("NestedUbd", m_nested_ubd_tree.t_Result);

  Debug.activate();

  m_nested_ubd_tree.finish();
  m_nested_ubd.finish();

  println(m_nested_ubd.v_program_errs(m_nested_ubd.t_Program.nodes(0)));
}
