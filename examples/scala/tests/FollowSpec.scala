object FollowSpec extends Spec {

  import GrammarUtil._

  def testBasic() = {
    // Arrange
    var map = List[(String, List[String])]()
    map +:= ("Z", List("Y", "X"))
    map +:= ("Y", List("y"))
    map +:= ("X", List("x"))
    map +:= ("X", List("Z", "z"))
  
    // Act
    val first = new M_FOLLOW("Follow", buildGrammar(map))
    first.finish()

    val followTable = (for ((key, value) <- first.v_followTable) yield (key.name, value.map(x => x.name).toSet)).toMap

    // Assert
    assertEquals(3, followTable.size, "Validate followTable size")
    assertEquals(Set("z"), followTable("X"), "Validate follow of X");
    assertEquals(Set("x", "y"), followTable("Y"), "Validate follow of Y");
    assertEquals(Set("z"), followTable("Z"), "Validate follow of Z");
  }
}