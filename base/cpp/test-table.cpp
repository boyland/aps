// test code
#include <iostream>
#include "aps-impl.h"
#include "basic.h"
#include "table.h"

using namespace std;

int main ()
{
  Debug::out(cout);
  typedef C_BAG<C_String> C_Strings;
  C_Strings t_Strings(t_String);
  typedef COLL<C_Strings,C_String> Coll;

  C_Strings::T_Result b1 = t_Strings.v_single("s1");
  C_Strings::T_Result b2a = t_Strings.v_single("s2a");
  C_Strings::T_Result b2b = t_Strings.v_single("s2b");
  C_Strings::T_Result b2 = t_Strings.v_append(b2a,b2b);
  C_Strings::T_Result b3 = t_Strings.v_none();

  typedef C_TABLE<C_Integer,C_Strings> C_Table;
  C_Table t_Table(t_Integer,&t_Strings);
  C_Table::T_Result t1 = t_Table.v_table_entry(3,b1);
  C_Table::T_Result t2 = t_Table.v_table_entry(3,b2);
  C_Table::T_Result t3 = t_Table.v_table_entry(1,b3);
  t1 = t_Table.v_combine(t1,t3);
  t1 = t_Table.v_combine(t1,t2);

  for (int i=1; i < 4; ++i) {
    C_Table::T_Result t4 = t_Table.v_select(t1,i);

    cout << "t.select(" << i << ") = " << t4 << endl;
    if (C_Table::V_table_entry* te =
	dynamic_cast<C_Table::V_table_entry*>(t4)) {
      cout << "t[" << i << "] = " <<
	Coll(&t_Strings,t_String).to_string(te->v_val) << endl;
    } else {
      cout << "t[" << i << "] is undefined" << endl;
    }
  }
}
