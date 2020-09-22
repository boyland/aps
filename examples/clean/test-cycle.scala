// Generated by aps2scala version 0.3.6
import basic_implicit._;
object test_cycle_implicit {
  val test_cycle_loaded = true;
}
import test_cycle_implicit._;

trait C_TEST_CYCLE[T_Result] extends C_TYPE[T_Result] {
  type T_Wood <: Node;
  val t_Wood : C_PHYLUM[T_Wood];
  val p_branch : PatternFunction[(T_Wood,T_Wood,T_Wood)];
  def v_branch : (T_Wood,T_Wood) => T_Wood;
  val p_leaf : PatternFunction[(T_Wood,T_Integer)];
  def v_leaf : (T_Integer) => T_Wood;
  type T_Integers;
  val t_Integers : C_TYPE[T_Integers]with C_SET[T_Integers,T_Integer];
  type T_IntegerLattice;
  val t_IntegerLattice : C_TYPE[T_IntegerLattice]with C_UNION_LATTICE[T_IntegerLattice,T_Integer,T_Integers];
  def v_leaves : T_IntegerLattice;
  val v_partial : (T_Wood) => T_IntegerLattice;
}

abstract class T_TEST_CYCLE(t : C_TEST_CYCLE[T_TEST_CYCLE]) extends Value(t) { }

class M_TEST_CYCLE(name : String)
  extends I_TYPE[T_TEST_CYCLE](name)
  with C_TEST_CYCLE[T_TEST_CYCLE]
{
  val t_Result : this.type = this;
  abstract class T_Wood(t : I_PHYLUM[T_Wood]) extends Node(t) {
    override def isRooted : Boolean = true;
  }
  val t_Wood = new I_PHYLUM[T_Wood]("Wood");

  case class c_branch(v_x : T_Wood,v_y : T_Wood) extends T_Wood(t_Wood) {
    override def children : List[Node] = List(v_x,v_y);
    override def toString() : String = Debug.with_level {
      "branch("+ v_x + ","+ v_y+ ")";
    }
  }
  def u_branch(x:Any) : Option[(T_Wood,T_Wood,T_Wood)] = x match {
    case x@c_branch(v_x,v_y) => Some((x,v_x,v_y));
    case _ => None };
  val v_branch = f_branch _;
  def f_branch(v_x : T_Wood, v_y : T_Wood):T_Wood = c_branch(v_x,v_y).register;
  val p_branch = new PatternFunction[(T_Wood,T_Wood,T_Wood)](u_branch);

  case class c_leaf(v_x : T_Integer) extends T_Wood(t_Wood) {
    override def children : List[Node] = List();
    override def toString() : String = Debug.with_level {
      "leaf("+ v_x+ ")";
    }
  }
  def u_leaf(x:Any) : Option[(T_Wood,T_Integer)] = x match {
    case x@c_leaf(v_x) => Some((x,v_x));
    case _ => None };
  val v_leaf = f_leaf _;
  def f_leaf(v_x : T_Integer):T_Wood = c_leaf(v_x).register;
  val p_leaf = new PatternFunction[(T_Wood,T_Integer)](u_leaf);

  val t_Integers = new M_SET[T_Integer]("Integers",t_Integer);
  type T_Integers = /*TI*/T_SET[T_Integer];
  val t_IntegerLattice = new M_UNION_LATTICE[T_Integer,T_Integers]("IntegerLattice",t_Integer,t_Integers);
  type T_IntegerLattice = /*TI*/T_UNION_LATTICE[T_Integer,T_Integers];
  private class E_leaves(anchor : Null) extends Evaluation[Null,T_IntegerLattice](anchor,"leaves") with CircularEvaluation[Null,T_IntegerLattice] with CollectionEvaluation[Null,T_IntegerLattice] {
    override def initial : T_IntegerLattice = t_IntegerLattice.v_initial;
    override def combine(v1 : T_IntegerLattice, v2 : T_IntegerLattice) = t_IntegerLattice.v_combine(v1,v2);
    def lattice() : C_LATTICE[T_IntegerLattice] = t_IntegerLattice;

    override def compute : ValueType = c_leaves();
  }
  private object a_leaves extends E_leaves(null) {}
  def v_leaves:T_IntegerLattice = a_leaves.get;

  private class E_partial(anchor : T_Wood) extends Evaluation[T_Wood,T_IntegerLattice](anchor,anchor.toString()+"."+"partial") with CircularEvaluation[T_Wood,T_IntegerLattice] {
    def lattice() : C_LATTICE[T_IntegerLattice] = t_IntegerLattice;

    override def compute : ValueType = c_partial(anchor);
  }
  private object a_partial extends Attribute[T_Wood,T_IntegerLattice](t_Wood,t_IntegerLattice,"partial") {
    override def createEvaluation(anchor : T_Wood) : Evaluation[T_Wood,T_IntegerLattice] = new E_partial(anchor);
  }
  val v_partial : T_Wood => T_IntegerLattice = a_partial.get _;

  def c_partial(anode : T_Wood) : T_IntegerLattice = {
    val anchor = anode;
    anchor match {
      case p_leaf(v_l,v_x) => {
        if (anode eq v_l) return new M__basic_19[ T_Integer,T_IntegerLattice](t_Integer,t_IntegerLattice).v__op_5w(new M__basic_20[ T_Integer,T_IntegerLattice](t_Integer,t_IntegerLattice).v__op_5(v_leaves,v_x),t_IntegerLattice.v_single(new M__basic_4[ T_Integer](t_Integer).v__op_s(v_x,1)));
      }
      case _ => {}
    }
    anchor match {
      case p_branch(v_b,v_x,v_y) => {
        if (anode eq v_b) return new M__basic_19[ T_Integer,T_IntegerLattice](t_Integer,t_IntegerLattice).v__op_5w(v_partial(v_x),v_partial(v_y));
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".partial");
  }
  def c_leaves() : T_IntegerLattice = {
    Debug.begin("leaves");
    try {
      var collection : T_IntegerLattice = t_IntegerLattice.v_initial;
      for (anchor <- t_Wood.nodes) anchor match {
        case p_leaf(v_l,v_x) => {
          collection = t_IntegerLattice.v_combine(collection,t_IntegerLattice.v_single(new M__basic_4[ T_Integer](t_Integer).v__op_u(v_x,1)));
        }
        case _ => {}
      }
      for (anchor <- t_Wood.nodes) anchor match {
        case p_branch(v_b,v_x,v_y) => {
          collection = t_IntegerLattice.v_combine(collection,v_partial(v_x));
        }
        case _ => {}
      }
      Debug.returns(collection.toString());
      return collection;
    } finally { Debug.end(); }
  }
  override def finish() : Unit = {
    a_leaves.get;
    a_partial.finish;
    super.finish();
  }

}
