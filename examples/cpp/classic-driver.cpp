#include "aps-impl.h"
#include "basic.h"
#include "simple.h"
#include "classic-binding.h"

#include <iostream>
#include <stdexcept>

using namespace std;

typedef C_NAME_RESOLUTION<C_SIMPLE> BINDING;

void print_bag(C_BAG<C_String>* type, C_BAG<C_String>::T_Result a_bag)
{
  if (a_bag->cons == type->c_append) {
    C_BAG<C_String>::V_append *n = (C_BAG<C_String>::V_append*)a_bag;
    print_bag(type,n->v_l1);
    print_bag(type,n->v_l2);
  } else if (a_bag->cons == type->c_single) {
    C_BAG<C_String>::V_single *n = (C_BAG<C_String>::V_single*)a_bag;
    cout << n->v_x;
  } else if (a_bag->cons == type->c_none) {
  } else {
    cout << "Badly typed?\n";
  }
}

int main()
{
  C_SIMPLE* simple = new C_SIMPLE();
  C_SIMPLE::T_Decls ds =
    simple->v_xcons_decls(simple->v_no_decls(),
			  simple->v_decl("x",simple->v_integer()));
  C_SIMPLE::T_Stmt s =
    simple->v_assign_stmt(simple->v_variable("x"),simple->v_variable("y"));
  C_SIMPLE::T_Stmts ss = simple->v_xcons_stmts(simple->v_no_stmts(),s);
					    
  C_SIMPLE::T_Program p = simple->v_program(simple->v_block(ds,ss));

  Debug::out(cout);

  try {
    BINDING* binding =
      new BINDING(simple);
    
    binding->finish();
    cout << "Messages:" << endl;
    print_bag(binding->t_Messages,binding->v_prog_msgs(p));
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << endl;
  }
}
