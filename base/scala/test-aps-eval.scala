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
    test1_staleNonCircular();
    test2_nonCircularInCyclePath();
    test3_caseMatchIsMonotone();
    println("All tests passed.");
  }

  // A non-circular attribute that reads a circular attribute should
  // reflect the converged value, not a value seen mid-iteration.
  def test1_staleNonCircular(): Unit = {
    var nonCircularAttribute: Evaluation[String, Int] = null;
    var firstCompute = true;

    val circularAttribute = new Evaluation[String, Int]("node", "circularAttribute")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = {
        if (firstCompute) { firstCompute = false; nonCircularAttribute.get; }
        math.min(value + 1, 3);
      }
    };

    nonCircularAttribute = new Evaluation[String, Int]("node", "nonCircularAttribute") {
      override def compute: Int = circularAttribute.get + 100;
    };

    val circularResult = circularAttribute.get;
    assert(circularResult == 3, s"circularAttribute: expected 3, got $circularResult");

    val nonCircularResult = nonCircularAttribute.get;
    assert(nonCircularResult == 103, s"nonCircularAttribute: expected 103, got $nonCircularResult");
  }

  // A plain (non-circular) attribute that participates in a cycle path
  // should not throw CyclicAttributeException.
  def test2_nonCircularInCyclePath(): Unit = {
    var circularAttribute: CircularEvaluation[String, Int] = null;
    var firstCall = true;

    val middleAttribute = new Evaluation[String, Int]("node", "middleAttribute") {
      override def compute: Int = circularAttribute.get + 10;
    };

    circularAttribute = new Evaluation[String, Int]("node", "circularAttribute")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = {
        if (firstCall) { firstCall = false; middleAttribute.get; }
        else math.min(value + 1, 30);
      }
    };

    val result = circularAttribute.get;
    assert(result == 30, s"circularAttribute: expected 30, got $result");

    val middleResult = middleAttribute.get;
    assert(middleResult == 40, s"middleAttribute: expected 40, got $middleResult");
  }

  // A non-circular attribute whose result depends on a circular attribute
  // via a conditional should reflect the converged value.
  def test3_caseMatchIsMonotone(): Unit = {
    var lookupAttribute: Evaluation[String, Int] = null;
    var firstCompute = true;

    val envAttribute = new Evaluation[String, Int]("node", "envAttribute")
      with CircularEvaluation[String, Int] {
      val lattice = IntLatticeInstance;
      override def compute: Int = {
        if (firstCompute) { firstCompute = false; lookupAttribute.get; }
        math.min(value + 1, 3);
      }
    };

    lookupAttribute = new Evaluation[String, Int]("node", "lookupAttribute") {
      override def compute: Int = {
        val env = envAttribute.get;
        if (env > 0) env else 0;
      }
    };

    val envResult = envAttribute.get;
    assert(envResult == 3, s"envAttribute: expected 3, got $envResult");

    val lookupResult = lookupAttribute.get;
    assert(lookupResult == 3, s"lookupAttribute: expected 3, got $lookupResult");
  }
}
