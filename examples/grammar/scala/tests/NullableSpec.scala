object NullableSpec extends Spec {

  import GrammarUtil._

  def testBasic() = {
    // Arrange
    var map = List[(String, List[String])]()
    map +:= ("Z", List("Y", "X"))
    map +:= ("Y", List())
    map +:= ("X", List())
    map +:= ("A", List("Z", "a"))
    map +:= ("B", List("b", "B"))
    map +:= ("B", List())
  
    // Act
    val first = new M_NULLABLE("Nullable", buildGrammar(map))
    first.finish()

    val nullableTable = (for ((key, value) <- first.v_nullableTable) yield (key.name, value)).toMap

    // Assert
    assertEquals(5, nullableTable.size, "Validate nullableTable size")
    assertEquals(true, nullableTable("X"), "Validate nullable of X");
    assertEquals(true, nullableTable("Y"), "Validate nullable of Y");
    assertEquals(true, nullableTable("Z"), "Validate nullable of Z");
    assertEquals(false, nullableTable("A"), "Validate nullable of A");
    assertEquals(true, nullableTable("B"), "Validate nullable of B");
  }
}
