#include <iostream>
#include <stdexcept>
#include "aps-impl.h"
#include "basic.h"
#include "test-coll.h"

using namespace std;

typedef C_BAG<C_Integer> C_Integers;

int main()
{
  try {
    C_TINY m;

    C_SEQUENCE<C_TINY::C_Wood> seq_type(m.t_Wood);
    
    C_TINY::T_Wood w = m.v_branch(m.v_leaf(3),m.v_leaf(4));

    Debug::out(cout);

    m.finish();

    cout << "sum is " << m.v_sum << endl;
    cout << "leaves is "
	 << COLL<C_Integers,C_Integer>
            (m.t_Integers,t_Integer).to_string(m.v_leaves)
	 << endl;
  } catch (exception& e) {
    cout << "Got error: " << e.what() << endl;
  }
}
