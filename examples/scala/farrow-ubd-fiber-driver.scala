object FarrowUbdFiberDriver extends App {

  val symb = new M_SYMBOL("Symbol")
  symb.finish()

  val ss = new FarrowUbdScanner(new java.io.FileReader(args(0)));
  val sp = new FarrowUbdParser();
  sp.reset(ss, args(0));
  if (!sp.yyparse()) {
    println("Errors found.\n");
    System.exit(1);
  }
  var farrow_ubd_tree = sp.getTree();
  val p = farrow_ubd_tree.t_Program;
  
  val m_farrow_ubd_tree = farrow_ubd_tree;
  val m_farrow_ubd = new M_FARROW_UBD_FIBER[m_farrow_ubd_tree.T_Result]("FarrowUbdFiber", m_farrow_ubd_tree.t_Result);

  Debug.activate();

  m_farrow_ubd_tree.finish();
  m_farrow_ubd.finish();

}
