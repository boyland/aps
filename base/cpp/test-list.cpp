#include <iostream>
#include "aps-impl.h"
#include "basic.h"

using namespace std;

typedef C_LIST<C_INTEGER> C_IntegerList;
typedef C_IntegerList::T_Result T_IntegerList;

bool eq_int(int x, int y) { return x == y; }

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

int main(int argc, char**argv)
{
  if (argc > 1) verbose = true;
  T_IntegerList is, is2;
  C_IntegerList* t_IntegerList = new C_IntegerList(get_Integer());
  COLL<C_IntegerList,C_Integer> iscoll(t_IntegerList,get_Integer());
  C_IntegerList& t = *t_IntegerList;
  is = t.v_append(t.v_append(t.v_single(1),t.v_single(2)),t.v_single(3));
  // is2 = t.v_append(t.v_append(t.v_single(3),t.v_single(0)),t.v_single(2));

  assert_equals("to_string(l)",string("{1,2,3}"),iscoll.to_string(is));

  assert_equals("position(1,l)",0,t.v_position(1,is));
  assert_equals("position(2,l)",1,t.v_position(2,is));
  assert_equals("position(3,l)",2,t.v_position(3,is));
  assert_equals("position(0,l)",-1,t.v_position(0,is));

  assert_equals("member(0,l)",false,t.v_member(0,is));
  assert_equals("member(1,l)",true,t.v_member(1,is));
  assert_equals("member(2,l)",true,t.v_member(2,is));
  assert_equals("member(3,l)",true,t.v_member(3,is));

  assert_equals("nth_from_end(0,l)",3,t.v_nth_from_end(0,is));
  assert_equals("nth_from_end(1,l)",2,t.v_nth_from_end(1,is));
  assert_equals("nth_from_end(2,l)",1,t.v_nth_from_end(2,is));
  
  return failed;
}
