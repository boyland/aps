#include <iostream>
#include "aps-impl.h"
#include "basic.h"

using namespace std;

typedef C_SET<C_INTEGER> C_IntegerSet;
typedef C_IntegerSet::T_Result T_IntegerSet;

bool eq_int(int x, int y) { return x == y; }

main()
{
  T_IntegerSet is, is2;
  C_IntegerSet* t_IntegerSet = new C_IntegerSet(get_Integer(),eq_int);
  COLL<C_IntegerSet,C_Integer> iscoll(t_IntegerSet,get_Integer());
  C_IntegerSet& t = *t_IntegerSet;
  is = t.v_append(t.v_append(t.v_single(1),t.v_single(2)),t.v_single(3));
  is2 = t.v_append(t.v_append(t.v_single(3),t.v_single(0)),t.v_single(2));
  cout << iscoll.to_string(is) << " with "
       << iscoll.to_string(is2) << endl;
  cout << "Union = " << iscoll.to_string(t.v_union(is,is2)) << endl;
  cout << "Intersection = " << iscoll.to_string(t.v_intersect(is,is2)) << endl;
  cout << "Difference = " << iscoll.to_string(t.v_difference(is,is2)) << endl;
}
