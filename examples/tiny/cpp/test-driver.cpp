#include <iostream>
#include "aps-impl.h"
#include "basic.h"
#include "test.h"

using namespace std;

void print_list(C_LIST<C_String>* type, C_LIST<C_String>::T_Result a_list)
{
  if (a_list->cons == type->c_append) {
    C_LIST<C_String>::V_append *n = (C_LIST<C_String>::V_append*)a_list;
    print_list(type,n->v_l1);
    print_list(type,n->v_l2);
  } else if (a_list->cons == type->c_single) {
    C_LIST<C_String>::V_single *n = (C_LIST<C_String>::V_single*)a_list;
    cout << n->v_x << endl;
  } else if (a_list->cons == type->c_none) {
  } else {
    cout << "Badly typed?\n";
  }
}

int main()
{
  C_TEST *t = new C_TEST();
  print_list(t->t_Test,t->v_t);
  cout << "t is " << COLL<C_TEST::C_Test,C_String>(t->t_Test,t_String).to_string(t->v_t) << endl;
  
  for (int i=0; i < 10; ++i) {
    cout << "nth(" << i << ") ";
    try {
      string s = t->t_Test->v_nth(i,t->v_t);
      cout << " = " << s << endl;
    } catch (exception& w) {
      cout << "throws exception: " << w.what() << endl;
    }
    cout << "butnth(" << i << ") ";
    try {
      C_LIST<C_String>::T_Result l = t->t_Test->v_butsubseq(t->v_t,i,i+1);
      cout << " = ";
      print_list(t->t_Test,l);
    } catch (exception& w) {
      cout << "throws exception: " << w.what() << endl;
    }
  }
}
