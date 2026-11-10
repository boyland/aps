object BelowFiberCycleDriver extends App
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

    val m2 = new M_BELOW_FIBER_CYCLE("Test below",m);
    val w2 = w.asInstanceOf[m2.T_Root];

    m2.finish();

    println("Results:");
    println("answer is " + m2.v_answer(w2));
}
