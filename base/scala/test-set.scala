import basic_implicit._;

object TestSet extends App {
  val t_Set = new M_SET[T_Integer]("SET", t_Integer);

  val l1 = Set(1,2,3,2,1,4,5,4,3,2,2,2);
  val l2 = Set(0,1,2,3,4,5,2);
  val l1_clone = l1 ++ Set();

  println("union = "+ t_Set.v_union(l1,l2));
  println("intersect = " + t_Set.v_intersect(l1,l2));
  println("difference = " + t_Set.v_difference(l1,l2));
  println("combine = "+ t_Set.v_combine(l1,l2));
  println("equal(l1,l1) = " + t_Set.v_equal(l1,l1));
  println("equal(l1,l1_clone) = " + t_Set.v_equal(l1,l1_clone));
  println("equal(l1,l2) = " + t_Set.v_equal(l1,l2));
}
