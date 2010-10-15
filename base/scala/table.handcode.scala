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


class M_TABLE[T_KeyType, T_ValueType](name : String,val t_KeyType : C_TYPE[T_KeyType] with C_ORDERED[T_KeyType],val t_ValueType : C_TYPE[T_ValueType] with C_COMBINABLE[T_ValueType]) extends Module(name) {
  val m_tmp = new M_TYPE(name);
  type T_tmp = m_tmp.T_Result;
  val t_tmp = m_tmp.t_Result;

  implicit def key_order(x : T_KeyType) = new Ordered[T_KeyType] {
    def compare(y : T_KeyType) : Int = {
      if (t_KeyType.v_equal(x,y)) return 0;
      if (t_KeyType.v_less(x,y)) return -1;
      else return 1;
    }
  };

  import scala.collection.immutable.TreeMap;

  type Table = TreeMap[T_KeyType,T_ValueType];

  type T_Result = T_tmp;
  object t_Result extends C_TABLE[T_Result,T_KeyType,T_ValueType] {
    val v_equal = t_tmp.v_equal;
    val v_string = t_tmp.v_string;
    val v_assert = t_tmp.v_assert;
    val v_node_equivalent = t_tmp.v_node_equivalent;

    case class c_full_table(v_entries : Table) extends T_Result {
      override def toString() : String = Debug.with_level {
        "table("+ v_entries+ ")";
      }
    }
    val v_empty_table = f_empty_table _;
    def f_empty_table():T_Result = c_full_table(TreeMap.empty);

    case class c_table_entry(v_key : T_KeyType,v_val : T_ValueType) extends T_Result {
      override def toString() : String = Debug.with_level {
        "table_entry("+ v_key + ","+ v_val+ ")";
      }
    }
    val v_table_entry = f_table_entry _;
    def f_table_entry(v_key : T_KeyType, v_val : T_ValueType):T_Result = c_table_entry(v_key,v_val);
    def u_table_entry(x:Any) : Option[(T_KeyType,T_ValueType)] = x match {
      case c_table_entry(v_key,v_val) => Some((v_key,v_val));
      case _ => None };
    val p_table_entry = new PatternFunction[(T_KeyType,T_ValueType),T_Result](u_table_entry);

    def canon(v : T_Result) : Table = v match {
      case c_full_table(t) => t
      case c_table_entry(k,v) => 
	TreeMap.empty[T_KeyType,T_ValueType].insert(k,v)
    }

    val v_initial:T_Result = f_empty_table();

    val v_combine = f_combine _;
    def f_combine(v_t1 : T_Result, v_t2 : T_Result):T_Result = {
      var result = canon(v_t1);
      for ((k,v) <- canon(v_t2)) {
	if (result.isDefinedAt(k)) {
	  result = result.update(k,t_ValueType.v_combine(result(k),v))
	} else {
	  result = result.insert(k,v);
	}
      };
      c_full_table(result);
    }

    val v_select = f_select _;
    def f_select(v_table : T_Result, v_key : T_KeyType):T_Result = {
      canon(v_table).get(v_key) match {
	case Some(v) => c_table_entry(v_key,v)
	case None => c_table_entry(v_key,t_ValueType.v_initial)
      }
    }
  }

  override def finish() : Unit = {
    super.finish();
  }
}

