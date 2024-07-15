#include <iostream>
#include "aps-impl.h"
#include "basic.h"
#include "flat.h"

using namespace std;

typedef C_FLAT_LATTICE<C_INTEGER> C_LiftedInteger;
typedef C_LiftedInteger::T_Result T_LiftedInteger;

static bool verbose = false;
static int failed = 0;

template <class T>
static void assert_equals(const string &test, T expected, T actual)
{
  bool passed = actual == expected;
  if (verbose || !passed) cout << test << " = " << actual << endl;
  if (passed) return;
  ++failed;
  cout << "  failed!  (Expected " << expected << ")" << endl;
}

int main(int argc, char **argv)
{
  if (argc > 1) verbose = true;
  T_LiftedInteger i1, i2, i1a, top, bot;
  C_LiftedInteger* t_LiftedInteger = new C_LiftedInteger(get_Integer());
  C_LiftedInteger& t = *t_LiftedInteger;
  i1 = t.v_lift(1);
  i2 = t.v_lift(2);
  i1a = t.v_lift(1);
  top = t.v_top;
  bot = t.v_bottom;

  assert_equals("bot",string("BOT"),t.v_string(bot));
  assert_equals("i1",string("LIFT(1)"),t.v_string(i1));
  assert_equals("i2",string("LIFT(2)"),t.v_string(i2));
  assert_equals("top",string("TOP"),t.v_string(top));
  
  assert_equals("bot < bot",false,t.v_compare(bot,bot));
  assert_equals("bot < i1",true,t.v_compare(bot,i1));
  assert_equals("bot < i2",true,t.v_compare(bot,i2));
  assert_equals("bot < top",true,t.v_compare(bot,top));

  assert_equals("i1 < bot",false,t.v_compare(i1,bot));
  assert_equals("i1 < i1",false,t.v_compare(i1,i1));
  assert_equals("i1 < i2",false,t.v_compare(i1,i2));
  assert_equals("i1 < top",true,t.v_compare(i1,top));

  assert_equals("top < bot",false,t.v_compare(top,bot));
  assert_equals("top < i1",false,t.v_compare(top,i1));
  assert_equals("top < i2",false,t.v_compare(top,i2));
  assert_equals("top < top",false,t.v_compare(top,top));

  assert_equals("bot <= bot",true,t.v_compare_equal(bot,bot));
  assert_equals("bot <= i1",true,t.v_compare_equal(bot,i1));
  assert_equals("bot <= i2",true,t.v_compare_equal(bot,i2));
  assert_equals("bot <= top",true,t.v_compare_equal(bot,top));

  assert_equals("i1 <= bot",false,t.v_compare_equal(i1,bot));
  assert_equals("i1 <= i1",true,t.v_compare_equal(i1,i1));
  assert_equals("i1 <= i2",false,t.v_compare_equal(i1,i2));
  assert_equals("i1 <= top",true,t.v_compare_equal(i1,top));

  assert_equals("top <= bot",false,t.v_compare_equal(top,bot));
  assert_equals("top <= i1",false,t.v_compare_equal(top,i1));
  assert_equals("top <= i2",false,t.v_compare_equal(top,i2));
  assert_equals("top <= top",true,t.v_compare_equal(top,top));

  assert_equals("bot \\/ bot",string("BOT"),t.v_string(t.v_join(bot,bot)));
  assert_equals("bot \\/ i1",string("LIFT(1)"),t.v_string(t.v_join(bot,i1)));
  assert_equals("bot \\/ i2",string("LIFT(2)"),t.v_string(t.v_join(bot,i2)));
  assert_equals("bot \\/ top",string("TOP"),t.v_string(t.v_join(bot,top)));

  assert_equals("i1 \\/ bot",string("LIFT(1)"),t.v_string(t.v_join(i1,bot)));
  assert_equals("i1 \\/ i1",string("LIFT(1)"),t.v_string(t.v_join(i1,i1)));
  assert_equals("i1 \\/ i2",string("TOP"),t.v_string(t.v_join(i1,i2)));
  assert_equals("i1 \\/ top",string("TOP"),t.v_string(t.v_join(i1,top)));

  assert_equals("top \\/ bot",string("TOP"),t.v_string(t.v_join(top,bot)));
  assert_equals("top \\/ i1",string("TOP"),t.v_string(t.v_join(top,i1)));
  assert_equals("top \\/ i2",string("TOP"),t.v_string(t.v_join(top,i2)));
  assert_equals("top \\/ top",string("TOP"),t.v_string(t.v_join(top,top)));

  assert_equals("bot /\\ bot",string("BOT"),t.v_string(t.v_meet(bot,bot)));
  assert_equals("bot /\\ i1",string("BOT"),t.v_string(t.v_meet(bot,i1)));
  assert_equals("bot /\\ i2",string("BOT"),t.v_string(t.v_meet(bot,i2)));
  assert_equals("bot /\\ top",string("BOT"),t.v_string(t.v_meet(bot,top)));

  assert_equals("i1 /\\ bot",string("BOT"),t.v_string(t.v_meet(i1,bot)));
  assert_equals("i1 /\\ i1",string("LIFT(1)"),t.v_string(t.v_meet(i1,i1)));
  assert_equals("i1 /\\ i2",string("BOT"),t.v_string(t.v_meet(i1,i2)));
  assert_equals("i1 /\\ top",string("LIFT(1)"),t.v_string(t.v_meet(i1,top)));

  assert_equals("top /\\ bot",string("BOT"),t.v_string(t.v_meet(top,bot)));
  assert_equals("top /\\ i1",string("LIFT(1)"),t.v_string(t.v_meet(top,i1)));
  assert_equals("top /\\ i2",string("LIFT(2)"),t.v_string(t.v_meet(top,i2)));
  assert_equals("top /\\ top",string("TOP"),t.v_string(t.v_meet(top,top)));

  return failed;
}
