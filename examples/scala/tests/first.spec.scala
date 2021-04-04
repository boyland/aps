object FirstSpec extends Spec {

  import GrammarUtil._

  def testBasic() = {
    // Arrange
    var map = List[(String, List[String])]()
    map +:= ("Z", List("Y", "X"))
    map +:= ("Y", List("y"))
    map +:= ("X", List("x"))
    map +:= ("X", List("Z", "z"))
  
    // Act
    val first = new M_FIRST("First", toGrammar(map))
    first.finish()

    val firstTable = (for ((key, value) <- first.v_firstTable) yield (key.name, value.map(x => x.name).toSet)).toMap

    // Assert
    assertEquals(3, firstTable.size, "Validate firstTable size")
    assertEquals(Set("x", "y"), firstTable("X"), "Validate first of X");
    assertEquals(Set("y"), firstTable("Y"), "Validate first of Y");
    assertEquals(Set("y"), firstTable("Z"), "Validate first of Z");
  }
}