// APS runtime system
// September, 2010
// John Boyland

import scala.collection.mutable.Buffer;
import scala.collection.mutable.ListBuffer;
import scala.collection.mutable.ArrayBuffer;

object Debug {
  private var depth : Int = 0;
  private var _active : Boolean = false;
  private var print_level : Int = 3;

  def active = _active;
  
  def activate() : Unit = _active = true;

  def indent() {
    for (i <- 0 until depth)
      print(' ');
  }

  def begin(s : String) = {
    out(s);
    depth += 1;
  };

  def out(s : String) = {
    if (active) {
      indent();
      println(s);
    }
  }

  def end() = {
    depth -= 1;
  }

  def returns(s : Any) = {
    if (active) {
      indent();
      println("=> " + s);
    }
  }

  def with_level(s : => String) : String =
    if (print_level <= 0) "#"
    else {
      try {
	print_level -= 1;
	s
      } finally {
	print_level += 1;
      }
    }
      
}

class Module(val mname : String) {
  private var complete : Boolean = false;
  def finish() : Unit = {
    if (Debug.active) {
      println("Module " + mname + " is now complete.");
    }
    complete = true; 
  }
  def isComplete : Boolean = complete;
};
 
abstract class Type
{
  def getType : C_TYPE[_];
}

trait C_TYPE[T_Result] extends C_BASIC[T_Result] with C_PRINTABLE[T_Result]
{
  val v_assert : (T_Result) => Unit;
  val v_equal : (T_Result,T_Result) => Boolean;
  val v_node_equivalent : (T_Result,T_Result) => Boolean;
  val v_string : (T_Result) => String;
}

class I_TYPE[T_Result](n : String) extends Module(n) with C_TYPE[T_Result] {
  val v_assert = f_assert _;
  val v_equal = f_equal _;
  val v_node_equivalent = f_node_equivalent _;
  val v_string = f_string _;

  def f_assert(x : T_Result) : Unit =
    x match {
      case t:Type => assert(t.getType == this)
    };
  def f_equal(x : T_Result, y : T_Result) : Boolean = f_node_equivalent(x,y);
  def f_node_equivalent(x : T_Result, y : T_Result) : Boolean = x.equals(y);
  def f_string(x : T_Result) : String = x.toString();
}
    
class M_TYPE extends Module("<anonymous type>") {
  class T_Result extends Type {
    def getType : C_TYPE[_] = t_Result;
  }
  object t_Result extends I_TYPE[T_Result]("<anonymous>") {}
}

object PARSE {
  var lineNumber : Int = 0;
}

abstract class Phylum extends Type
{
  def getType : C_PHYLUM[_];
  val lineNumber = PARSE.lineNumber;
  private var _nodeNumber : Int = 0;
  def nodeNumber = _nodeNumber;
  private var _parent : Phylum = null;
  def parent : Phylum = _parent;
  def children : List[Phylum];
  def register : this.type = {
    _nodeNumber = getType.asInstanceOf[I_PHYLUM[Phylum]].registerNode(this);
    for (ch <- children) {
      assert (ch.parent == null);
      ch._parent = this;
    }
    this
  }
}

trait C_PHYLUM[T_Result] extends C_TYPE[T_Result] {
  val v_identical : (T_Result,T_Result) => Boolean;
  val v_object_id : (T_Result) => Int;
  val v_object_id_less: (T_Result, T_Result) => Boolean;
  def v_nil : T_Result;
}

abstract class I_PHYLUM[T_Result >: Null](n : String)
	 extends I_TYPE[T_Result](n) with C_PHYLUM[T_Result] 
{
  private val allNodes : Buffer[T_Result] = new ListBuffer[T_Result];
  def registerNode(x : Phylum) : Int = {
    assert (!isComplete);
    val result = size;
    allNodes += x.asInstanceOf[T_Result];
    // println("registering " + x + " as #" + result);
    result
  };
  def size : Int = allNodes.size;
  def get(i : Int) : T_Result = allNodes(i);

  val v_identical = f_identical _;
  val v_object_id = f_object_id _;
  val v_object_id_less = f_object_id_less _;

  override def f_equal(x : T_Result, y : T_Result) : Boolean = f_identical(x,y);
  def f_identical(x : T_Result, y : T_Result) : Boolean =
    if (x == null || y == null) x == y
    else f_object_id(x) == f_object_id(y);
  def f_object_id(x : T_Result) : Int = 
    x match {
      case n:Phylum => n.nodeNumber;
    };
  def f_object_id_less(x1 : T_Result, x2 : T_Result) : Boolean =
    f_object_id(x1) < f_object_id(x2);
  override def f_string(x : T_Result) : String = 
    super.f_string(x) + "@" + f_object_id(x);

  val v_nil : T_Result = null;
}

class M_PHYLUM extends Module("<anonymous phylum>")
{
  class T_Result extends Phylum {
    def getType : I_PHYLUM[_] = t_Result;
    def children : List[Phylum] = List();
  }
  object t_Result extends I_PHYLUM[T_Result]("anonymous") { }
  override def finish() {
    super.finish();
    t_Result.finish();
  }
}

class Evaluation {
  var inCycle : Boolean = false;
}

object APS {
  // evaluation
  sealed trait EvalStatus;

  case object UNINITIALIZED extends EvalStatus;
  case object UNEVALUATED extends EvalStatus;
  case class CYCLE(d : Int) extends EvalStatus;
  case object EVALUATED extends EvalStatus;
  case object ASSIGNED extends EvalStatus;

  class APSException(s : String) extends RuntimeException(s) {}
  object TooLateError extends APSException("cannot assign to attribute once evaluation has started") {}
  object NullConstructorError extends APSException("null constructor") {}
  case class UndefinedAttributeException(w : String) extends APSException("undefined attribute: " + w) {}
  case class CyclicAttributeException(w : String) extends APSException("cyclic attribute: " + w) {}
  object StubError extends APSException("stub error") {}

  import scala.collection.mutable.Stack;

  val pending : Stack[Evaluation] = new Stack();
  val acyclic : Evaluation = new Evaluation;
  pending.push(acyclic); // stack should never be empty

}

class Attribute[T_P >: Null, T_V]
               (val t_P : C_PHYLUM[T_P],
		val t_V : Any, // unused
		val name : String) 
extends Module("Attribute " + name)
{
  type NodeType = T_P;
  type ValueType = T_V;
  import APS._;

  private val values : Buffer[ValueType] = new ArrayBuffer();
  private val status : Buffer[EvalStatus] = new ArrayBuffer();
  private var evaluationStarted : Boolean = false;

  def assign(n : NodeType, v : ValueType) : Unit = {
    Debug.begin(t_P.v_string(n) + "." + name + ":=" + v);
    if (evaluationStarted) throw TooLateError;
    set(n,v);
    Debug.end();
  }
    
  override def finish() : Unit = {
    if (!isComplete) {
      val t_NodeType = t_P.asInstanceOf[I_PHYLUM[NodeType]];
      val n = t_NodeType.size;
      for (i <- 0 until n) {
	evaluate(t_NodeType.get(i));
      }
    }
    super.finish();
  }

  def evaluation : Evaluation = acyclic;

  private def doEvaluate(n : NodeType) : ValueType = {
    val num = checkPhylum(n);
    status(num) match {
      case CYCLE(d) => {
	detectCycle(n,d);
      }
      case UNINITIALIZED | UNEVALUATED => {
	status(num) = CYCLE(pending.size);
	pending.push(evaluation);
        var v : ValueType = compute(n);
        values(num) = v;
	if (pending.pop().inCycle) {
	  evaluateCycle(n);
	  v = values(num);
	}
	status(num) = EVALUATED;
	v
      }
      case _ => get(n)
    }
  }

  def evaluate(n : Any) : ValueType = {
    Debug.begin(n + "." + name);
    try {
      evaluationStarted = true;
      val v = doEvaluate(n.asInstanceOf[NodeType]);
      Debug.returns(v);
      v
    } finally {
      Debug.end();
    }
  }

  def evaluateCycle(n : NodeType) : Unit = {
    throw new CyclicAttributeException("internal cyclic error: " + n+"."+name);
  }
  
  def detectCycle(n : NodeType, d : Int) : ValueType = {
    throw new CyclicAttributeException(n+"."+name);
  }
  
  def checkPhylum(n : NodeType) : Int = {
    t_P.v_assert(n);
    n match {
      case p:Phylum => {
	val num = p.nodeNumber;
	val pp = p.getType.asInstanceOf[I_PHYLUM[NodeType]].get(num);
	if (p != pp) {
	  println("Got " + p + "'s number as " + num + ", but the node with that number is " + pp);
	}
	assert (p == pp);
	while (num >= values.size) {
	  values += null.asInstanceOf[ValueType];
	  status += UNINITIALIZED;
	};
	num
      }
    }
  }
  
  def set(n : NodeType, v : ValueType) : Unit = {
    val num = checkPhylum(n);
    values(num) = v;
    status(num) = ASSIGNED;
  }

  def get(n : NodeType) : ValueType = {
    val i = checkPhylum(n);
    if (status(i) == UNINITIALIZED) {
      values(i) = getDefault(n);
      status(i) = UNEVALUATED;
    }
    return values(i);
  }
  
  def getStatus(n : NodeType) : EvalStatus = {
    val i = checkPhylum(n);
    return status(i);
  }
  
  def setStatus(n : NodeType, es : EvalStatus) : Unit = {
    val i = checkPhylum(n);
    status(i) = es;
  }
  
  def getDefault(n : NodeType) : ValueType = {
    throw new UndefinedAttributeException(n + "." + name);
  }
  
  def compute(n : NodeType) : ValueType = {
    return getDefault(n);
  }
}

trait Collection[V_P >: Null, V_T] extends Attribute[V_P,V_T] {
  def initial : ValueType;
  def combine(v1 : ValueType, v2 : ValueType) : ValueType;

  override def getDefault(n : NodeType) : ValueType = initial;

  override def set(n : NodeType, v : ValueType) : Unit = {
    super.set(n,combine(get(n),v));
  }
}

trait Circular[V_P >: Null, V_T] extends Attribute[V_P,V_T] {
  import APS._;

  def lattice : C_LATTICE[ValueType];

  override def getDefault(n : NodeType) : ValueType = lattice.v_bottom;

  override def detectCycle(n : NodeType, d : Int) : ValueType = {
    markPending(d);
    get(n)
  }

  def markPending(d : Int) : Unit = {
    var diff : Int = pending.size - d;
    for (e <- pending.reverse) {
      if (diff == 0 || e.inCycle) return;
      if (e == acyclic) {
	throw new CyclicAttributeException("indirectly cyclic: " + name);	
      }
      e.inCycle = true;
      diff -= 1;
    }
  }

  override def evaluateCycle(n : NodeType) : Unit = {
    if (pending.top == acyclic) {
      // it's our job to repeat evaluation until there is no change
      var last = super.get(n);
      pending.push(new Evaluation());
      while (true) {
	val next = compute(n);
	if (lattice.v_equal(last,next)) {
	  setStatus(n,EVALUATED);
	  pending.pop();
	  return;
	}
	if (!lattice.v_compare(last,next)) {
	  throw new CyclicAttributeException("non-monotonic " + n+"."+name);
	}
	last = next;
      }
    }
  }
}

class PatternFunction[A,R](f : Any => Option[A]) {
  def unapply(x : Any) : Option[A] = f(x);
}

class Pattern0Function[R](f : Any => Option[Unit]) {
  def unapply(x : Any) : Boolean = f(x) == Some(());
}

class PatternSeqFunction[A,R](f : Any => Option[Seq[A]]) {
  def unapplySeq(x : Any) : Option[Seq[A]] = f(x);
}

object P_AND {
  def unapply[T](x : T) : Option[(T,T)] = Some((x,x));
}


