object TinyCircularTests extends App
{
    val m = new M_TINY_CIRCULAR("TinyCircular");
    type T_Tiny = m.T_Result;
    val t_Tiny = m.t_Result;

    val b = t_Tiny.v_branch(t_Tiny.v_leaf(3),t_Tiny.v_leaf(4));
    val r = t_Tiny.v_root(b);

    Debug.activate();

    m.finish();

    val classDef = new TINY_CIRCULAR("Test Cycle",m);
    val instance = w.asInstanceOf[classDef.T_Root];

    println("leaves is " + m2);
}
