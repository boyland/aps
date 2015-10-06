import basic_implicit._;

object TestMultiSet extends App {
  val t_MI = new M_MULTISET[T_Integer]("MI",t_Integer);

  val l1 = List(1,2,3,2,1,4,5,4,3,2,2,2);
  val l2 = List(0,1,2,3,4,5,2);

  println("union = "+ t_MI.v_union(l1,l2));
  println("intersect = " + t_MI.v_intersect(l1,l2));
  println("difference = " + t_MI.v_difference(l1,l2));
}
