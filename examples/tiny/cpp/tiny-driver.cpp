#include <iostream>
#include <stdexcept>
#include "aps-impl.h"
#include "basic.h"
#include "tiny.h"

using namespace std;

int main()
{
  try {
    C_TINY m;

    C_SEQUENCE<C_TINY::C_Wood> seq_type(m.t_Wood);
    
    C_TINY::T_Wood w = m.v_branch(m.v_leaf(3),m.v_leaf(4));
    
    m.finish();

    cout << "result is " << (w) << endl;
  } catch (exception& e) {
    cout << "Got error: " << e.what() << endl;
  }
}
