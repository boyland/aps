object BroadFiberCycleDriver extends App
{
    val m = new M_TINY("Tiny");
    type T_Tiny = m.T_Result;
    val t_Tiny = m.t_Result;

    val w = t_Tiny.v_root(t_Tiny.v_branch(t_Tiny.v_leaf(3),t_Tiny.v_leaf(4)));

    Debug.activate();

    m.finish();

    val m2 = new M_BROAD_FIBER_CYCLE("Test broad",m);
    val w2 = w.asInstanceOf[m2.T_Root];

    m2.finish();

    println("answer is " + m2.v_answer);
}
