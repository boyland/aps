#include <iostream>
#include "aps-impl.h"
#include "basic.h"

using namespace std;

typedef C_SET<C_INTEGER> C_IntegerSet;
typedef C_IntegerSet::T_Result T_IntegerSet;

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
  T_IntegerSet is, is2, is3;
  C_IntegerSet* t_IntegerSet = new C_IntegerSet(get_Integer());
  COLL<C_IntegerSet,C_Integer> iscoll(t_IntegerSet,get_Integer());
  C_IntegerSet& t = *t_IntegerSet;
  is = t.v_append(t.v_append(t.v_single(1),t.v_single(2)),t.v_single(3));
  is2 = t.v_append(t.v_append(t.v_single(3),t.v_single(0)),t.v_single(2));
  is3 = t.v_append(t.v_append(t.v_single(3),t.v_single(1)),t.v_single(2));

  assert_equals("s1",string("{1,2,3}"),iscoll.to_string(is));
  assert_equals("s2",string("{3,0,2}"),iscoll.to_string(is2));
  assert_equals("s3",string("{3,1,2}"),iscoll.to_string(is3));

  assert_equals("union(s1,s2)",string("{1,3,0,2}"),
		iscoll.to_string(t.v_union(is,is2)));
  assert_equals("intersect(s1,s2)",string("{2,3}"),
		iscoll.to_string(t.v_intersect(is,is2)));
  assert_equals("difference(s1,s2)",string("{1}"),
		iscoll.to_string(t.v_difference(is,is2)));

  assert_equals("equal(s1,s2)",false,t.v_equal(is,is2));
  assert_equals("equal(s1,s3)",true,t.v_equal(is,is3));
  assert_equals("equal(s2,s3)",false,t.v_equal(is2,is3));
  
  return failed;
}
