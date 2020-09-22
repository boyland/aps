// Generated by aps2scala version 0.3.6
import basic_implicit._;
object tiny_impl_implicit {
  val tiny_impl_loaded = true;
import basic_implicit._;
import tiny_implicit._;
type T_TINY_IMPL[T_T] = T_T;
}
import tiny_impl_implicit._;

import basic_implicit._;
import tiny_implicit._;
trait C_TINY_IMPL[T_Result, T_T] extends C_TYPE[T_Result] with C_TINY[T_Result] {
  type T_IntegerSet;
  val t_IntegerSet : C_TYPE[T_IntegerSet]with C_SET[T_IntegerSet,T_Integer];
  type T_CustomLattice;
  val t_CustomLattice : C_TYPE[T_CustomLattice]with C_UNION_LATTICE[T_CustomLattice,T_Integer,T_IntegerSet];
  val v_bag : (T_Wood) => T_CustomLattice;
  val v_rslt : (T_Root) => T_CustomLattice;
  val v_temp : (T_Wood) => T_CustomLattice;
}

class M_TINY_IMPL[T_T](name : String,val t_T : C_TYPE[T_T] with C_TINY[T_T])
  extends Module(name)
  with C_TINY_IMPL[T_T,T_T]
{
  type T_Result = T_T;
  val v_equal = t_T.v_equal;
  val v_string = t_T.v_string;
  val v_assert = t_T.v_assert;
  val v_node_equivalent = t_T.v_node_equivalent;
  type T_Root = t_T.T_Root;
  val t_Root = t_T.t_Root;
  type T_Wood = t_T.T_Wood;
  val t_Wood = t_T.t_Wood;
  val p_root = t_T.p_root;
  val v_root = t_T.v_root;
  val p_branch = t_T.p_branch;
  val v_branch = t_T.v_branch;
  val p_leaf = t_T.p_leaf;
  val v_leaf = t_T.v_leaf;

  val t_Result : this.type = this;
  val t_IntegerSet = new M_SET[T_Integer]("IntegerSet",t_Integer);
  type T_IntegerSet = /*TI*/T_SET[T_Integer];
  val t_CustomLattice = new M_UNION_LATTICE[T_Integer,T_IntegerSet]("CustomLattice",t_Integer,t_IntegerSet);
  type T_CustomLattice = /*TI*/T_UNION_LATTICE[T_Integer,T_IntegerSet];
  private class E_bag(anchor : T_Wood) extends Evaluation[T_Wood,T_CustomLattice](anchor,anchor.toString()+"."+"bag") with CircularEvaluation[T_Wood,T_CustomLattice] with CollectionEvaluation[T_Wood,T_CustomLattice] {
    override def initial : T_CustomLattice = t_CustomLattice.v_initial;
    override def combine(v1 : T_CustomLattice, v2 : T_CustomLattice) = t_CustomLattice.v_combine(v1,v2);
    def lattice() : C_LATTICE[T_CustomLattice] = t_CustomLattice;

  }
  private object a_bag extends Attribute[T_Wood,T_CustomLattice](t_Wood,t_CustomLattice,"bag") {
    override def createEvaluation(anchor : T_Wood) : Evaluation[T_Wood,T_CustomLattice] = new E_bag(anchor);
  }
  val v_bag : T_Wood => T_CustomLattice = a_bag.get _;

  private class E_rslt(anchor : T_Root) extends Evaluation[T_Root,T_CustomLattice](anchor,anchor.toString()+"."+"rslt") with CircularEvaluation[T_Root,T_CustomLattice] with CollectionEvaluation[T_Root,T_CustomLattice] {
    override def initial : T_CustomLattice = t_CustomLattice.v_initial;
    override def combine(v1 : T_CustomLattice, v2 : T_CustomLattice) = t_CustomLattice.v_combine(v1,v2);
    def lattice() : C_LATTICE[T_CustomLattice] = t_CustomLattice;

  }
  private object a_rslt extends Attribute[T_Root,T_CustomLattice](t_Root,t_CustomLattice,"rslt") {
    override def createEvaluation(anchor : T_Root) : Evaluation[T_Root,T_CustomLattice] = new E_rslt(anchor);
  }
  val v_rslt : T_Root => T_CustomLattice = a_rslt.get _;

  private class E_temp(anchor : T_Wood) extends Evaluation[T_Wood,T_CustomLattice](anchor,anchor.toString()+"."+"temp") with CircularEvaluation[T_Wood,T_CustomLattice] with CollectionEvaluation[T_Wood,T_CustomLattice] {
    override def initial : T_CustomLattice = t_CustomLattice.v_initial;
    override def combine(v1 : T_CustomLattice, v2 : T_CustomLattice) = t_CustomLattice.v_combine(v1,v2);
    def lattice() : C_LATTICE[T_CustomLattice] = t_CustomLattice;

  }
  private object a_temp extends Attribute[T_Wood,T_CustomLattice](t_Wood,t_CustomLattice,"temp") {
    override def createEvaluation(anchor : T_Wood) : Evaluation[T_Wood,T_CustomLattice] = new E_temp(anchor);
  }
  val v_temp : T_Wood => T_CustomLattice = a_temp.get _;

  def visit_0_1(node : T_Root) : Unit = node match {
    case p_root(_,_) => visit_0_1_0(node);
  };
  def visit_0_1_0(anchor : T_Root) : Unit = anchor match {
    case p_root(v_p,v_b) => {
      // p.G[Root]'shared_info is ready now.
      // shared info for b is ready.
      visit_1_1(v_b);
      // b.temp is ready now.
      // b.bag is ready now.
      a_rslt.set(v_p,a_bag.get(v_b));
    }
  }


  def visit_1_1(node : T_Wood) : Unit = node match {
    case p_leaf(_,_) => visit_1_1_1(node);
    case p_branch(_,_,_) => visit_1_1_0(node);
  };
  def visit_1_1_1(anchor : T_Wood) : Unit = anchor match {
    case p_leaf(v_l,v_x) => {
      // l.G[Wood]'shared_info is ready now.
      a_temp.set(v_l,t_CustomLattice.v_single(2));
      a_bag.set(v_l,a_temp.get(v_l));
    }
  }

  def visit_1_1_0(anchor : T_Wood) : Unit = anchor match {
    case p_branch(v_b,v_x,v_y) => {
      // b.G[Wood]'shared_info is ready now.
      // b.temp is ready.
      // shared info for x is ready.
      visit_1_1(v_x);
      // x.temp is ready now.
      // x.bag is ready now.
      // shared info for y is ready.
      visit_1_1(v_y);
      // y.temp is ready now.
      // y.bag is ready now.
      a_bag.set(v_b,new M__basic_19[ T_Integer,T_CustomLattice](t_Integer,t_CustomLattice).v__op_5w(new M__basic_19[ T_Integer,T_CustomLattice](t_Integer,t_CustomLattice).v__op_5w(a_temp.get(v_x),a_bag.get(v_y)),a_temp.get(v_y)));
    }
  }


  def visit() : Unit = {
    val roots = t_Root.nodes;
    // shared info for TINY_IMPL is ready.
    for (root <- roots) {
      visit_0_1(root);
    }
    // TINY_IMPL.rslt is ready now.
  }

  override def finish() : Unit = {
    visit();
    t_IntegerSet.finish();
    t_CustomLattice.finish();
super.finish();
  }

}

