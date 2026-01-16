// APS runtime system
// September, 2010
// John Boyland

import scala.collection.mutable.Buffer;
import scala.collection.mutable.ArrayBuffer;

object Debug {
  private var depth : Int = 0;
  private var _active : Boolean = false;
  private var print_level : Int = 3;

  def active = _active;
  
  def activate() : Unit = _active = true;

  def indent(): Unit = {
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
      println("Type " + mname + " is now finished.");
    }
    complete = true; 
  }
  def isComplete : Boolean = complete;
};
 
/**
 * The class of all constructed APS values.
 */
class Value(val myType : C_TYPE[_ <: Value]) { }

trait C_TYPE[T_Result] extends C_BASIC[T_Result] with C_PRINTABLE[T_Result]
{
  val v_assert : (T_Result) => Unit;
  val v_equal : (T_Result,T_Result) => Boolean;
  val v_node_equivalent : (T_Result,T_Result) => Boolean;
  val v_string : (T_Result) => String;
}

class I_TYPE[T](name : String) extends Module(name) with C_TYPE[T] {
  type T_Result = T;
  val v_assert = f_assert _;
  val v_equal = f_equal _;
  val v_node_equivalent = f_node_equivalent _;
  val v_string = f_string _;

  def f_assert(x : T_Result) : Unit =
    x match {
      case t:Value => assert(t.myType == this)
    };
  def f_equal(x : T_Result, y : T_Result) : Boolean = f_node_equivalent(x,y);
  def f_node_equivalent(x : T_Result, y : T_Result) : Boolean = x != null && y != null && x == y;
  def f_string(x : T_Result) : String = x.toString();
}
    
class T_TYPE(t_Result : C_TYPE[T_TYPE]) extends Value(t_Result) { };

class M_TYPE(name : String) extends I_TYPE[T_TYPE](name) {
}

object PARSE {
  var lineNumber : Int = 0;
}

abstract class Node(t : C_PHYLUM[_ <: Node]) extends Value(t)
{
  override val myType = t;
  val lineNumber = PARSE.lineNumber;
  private var _nodeNumber : Int = 0;
  def nodeNumber = _nodeNumber;
  private var _parent : Node = null;
  def parent : Node = _parent;
  def children : List[Node];
  def register : this.type = {
    _nodeNumber = myType.nodes.add(this);
    for (ch <- children) {
      assert (ch.parent == null);
      ch._parent = this;
    }
    this
  }
  def isRooted : Boolean = (parent != null) && parent.isRooted;
}

class Nodes[T_Result <: Node] extends ArrayBuffer[T_Result] {
  private var _frozen = false;
  def freeze() : Unit = _frozen = true;
  def add(x : Node) : Int = {
    assert (!_frozen);
    val result = size;
    this += x.asInstanceOf[T_Result];
    // println("registering " + x + " as #" + result);
    result
  };
}

trait C_PHYLUM[T_Result <: Node] extends C_TYPE[T_Result] {
  val v_identical : (T_Result,T_Result) => Boolean;
  val v_object_id : (T_Result) => Int;
  val v_object_id_less: (T_Result, T_Result) => Boolean;
  def v_nil : T_Result;
  val nodes : Nodes[T_Result];
}

class I_PHYLUM[T_Result <: Node](name : String)
	 extends I_TYPE[T_Result](name) with C_PHYLUM[T_Result] 
{
  val v_identical = f_identical _;
  val v_object_id = f_object_id _;
  val v_object_id_less = f_object_id_less _;

  override def f_equal(x : T_Result, y : T_Result) : Boolean = f_identical(x,y);
  def f_identical(x : T_Result, y : T_Result) : Boolean = x eq y;
  def f_object_id(x : T_Result) : Int = 
    x match {
      case n:Node => n.nodeNumber;
    };
  def f_object_id_less(x1 : T_Result, x2 : T_Result) : Boolean =
    f_object_id(x1) < f_object_id(x2);
  override def f_string(x : T_Result) : String = 
    super.f_string(x) + "@" + f_object_id(x);

  val v_nil : T_Result = null.asInstanceOf[T_Result];
  val nodes : Nodes[T_Result] = new Nodes();

  override def finish() = {
    nodes.freeze();
    super.finish();
  }
}

abstract class T_PHYLUM(t_Result : C_PHYLUM[T_PHYLUM]) 
extends Node(t_Result) { }

class M_PHYLUM(name : String) extends I_PHYLUM[T_PHYLUM](name)
{ }

object Evaluation {
  sealed trait EvalStatus;

  case object UNINITIALIZED extends EvalStatus;
  case object UNEVALUATED extends EvalStatus;
  case object CYCLE extends EvalStatus;
  case object EVALUATED extends EvalStatus;
  case object ASSIGNED extends EvalStatus;

  class APSException(s : String) extends RuntimeException(s) {}
  object TooLateError extends APSException("cannot assign to attribute once evaluation has started") {}
  object NullConstructorError extends APSException("null constructor") {}
  case class UndefinedAttributeException(w : String) extends APSException("undefined attribute: " + w) {}
  case class CyclicAttributeException(w : String) extends APSException("cyclic attribute: " + w) {}
  object StubError extends APSException("stub error") {}

  class Stack[A](private var elems: List[A] = List.empty[A]) extends Iterable[A] {
    def push(v: A): Stack[A] = {
      elems = v :: elems;
      this;
    }

    def pop(): A = {
      if (elems.isEmpty) {
        throw new NoSuchElementException("Empty Stack");
      } else {
        val popped = elems.head;
        elems = elems.tail;
        popped;
      }
    }

    override def iterator: Iterator[A] = elems.iterator
  }

  val pending : Stack[Evaluation[_,_]] = new Stack();
}

class Evaluation[T_P, T_V](val anchor : T_P, val name : String)
{
  import Evaluation._;

  type NodeType = T_P;
  type ValueType = T_V;
  import Evaluation._;

  var status : EvalStatus = UNINITIALIZED;
  var value : ValueType = null.asInstanceOf[ValueType];
  // Flag that can be overridden to prevent testing for TooLateError
  var checkForLateUpdate = true;

  def inCycle : CircularEvaluation[_,_] = null;
  def setInCycle(ce : CircularEvaluation[_,_]) : Unit = {
    throw new CyclicAttributeException(name);
  }

  private def doEvaluate : ValueType = {
    status match {
      case CYCLE => detectedCycle
      case UNINITIALIZED | UNEVALUATED => {
	status = CYCLE;
	pending.push(this);
        value = compute;
	pending.pop();
	if (inCycle != null) {
	  evaluateCycle;
	} else {
	  status = EVALUATED;
	}
	value
      }
      case _ => value
    }
  }

  def get : ValueType = {
    Debug.begin(name);
    try {
      val v = doEvaluate;
      Debug.returns(v);
      v
    } finally {
      Debug.end();
    }
  }

  def assign(v : ValueType) : Unit = {
    Debug.out(name + " := " + v);
    set(v);
  }

  def set(v : ValueType) : Unit = {
    status match {
    case EVALUATED => if (checkForLateUpdate) throw TooLateError else ();
    case _ => ();
    }
    value = v;
    status = ASSIGNED;
  }

  def evaluateCycle : Unit = {
    throw new CyclicAttributeException("internal cyclic error: " + anchor+"."+name);
  }
  
  def detectedCycle : ValueType = {
    throw new CyclicAttributeException(s"$anchor.$name");
  }

  def compute : ValueType = getDefault;

  def getDefault : ValueType = {
    throw new UndefinedAttributeException(s"$anchor.$name");
  };
}

class Attribute[T_P <: Node, T_V]
               (val t_P : C_PHYLUM[T_P],
		val t_V : Any, // unused
		val name : String) 
extends Module("Attribute " + name)
{
  type NodeType = T_P;
  type ValueType = T_V;
  import Evaluation._;

  private val evaluations : Buffer[Evaluation[T_P,T_V]] = new ArrayBuffer();
  private var evaluationStarted : Boolean = false;

  def assign(n : NodeType, v : ValueType) : Unit = {
    Debug.begin(t_P.v_string(n) + "." + name + ":=" + v);
    if (evaluationStarted) throw TooLateError;
    set(n,v);
    Debug.end();
  }
    
  override def finish() : Unit = {
    if (!isComplete) {
      // We only demand values of rooted trees
      // Otherwise "garbage" trees will cause problems 
      for (n <- t_P.nodes) {
	if (n.isRooted) get(n);
      }
    }
    super.finish();
  }

  def checkNode(n : NodeType) : Evaluation[T_P,T_V] = {
    t_P.v_assert(n);
    val num = n.nodeNumber;
    {
      val nn = n.myType.nodes(num);
      if (n != nn) {
	println("Got " + n + "'s number as " + num + ", but the node with that number is " + nn);
      }
      assert (n == nn);
    }
    while (num >= evaluations.size) {
      val nn = n.myType.nodes(evaluations.size);
      evaluations += createEvaluation(nn.asInstanceOf[NodeType])
    };
    evaluations(num)
  }
  
  def set(n : NodeType, v : ValueType) : Unit = {
    checkNode(n).set(v);
  }

  def get(n : Any) : ValueType = {
    checkNode(n.asInstanceOf[NodeType]).get;
  }
  
  def createEvaluation(anchor : NodeType) : Evaluation[NodeType,ValueType] = {
    return new Evaluation(anchor, s"$anchor.$name");
  }
}

trait CollectionEvaluation[V_P, V_T] extends Evaluation[V_P,V_T] {
  import Evaluation._;

  def initial : ValueType = super.getDefault;
  def combine(v1 : ValueType, v2 : ValueType) : ValueType;

  override
  def getDefault : ValueType = initial;

  private def initialize : Unit = {
    if (status == UNINITIALIZED) {
      status = UNEVALUATED;
      value = initial;
    }
  };

  override def get : ValueType = {
    initialize;
    super.get
  }

  override def assign(v : ValueType) : Unit = {
    Debug.out(name + " :> " + v);
    set(v);
  }

  override def set(v : ValueType) : Unit = {
    initialize;
    super.set(combine(value,v));
  }
}

class CircularHelper(var cycleLast : CircularEvaluation[_,_]) {
  var modified : Boolean = false;
  
  {
    Debug.out("Created cycle helper " + this + " for " + cycleLast.name);
  }

  def add(c : CircularEvaluation[_,_]) : Unit = {
    Debug.out("Adding " + c.name + " to cycle " + this);
    assert (cycleLast != null);
    assert (c.cycleNext == null);
    cycleLast.cycleNext = c;
    cycleLast = c;
  }
  
  def addAll(c : CircularEvaluation[_,_]) : Unit = {
    {
      Debug.out("Adding all from " + c.helper + " to cycle " + this);
      var p = c;
      while (p != null) {
	Debug.out("  " + p.name);
	p = p.cycleNext;
      }
    }
    val other = c.helper;
    if (other == this) return;
    assert (other != null);
    assert (cycleLast != null);
    assert (cycleLast.cycleNext == null);
    assert (other.cycleLast != null);
    if (other.modified) modified = true;
    cycleLast.cycleNext = c;
    cycleLast = other.cycleLast;
    other.cycleLast = null;
  }
}

trait CircularEvaluation[V_P, V_T] extends Evaluation[V_P,V_T] {
  import Evaluation._;

  def lattice : C_LATTICE[ValueType];
  
  // a union-find link
  var cycleParent :  CircularEvaluation[_,_] = null;

  // keep a list of elements in cycle
  var cycleNext : CircularEvaluation[_,_] = null;

  // keep track of the whole list nodes in the cycle, and a modified flag
  var helper : CircularHelper = null;

  override def inCycle : CircularEvaluation[_,_] = {
    var p = cycleParent;
    if (p == null) return null;
    if (p != this) p = p.inCycle;
    if (p != cycleParent) cycleParent = p;
    p
  }

  override def setInCycle(ce : CircularEvaluation[_,_]) : Unit = {
    if (cycleParent == null) {
      cycleParent = ce;
      ce.helper.add(this);
    } else {
      val p = inCycle;
      ce.helper.addAll(p);
      p.cycleParent = ce;
    }
  }

  override def getDefault : ValueType = lattice.v_bottom;

  override def detectedCycle : ValueType = {
    Debug.out("Detected cycle for " + name);
    if (cycleParent == null) {
      value = getDefault;
      helper = new CircularHelper(this);
      cycleParent = this;
    }
    markPending();
    value
  }

  def markPending() : Unit = {
    val cycle = inCycle;
    for (e <- pending) {
      Debug.out("Checking " + e + " in pending.");
      if (e.inCycle == cycle) return;
      e.setInCycle(cycle);
    }
  }

  override def evaluateCycle : Unit = {
    if (cycleParent == this) {
      // we're in charge
      do {
	helper.modified = false;
	var c : CircularEvaluation[_,_] = this;
	do {
	  pending.push(c);
	  c.recompute();
	  pending.pop();
	  if (cycleParent != this) return; // no longer our responsibility
	  c = c.cycleNext;
	} while (c != null);
      } while (helper.modified);
      // now freeze all values:
      var c : CircularEvaluation[_,_] = this;
      do {
	c.status = EVALUATED;
	c = c.cycleNext;
      } while (c != null);
    }
  }

  def check(newValue : ValueType) : Unit = {
    if (!lattice.v_equal(value,newValue)) {
      inCycle.helper.modified = true;
      if (!lattice.v_compare(value,newValue)) {
	throw new CyclicAttributeException("non-monotonic " + name);
      }
    }
  };
  
  override def set(newValue : ValueType) : Unit = {
    check(newValue);
    super.set(newValue);
  }
  
  def recompute() : Unit = {
    val newValue = compute;
    check(newValue);
    value = newValue;
  }
}

// RA = (R,A1,A2,...)
class PatternFunction[RA](f : Any => Option[RA]) {
  def unapply(x : Any) : Option[RA] = f(x);
}

class PatternSeqFunction[R,A](f : Any => Option[(R,Seq[A])]) {
  def unapplySeq(x : Any) : Option[(R,Seq[A])] = f(x);
}

object P_AND {
  def unapply[T](x : T) : Option[(T,T)] = Some((x,x));
}
