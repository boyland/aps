import basic_implicit._;
object symbol_implicit {
  val cool_symbol_loaded = true;
  val t_Symbol = new M_SYMBOL("Symbol");
  type T_Symbol = /*TI*/t_Symbol.T_Result;
  val v_make_symbol : (T_String) => T_Symbol = t_Symbol.v_create;
  val v_symbol_name : (T_Symbol) => T_String = t_Symbol.v_name;
  val v_symbol_equal : (T_Symbol,T_Symbol) => T_Boolean = t_Symbol.v_equal;
  val v_null_symbol:T_Symbol = t_Symbol.v_null;
}
import symbol_implicit._;

