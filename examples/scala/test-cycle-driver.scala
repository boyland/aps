object TestCycleDriver extends App
{
    val m = new M_TINY("Tiny");
    type T_Tiny = m.T_Result;
    val t_Tiny = m.t_Result;

    val w = t_Tiny.v_branch(t_Tiny.v_leaf(3),t_Tiny.v_leaf(4));

    Debug.activate();

    m.finish();

    val m2 = new M_TEST_CYCLE("Test Cycle",m);
    val w2 = w.asInstanceOf[m2.T_Wood];

    println("leaves is " + m2.v_leaves);
}
