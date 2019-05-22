object TestCollDriver extends App
{
    val m = new M_TEST_COLL("Tiny");
    type T_Tiny = m.T_Result;
    val t_Tiny = m.t_Result;

    val w = t_Tiny.v_branch(t_Tiny.v_leaf(3),t_Tiny.v_leaf(4));

    Debug.activate();

    m.finish();

    println("sum is " + t_Tiny.v_sum);
    println("leaves is " + t_Tiny.v_leaves);
}
