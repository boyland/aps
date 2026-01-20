import basic_implicit._;

object TestFlat extends App {

  private def assert_equals[T](text: String, expected: T, actual: T) = {
    if (expected != actual) {
      throw new RuntimeException(s"Failed: $text")
    }
  }

  val t_FlatIntegerLattice = new M_FLAT_LATTICE[T_Integer]("Flat", t_Integer);

  val i1 = t_FlatIntegerLattice.v_lift(1);
  val i2 = t_FlatIntegerLattice.v_lift(2);
  val i1a = t_FlatIntegerLattice.v_lift(1);
  val top = t_FlatIntegerLattice.v_top;
  val bot = t_FlatIntegerLattice.v_bottom;

  assert_equals("bot", "BOT", t_FlatIntegerLattice.v_string(bot));
  assert_equals("i1", "LIFT(1)", t_FlatIntegerLattice.v_string(i1));
  assert_equals("i2", "LIFT(2)", t_FlatIntegerLattice.v_string(i2));
  assert_equals("top", "TOP", t_FlatIntegerLattice.v_string(top));

  assert_equals("bot < bot", false, t_FlatIntegerLattice.v_compare(bot, bot));
  assert_equals("bot < i1", true, t_FlatIntegerLattice.v_compare(bot, i1));
  assert_equals("bot < i2", true, t_FlatIntegerLattice.v_compare(bot, i2));
  assert_equals("bot < top", true, t_FlatIntegerLattice.v_compare(bot, top));

  assert_equals("i1 < bot", false, t_FlatIntegerLattice.v_compare(i1, bot));
  assert_equals("i1 < i1", false, t_FlatIntegerLattice.v_compare(i1, i1));
  assert_equals("i1 < i2", false, t_FlatIntegerLattice.v_compare(i1, i2));
  assert_equals("i1 < top", true, t_FlatIntegerLattice.v_compare(i1, top));

  assert_equals("top < bot", false, t_FlatIntegerLattice.v_compare(top, bot));
  assert_equals("top < i1", false, t_FlatIntegerLattice.v_compare(top, i1));
  assert_equals("top < i2", false, t_FlatIntegerLattice.v_compare(top, i2));
  assert_equals("top < top", false, t_FlatIntegerLattice.v_compare(top, top));

  assert_equals("bot <= bot", true, t_FlatIntegerLattice.v_compare_equal(bot, bot));
  assert_equals("bot <= i1", true, t_FlatIntegerLattice.v_compare_equal(bot, i1));
  assert_equals("bot <= i2", true, t_FlatIntegerLattice.v_compare_equal(bot, i2));
  assert_equals("bot <= top", true, t_FlatIntegerLattice.v_compare_equal(bot, top));

  assert_equals("i1 <= bot", false, t_FlatIntegerLattice.v_compare_equal(i1, bot));
  assert_equals("i1 <= i1", true, t_FlatIntegerLattice.v_compare_equal(i1, i1));
  assert_equals("i1 <= i2", false, t_FlatIntegerLattice.v_compare_equal(i1, i2));
  assert_equals("i1 <= top", true, t_FlatIntegerLattice.v_compare_equal(i1, top));

  assert_equals("top <= bot", false, t_FlatIntegerLattice.v_compare_equal(top, bot));
  assert_equals("top <= i1", false, t_FlatIntegerLattice.v_compare_equal(top, i1));
  assert_equals("top <= i2", false, t_FlatIntegerLattice.v_compare_equal(top, i2));
  assert_equals("top <= top", true, t_FlatIntegerLattice.v_compare_equal(top, top));

  assert_equals("bot \\/ bot", "BOT", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(bot, bot)));
  assert_equals("bot \\/ i1", "LIFT(1)", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(bot, i1)));
  assert_equals("bot \\/ i2", "LIFT(2)", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(bot, i2)));
  assert_equals("bot \\/ top", "TOP", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(bot, top)));

  assert_equals("i1 \\/ bot", "LIFT(1)", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(i1, bot)));
  assert_equals("i1 \\/ i1", "LIFT(1)", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(i1, i1)));
  assert_equals("i1 \\/ i2", "TOP", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(i1, i2)));
  assert_equals("i1 \\/ top", "TOP", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(i1, top)));

  assert_equals("top \\/ bot", "TOP", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(top, bot)));
  assert_equals("top \\/ i1", "TOP", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(top, i1)));
  assert_equals("top \\/ i2", "TOP", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(top, i2)));
  assert_equals("top \\/ top", "TOP", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_join(top, top)));

  assert_equals("bot /\\ bot", "BOT", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(bot, bot)));
  assert_equals("bot /\\ i1", "BOT", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(bot, i1)));
  assert_equals("bot /\\ i2", "BOT", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(bot, i2)));
  assert_equals("bot /\\ top", "BOT", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(bot, top)));

  assert_equals("i1 /\\ bot", "BOT", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(i1, bot)));
  assert_equals("i1 /\\ i1", "LIFT(1)", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(i1, i1)));
  assert_equals("i1 /\\ i2", "BOT", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(i1, i2)));
  assert_equals("i1 /\\ top", "LIFT(1)", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(i1, top)));

  assert_equals("top /\\ bot", "BOT", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(top, bot)));
  assert_equals("top /\\ i1", "LIFT(1)", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(top, i1)));
  assert_equals("top /\\ i2", "LIFT(2)", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(top, i2)));
  assert_equals("top /\\ top", "TOP", t_FlatIntegerLattice.v_string(t_FlatIntegerLattice.v_meet(top, top)));
}