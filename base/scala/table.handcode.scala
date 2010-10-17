// Handcoded tables for APS
// John Boyland
// October 2010

import basic_implicit._;
object table_implicit {
  val table_loaded = true;
}
import table_implicit._;

trait C_TABLE[T_Result, T_KeyType, T_ValueType] extends C_TYPE[T_Result] with C_COMBINABLE[T_Result] {
  val p_table_entry : PatternFunction[(T_KeyType,T_ValueType),T_Result];
  def v_table_entry : (T_KeyType,T_ValueType) => T_Result;
  def v_initial : T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
  val v_select : (T_Result,T_KeyType) => T_Result;
}

import scala.collection.immutable.TreeMap;

class M_TABLE[T_KeyType, T_ValueType]
             (name : String,
	      val t_KeyType : C_TYPE[T_KeyType] with C_ORDERED[T_KeyType],
	      val t_ValueType : C_TYPE[T_ValueType] with C_COMBINABLE[T_ValueType])
extends I_TYPE[TreeMap[T_KeyType,T_ValueType]](name)
with C_TABLE[TreeMap[T_KeyType,T_ValueType],T_KeyType,T_ValueType]
{
  implicit def key_order(x : T_KeyType) = new Ordered[T_KeyType] {
    def compare(y : T_KeyType) : Int = {
      if (t_KeyType.v_equal(x,y)) return 0;
      if (t_KeyType.v_less(x,y)) return -1;
      else return 1;
    }
  };

  type Table = TreeMap[T_KeyType,T_ValueType];

  val v_empty_table = new TreeMap[T_KeyType,T_ValueType];

  val v_table_entry = f_table_entry _;
  def f_table_entry(v_key : T_KeyType, v_val : T_ValueType):T_Result = 
    v_empty_table.insert(v_key,v_val);

  def u_table_entry(x:Any) : Option[(T_KeyType,T_ValueType)] = x match {
    case m:Table => {
      if (m.size == 1) {
	val k = m.firstKey;
	Some((k,m(k)))
      } else None
    }
    case _ => None
  };
  val p_table_entry = new PatternFunction[(T_KeyType,T_ValueType),T_Result](u_table_entry);

  val v_initial:T_Result = v_empty_table;

  val v_combine = f_combine _;
  def f_combine(v_t1 : T_Result, v_t2 : T_Result):T_Result = {
    var result : Table = v_t1;
    for ((k,v) <- v_t2) {
      if (result.isDefinedAt(k)) {
	result = result.update(k,t_ValueType.v_combine(result(k),v))
      } else {
	result = result.insert(k,v);
      }
    };
    result
  };

  val v_select = f_select _;
  def f_select(v_table : T_Result, v_key : T_KeyType):T_Result = {
    v_table.get(v_key) match {
      case Some(v) => f_table_entry(v_key,v)
      case None => f_table_entry(v_key,t_ValueType.v_initial)
    }
  }
}

