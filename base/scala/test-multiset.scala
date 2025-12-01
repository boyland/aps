import basic_implicit._;

object TestMultiSet extends App {
  val t_MI = new M_MULTISET[T_Integer]("MI",t_Integer);

  val l1 = List(1,2,3,2,1,4,5,4,3,2,2,2);
  val l2 = List(0,1,2,3,4,5,2);
  val l1_clone = l1 ++ Set();

  println("union = "+ t_MI.v_union(l1,l2));
  println("intersect = " + t_MI.v_intersect(l1,l2));
  println("difference = " + t_MI.v_difference(l1,l2));
  println("combine = "+ t_MI.v_combine(l1,l2));
  println("count(2) = " + t_MI.v_count(2,l1));
  println("equal(l1,l1) = " + t_MI.v_equal(l1,l1));
  println("equal(l1,l1_clone) = " + t_MI.v_equal(l1,l1_clone));
  println("equal(l1,l2) = " + t_MI.v_equal(l1,l2));
}
