class Spec extends App {

  def assertTrue(result: Boolean, message: String) = {
    assertEquals(true, result, message);
  }

  def assertEquals[T](expected: T, actual: T, message: String) = {
    if (expected != actual) {
      throw new Error(f"${message} ~> expected: ${expected} but got: ${actual}")
    }
  }

  {
    println(f"Running test file: ${getClass.getSimpleName}")

    getClass
      .getMethods
      .filter(_.getName.startsWith("test"))
      .foreach(m => {
        println(f" > Running ${m.getName}")
        m.invoke(this)
        println(f" > Finished ${m.getName}")
      })
  }
}