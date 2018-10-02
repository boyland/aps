#include <iostream>
#include "aps-impl.h"
#include "basic.h"

using namespace std;

typedef C_LIST<C_INTEGER> C_IntegerList;
typedef C_IntegerList::T_Result T_IntegerList;

bool eq_int(int x, int y) { return x == y; }

main()
{
  T_IntegerList is, is2;
  C_IntegerList* t_IntegerList = new C_IntegerList(get_Integer());
  COLL<C_IntegerList,C_Integer> iscoll(t_IntegerList,get_Integer());
  C_IntegerList& t = *t_IntegerList;
  is = t.v_append(t.v_append(t.v_single(1),t.v_single(2)),t.v_single(3));
  // is2 = t.v_append(t.v_append(t.v_single(3),t.v_single(0)),t.v_single(2));
  cout << iscoll.to_string(is) << endl;
  for (int i=0; i < 5; ++i) {
    cout << "Position(" << i << ") = " << t.v_position(i,is) << endl;
    cout << "Member(" << i << ") = " << t.v_member(i,is) << endl;
    cout << "NthFromEnd(" << i << ") = " << t.v_nth_from_end(i,is) << endl;
  }
}
