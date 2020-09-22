object TinyCircularDriver extends App
{
    val m = new M_TINY("Tiny");
    type T_Tiny = m.T_Result;
    val t_Tiny = m.t_Result;

    val b = t_Tiny.v_branch(t_Tiny.v_leaf(3),t_Tiny.v_leaf(4));
    val r = t_Tiny.v_root(b);

    Debug.activate();

    m.finish();

    val m2 = new M_TINY_CIRCULAR_SIMPLE("Test Simple Cycle",m);
    val root = r.asInstanceOf[m2.T_Root];

    m2.finish();

    println("s: " + m2.v_s);

    println("Hello world!")
}
