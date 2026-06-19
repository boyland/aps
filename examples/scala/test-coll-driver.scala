object TestCollDriver extends App
{
    val m = new M_TINY("Tiny");
    val t_Tiny = m.t_Result;
    val p = new TinyParser(m);

    val w = if (args.length > 0) {
      p.parseFile(args(0))
    } else {
      t_Tiny.f_root(t_Tiny.f_branch(t_Tiny.f_leaf(3),t_Tiny.f_leaf(4)))
    };

    if (args.contains("--debug")) Debug.activate();

    m.finish();

    val m2 = new M_TEST_COLL("Test Coll",m);
    val w2 = w.asInstanceOf[m2.T_Root];

    m2.finish();

    println("Results:");
    println("sum is " + m2.v_sum);
    println("leaves is " + m2.v_leaves);
    println("result is " + m2.v_result(w2));
}
