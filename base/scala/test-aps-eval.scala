// Tests for circular and non-circular attribute evaluation behavior.

trait IntLattice extends C_LATTICE[Int] {
  type T_Result = Int;
  val v_bottom : Int = 0;
  val v_compare = (a: Int, b: Int) => a <= b;
  val v_compare_equal = (a: Int, b: Int) => a <= b;
  val v_join = (a: Int, b: Int) => math.max(a, b);
  val v_meet = (a: Int, b: Int) => math.min(a, b);
  val v_equal = (a: Int, b: Int) => a == b;
  val v_assert = (_: Int) => ();
  val v_node_equivalent = (a: Int, b: Int) => a == b;
  val v_string = (x: Int) => x.toString;
}
object IntLatticeInstance extends IntLattice;

object TestCircularStale {
  import Evaluation._;

  def main(args: Array[String]): Unit = {
    test1_singleCircularConvergence();
    test2_mutualCircularCycle();
    test3_nonCircularReadsConvergedCircular();
    test4_shortCircuitPreventsFullCycle();
    println("All tests passed.");
  }

  // basic: one attribute demands itself, should iterate to fixpoint
  def test1_singleCircularConvergence(): Unit = {
    val attr = new Evaluation[String, Int]("node", "attr")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = {
        val current = this.get;
        math.min(current + 1, 5);
      }
    };

    val result = attr.get;
    assert(result == 5, s"test1: expected 5, got $result");
  }

  // two attrs in a mutual cycle, both should converge
  def test2_mutualCircularCycle(): Unit = {
    var attrB: Evaluation[String, Int] with CircularEvaluation[String, Int] = null;

    val attrA = new Evaluation[String, Int]("nodeA", "attrA")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = math.min(attrB.get + 1, 4);
    };

    attrB = new Evaluation[String, Int]("nodeB", "attrB")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = math.min(attrA.get + 1, 4);
    };

    val resultA = attrA.get;
    val resultB = attrB.get;
    assert(resultA == 4, s"test2: attrA expected 4, got $resultA");
    assert(resultB == 4, s"test2: attrB expected 4, got $resultB");
  }

  // non-circular attr should see the converged value, not stale mid-iteration state
  def test3_nonCircularReadsConvergedCircular(): Unit = {
    val circAttr = new Evaluation[String, Int]("node", "circAttr")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = {
        val current = this.get;
        math.min(current + 1, 7);
      }
    };

    val circResult = circAttr.get;
    assert(circResult == 7, s"test3: circAttr expected 7, got $circResult");

    val plainAttr = new Evaluation[String, Int]("node", "plainAttr") {
      override def compute: Int = circAttr.get + 100;
    };

    val plainResult = plainAttr.get;
    assert(plainResult == 107, s"test3: plainAttr expected 107, got $plainResult");
  }

  // This is the farrow-ubd (and nested-ubd) bug pattern: if we skip demanding attrB
  // on the first pass, attrB never joins the cycle. Here we always demand it, so both converge.
  def test4_shortCircuitPreventsFullCycle(): Unit = {
    var attrB: Evaluation[String, Int] with CircularEvaluation[String, Int] = null;

    val attrA = new Evaluation[String, Int]("nodeA", "attrA")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = {
        val bVal = attrB.get; // must demand eagerly
        math.min(value + 1, 3);
      }
    };

    attrB = new Evaluation[String, Int]("nodeB", "attrB")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = attrA.get;
    };

    val resultA = attrA.get;
    val resultB = attrB.get;
    assert(resultA == 3, s"test4: attrA expected 3, got $resultA");
    assert(resultB == 3, s"test4: attrB expected 3, got $resultB");
  }
}
