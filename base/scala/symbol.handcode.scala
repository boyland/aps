// partially handcoded SYMBOL module
import basic_implicit._;

trait C_SYMBOL[T_Result] extends C_BASIC[T_Result] with C_PRINTABLE[T_Result] with C_ORDERED[T_Result] with C_TYPE[T_Result] {
  val v_assert : (T_Result) => Unit;
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_create : (T_String) => T_Result;
  val v_name : (T_Result) => T_String;
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
  val v_string : (T_Result) => T_String;
  def v_null : T_Result;
}

class M_SYMBOL(name : String)  
  extends I_TYPE[Symbol](name) 
  with C_SYMBOL[Symbol] 
{
  override def f_equal(x : Symbol, y : Symbol) : Boolean = x eq y;
  override def f_string(x : Symbol) : String = x.name;

  val v_create = f_create _;
  def f_create(v__4 : T_String):T_Result = Symbol(v__4);
  val v_name = f_name _;
  def f_name(v__5 : T_Result):T_String = v__5.name;
  val v_less = f_less _;
  def f_less(v__6 : T_Result, v__7 : T_Result):T_Boolean =
    v__6.hashCode() < v__7.hashCode();
  val v_less_equal = f_less_equal _;
  def f_less_equal(v__8 : T_Result, v__9 : T_Result):T_Boolean =
    v__8.hashCode() <= v__9.hashCode();
  var v_null:T_Result = Symbol("nil");
}

