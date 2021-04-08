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
    for (method <- getClass.getMethods if method.getName.startsWith("test")) {
      println(f"> Running ${method.getName}")
      method.invoke(this)
      println(f"> Finished ${method.getName}")
    }
  }
}
