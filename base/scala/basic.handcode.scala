import Evaluation._;
import implicit_2._;
import implicit_34._;

trait C_NULL[T_Result] {
}

trait C_BASIC[T_Result] extends C_NULL[T_Result] {
  val v_equal : (T_Result,T_Result) => T_Boolean;
}

class M__basic_2[T_T](t_T:C_BASIC[T_T]) {
  val v__op_0 : (T_T,T_T) => T_Boolean = t_T.v_equal;
  val v__op_w0 = f__op_w0 _;
  def f__op_w0(v_x : T_T, v_y : T_T):T_Boolean = v_not(t_T.v_equal(v_x,v_y));
};

trait C_PRINTABLE[T_Result] {
  val v_string : (T_Result) => T_String;
}

trait C_COMPARABLE[T_Result] extends C_BASIC[T_Result] {
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
}

class M__basic_3[T_T](t_T:C_COMPARABLE[T_T]) {
  val v__op_z : (T_T,T_T) => T_Boolean = t_T.v_less;
  val v__op_z0 : (T_T,T_T) => T_Boolean = t_T.v_less_equal;
  val v__op_1 = f__op_1 _;
  def f__op_1(v_x : T_T, v_y : T_T):T_Boolean = t_T.v_less(v_y,v_x);
  val v__op_10 = f__op_10 _;
  def f__op_10(v_x : T_T, v_y : T_T):T_Boolean = t_T.v_less_equal(v_y,v_x);
};

trait C_ORDERED[T_Result] extends C_COMPARABLE[T_Result] {
}

trait C_NUMERIC[T_Result] extends C_BASIC[T_Result] {
  val v_zero : T_Result;
  val v_one : T_Result;
  val v_plus : (T_Result,T_Result) => T_Result;
  val v_minus : (T_Result,T_Result) => T_Result;
  val v_times : (T_Result,T_Result) => T_Result;
  val v_divide : (T_Result,T_Result) => T_Result;
  val v_unary_plus : (T_Result) => T_Result;
  val v_unary_minus : (T_Result) => T_Result;
  val v_unary_times : (T_Result) => T_Result;
  val v_unary_divide : (T_Result) => T_Result;
}

class M__basic_4[T_T](t_T:C_NUMERIC[T_T]) {
  val v__op_s : (T_T,T_T) => T_T = t_T.v_plus;
  val v__op_u : (T_T,T_T) => T_T = t_T.v_minus;
  val v__op_r : (T_T,T_T) => T_T = t_T.v_times;
  val v__op_w : (T_T,T_T) => T_T = t_T.v_divide;
  val v__op_ks : (T_T) => T_T = t_T.v_unary_plus;
  val v__op_ku : (T_T) => T_T = t_T.v_unary_minus;
  val v__op_kr : (T_T) => T_T = t_T.v_unary_times;
  val v__op_kw : (T_T) => T_T = t_T.v_unary_divide;
};

class M__basic_5[T_T](t_T:C_NUMERIC[T_T] with C_ORDERED[T_T]) {
  val v_abs = f_abs _;
  def f_abs(v_x : T_T):T_T = {
    { val cond = new M__basic_3[ T_T](t_T).v__op_z(v_x,t_T.v_zero);
      if (cond) {
        return new M__basic_4[ T_T](t_T).v__op_ku(v_x);
      }
      if (!cond) {
        return v_x;
      }
    }
    throw UndefinedAttributeException("local abs");
  }

};

trait C_NULL_TYPE[T_Result] extends C_NULL[T_Result] {
}

class M_NULL_TYPE extends Module("NULL_TYPE") {
  class T_tmp extends Type {
    def getType() = t_tmp;
  }
  object t_tmp extends I_TYPE[T_tmp] {}
  type T_Result = T_tmp;
  object t_Result extends C_NULL_TYPE[T_Result] {
    val v_assert = t_tmp.v_assert;
    val v_equal = t_tmp.v_equal;
    val v_node_equivalent = t_tmp.v_node_equivalent;
    val v_string = t_tmp.v_string;

    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

trait C_BOOLEAN[T_Result] extends C_BASIC[T_Result] with C_PRINTABLE[T_Result] {
  val v_assert : (T_Result) => Unit;
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_string : (T_Result) => T_String;
}

class M_BOOLEAN extends Module("BOOLEAN") {
  type T_Result = Boolean;
  object t_Result extends C_BOOLEAN[T_Result] {
    val v_node_equivalent = f_equal _;
    val v_assert = f_assert _;
    def f_assert(v__22 : T_Result) : Unit = { }
    val v_equal = f_equal _;
    def f_equal(v_x : T_Result, v_y : T_Result):T_Boolean = v_x == v_y;
    val v_string = f_string _;
    def f_string(v_x : T_Result):T_String = v_x.toString();
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

object implicit_2 {
  val m_Boolean = new M_BOOLEAN;
  type T_Boolean = m_Boolean.T_Result;
  val t_Boolean = m_Boolean.t_Result;
  var v_true:T_Boolean = true;
  var v_false:T_Boolean = false;
  val v_and = f_and _;
  def f_and(v__23 : T_Boolean, v__24 : T_Boolean):T_Boolean = v__23 && v__24;
  val v_or = f_or _;
  def f_or(v__25 : T_Boolean, v__26 : T_Boolean):T_Boolean = v__25 || v__26;
  val v_not = f_not _;
  def f_not(v__27 : T_Boolean):T_Boolean = !v__27;
}

trait C_INTEGER[T_Result] extends C_NUMERIC[T_Result] with C_ORDERED[T_Result] with C_PRINTABLE[T_Result] {
  val v_assert : (T_Result) => Unit;
  val v_zero : T_Result;
  val v_one : T_Result;
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
  val v_plus : (T_Result,T_Result) => T_Result;
  val v_minus : (T_Result,T_Result) => T_Result;
  val v_times : (T_Result,T_Result) => T_Result;
  val v_divide : (T_Result,T_Result) => T_Result;
  val v_unary_plus : (T_Result) => T_Result;
  val v_unary_minus : (T_Result) => T_Result;
  val v_unary_times : (T_Result) => T_Result;
  val v_unary_divide : (T_Result) => T_Result;
  val v_string : (T_Result) => T_String;
}

class M_INTEGER extends Module("INTEGER") {
  type T_Result = Int;
  object t_Result extends C_INTEGER[T_Result] {
    val v_node_equivalent = f_equal _;

    val v_assert = f_assert _;
    def f_assert(v__28 : T_Result) : Unit = {};
    val v_zero:T_Result = 0;
    val v_one:T_Result = 1;
    val v_equal = f_equal _;
    def f_equal(v_x : T_Result, v_y : T_Result):T_Boolean = v_x == v_y;
    val v_less = f_less _;
    def f_less(v_x : T_Result, v_y : T_Result):T_Boolean = v_x < v_y;
    val v_less_equal = f_less_equal _;
    def f_less_equal(v_x : T_Result, v_y : T_Result):T_Boolean = v_x <= v_y;
    val v_plus = f_plus _;
    def f_plus(v_x : T_Result, v_y : T_Result):T_Result = v_x + v_y;
    val v_minus = f_minus _;
    def f_minus(v_x : T_Result, v_y : T_Result):T_Result = v_x - v_y;
    val v_times = f_times _;
    def f_times(v_x : T_Result, v_y : T_Result):T_Result = v_x * v_y;
    val v_divide = f_divide _;
    def f_divide(v_x : T_Result, v_y : T_Result):T_Result = v_x / v_y;
    val v_unary_plus = f_unary_plus _;
    def f_unary_plus(v_x : T_Result):T_Result = v_x;
    val v_unary_minus = f_unary_minus _;
    def f_unary_minus(v_x : T_Result):T_Result = -v_x;
    val v_unary_times = f_unary_times _;
    def f_unary_times(v_x : T_Result):T_Result = v_x;
    val v_unary_divide = f_unary_divide _;
    def f_unary_divide(v_x : T_Result):T_Result = 1/v_x;
    val v_string = f_string _;
    def f_string(v_x : T_Result):T_String = v_x.toString();
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

object implicit_8 {
  val m_Integer = new M_INTEGER;
  type T_Integer = m_Integer.T_Result;
  val t_Integer = m_Integer.t_Result;
  val v_lognot = f_lognot _;
  def f_lognot(v_x : T_Integer):T_Integer = ~v_x;
  val v_logior = f_logior _;
  def f_logior(v_x : T_Integer, v_y : T_Integer):T_Integer = v_x | v_y;
  val v_logand = f_logand _;
  def f_logand(v_x : T_Integer, v_y : T_Integer):T_Integer = v_x & v_y;
  val v_logandc2 = f_logandc2 _;
  def f_logandc2(v_x : T_Integer, v_y : T_Integer):T_Integer = v_x & (~v_y);
  val v_logxor = f_logxor _;
  def f_logxor(v_x : T_Integer, v_y : T_Integer):T_Integer = v_x ^ v_y;
  // val v_logbitp = f_logbitp _;
  // def f_logbitp(v_index : T_Integer, v_set : T_Integer):T_Integer;
  // val v_ash = f_ash _;
  // def f_ash(v_n : T_Integer, v_count : T_Integer):T_Integer;
  val v_odd = f_odd _;
  def f_odd(v__29 : T_Integer):T_Boolean = ((v__29 & 1) == 0);
}
import implicit_8._

class M__basic_6[T_T](t_T:C_NUMERIC[T_T]) {
  val v__op_7 = f__op_7 _;
  def f__op_7(v_x : T_T, v_y : T_Integer):T_T = {
    { val cond = new M__basic_2[ T_Integer](t_Integer).v__op_0(v_y,0);
      if (cond) {
        return t_T.v_one;
      }
      if (!cond) {
        { val cond = new M__basic_3[ T_Integer](t_Integer).v__op_z(v_y,0);
          if (cond) {
            return new M__basic_4[ T_T](t_T).v__op_w(t_T.v_one,v__op_7(v_x,new M__basic_4[ T_Integer](t_Integer).v__op_ku(v_y)));
          }
          if (!cond) {
            { val cond = new M__basic_2[ T_Integer](t_Integer).v__op_0(v_y,1);
              if (cond) {
                return v_x;
              }
              if (!cond) {
                { val cond = v_odd(v_y);
                  if (cond) {
                    return new M__basic_4[ T_T](t_T).v__op_r(v_x,v__op_7(v_x,new M__basic_4[ T_Integer](t_Integer).v__op_u(v_y,1)));
                  }
                  if (!cond) {
                    return v__op_7(new M__basic_4[ T_T](t_T).v__op_r(v_x,v_x), new M__basic_4[ T_Integer](t_Integer).v__op_w(v_y,2));
                  }
                }
              }
            }
          }
        }
      }
    }
    throw UndefinedAttributeException("local ^");
  }

};

/*
trait C_REAL[T_Result] extends C_NUMERIC[T_Result] with C_ORDERED[T_Result] {
  val v_from_integer : (T_Integer) => T_Result;
  val v_to_integer : (T_Result) => T_Integer;
}

trait C_IEEE[T_Result] extends C_REAL[T_Result] with C_PRINTABLE[T_Result] {
  val v_assert : (T_Result) => Unit;
  val v_zero : T_Result;
  val v_one : T_Result;
  val v_max : T_Result;
  val v_min : T_Result;
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
  val v_plus : (T_Result,T_Result) => T_Result;
  val v_minus : (T_Result,T_Result) => T_Result;
  val v_times : (T_Result,T_Result) => T_Result;
  val v_divide : (T_Result,T_Result) => T_Result;
  val v_unary_plus : (T_Result) => T_Result;
  val v_unary_minus : (T_Result) => T_Result;
  val v_unary_times : (T_Result) => T_Result;
  val v_unary_divide : (T_Result) => T_Result;
  val v_from_integer : (T_Integer) => T_Result;
  val v_to_integer : (T_Result) => T_Integer;
  val v_string : (T_Result) => T_String;
}

class M_IEEE extends Module("IEEE") {
  class T_tmp extends Type {
    def getType() = t_tmp;
  }
  object t_tmp extends I_TYPE[T_tmp] {}
  type T_Result = T_tmp;
  object t_Result extends C_IEEE[T_Result] {
    val v_node_equivalent = t_tmp.v_node_equivalent;

    val v_assert = f_assert _;
    def f_assert(v__32 : T_Result);
    var v_zero:T_Result;
    var v_one:T_Result;
    var v_max:T_Result;
    var v_min:T_Result;
    val v_equal = f_equal _;
    def f_equal(v_x : T_Result, v_y : T_Result):T_Boolean;
    val v_less = f_less _;
    def f_less(v_x : T_Result, v_y : T_Result):T_Boolean;
    val v_less_equal = f_less_equal _;
    def f_less_equal(v_x : T_Result, v_y : T_Result):T_Boolean;
    val v_plus = f_plus _;
    def f_plus(v_x : T_Result, v_y : T_Result):T_Result;
    val v_minus = f_minus _;
    def f_minus(v_x : T_Result, v_y : T_Result):T_Result;
    val v_times = f_times _;
    def f_times(v_x : T_Result, v_y : T_Result):T_Result;
    val v_divide = f_divide _;
    def f_divide(v_x : T_Result, v_y : T_Result):T_Result;
    val v_unary_plus = f_unary_plus _;
    def f_unary_plus(v_x : T_Result):T_Result = v_x;
    val v_unary_minus = f_unary_minus _;
    def f_unary_minus(v_x : T_Result):T_Result;
    val v_unary_times = f_unary_times _;
    def f_unary_times(v_x : T_Result):T_Result = v_x;
    val v_unary_divide = f_unary_divide _;
    def f_unary_divide(v_x : T_Result):T_Result;
    val v_from_integer = f_from_integer _;
    def f_from_integer(v_y : T_Integer):T_Result;
    val v_to_integer = f_to_integer _;
    def f_to_integer(v_x : T_Result):T_Integer;
    val v_string = f_string _;
    def f_string(v_x : T_Result):T_String;
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

object implicit_17{
  val m_IEEEdouble = new M_IEEE;
  type T_IEEEdouble = m_IEEEdouble.T_Result;
  val t_IEEEdouble = m_IEEEdouble.t_Result;

}
import implicit_17._

object implicit_18{
  val m_IEEEsingle = new M_IEEE;
  type T_IEEEsingle = m_IEEEsingle.T_Result;
  val t_IEEEsingle = m_IEEEsingle.t_Result;

}
import implicit_18._

object implicit_19{
  val v_IEEEwiden = f_IEEEwiden _;
  def f_IEEEwiden(v_x : T_IEEEsingle):T_IEEEdouble;
}
import implicit_19._

object implicit_20{
  val v_IEEEnarrow = f_IEEEnarrow _;
  def f_IEEEnarrow(v_x : T_IEEEdouble):T_IEEEsingle;
}
import implicit_20._

object implicit_21{
  type T_Real = T_IEEEdouble;
  val t_Real = t_IEEEdouble;
}
import implicit_21._
*/

trait C_CHARACTER[T_Result] extends C_ORDERED[T_Result] with C_PRINTABLE[T_Result] {
  val v_assert : (T_Result) => Unit;
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
  val v_string : (T_Result) => T_String;
}

class M_CHARACTER extends Module("CHARACTER") {
  type T_Result = Char;
  object t_Result extends C_CHARACTER[T_Result] {
    val v_node_equivalent = f_equal _;

    val v_assert = f_assert _;
    def f_assert(v__33 : T_Result) : Unit = {}
    val v_equal = f_equal _;
    def f_equal(v_x : T_Result, v_y : T_Result):T_Boolean = v_x == v_y;
    val v_less = f_less _;
    def f_less(v_x : T_Result, v_y : T_Result):T_Boolean = v_x < v_y;
    val v_less_equal = f_less_equal _;
    def f_less_equal(v_x : T_Result, v_y : T_Result):T_Boolean = v_x <= v_y;
    val v_string = f_string _;
    def f_string(v_x : T_Result):T_String = v_x.toString();
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

object implicit_22{
  val m_Character = new M_CHARACTER;
  type T_Character = m_Character.T_Result;
  val t_Character = m_Character.t_Result;
  val v_char_code = f_char_code _;
  def f_char_code(v_x : T_Character):T_Integer = v_x;
  val v_int_char = f_int_char _;
  def f_int_char(v_x : T_Integer):T_Character = v_x.asInstanceOf[Char];
  var v_tab:T_Character = v_int_char(9);
  var v_newline:T_Character = v_int_char(10);
}
import implicit_22._;

trait C_NULL_PHYLUM[T_Result] extends C_NULL[T_Result] {
}

class M_NULL_PHYLUM extends Module("NULL_PHYLUM") {
  class T_tmp extends Phylum {
    def getType() : C_PHYLUM[T_tmp] = t_tmp;
  }
  object t_tmp extends I_PHYLUM[T_tmp] {
    def isComplete : Boolean = complete;
  }

  type T_Result = T_tmp;
  object t_Result extends C_NULL_PHYLUM[T_Result] {
    val v_assert = t_tmp.v_assert;
    val v_identical = t_tmp.v_identical;
    val v_equal = t_tmp.v_equal;
    val v_string = t_tmp.v_string;
    val v_object_id = t_tmp.v_object_id;
    val v_object_id_less = t_tmp.v_object_id_less;
    val v_nil = t_tmp.v_nil;

    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

class M__basic_8[T_T](t_T:C_PHYLUM[T_T]) {
  val v__op_00 : (T_T,T_T) => T_Boolean = t_T.v_identical;
  val v__op_kk : (T_T) => T_Integer = t_T.v_object_id;
  val v__op_zk : (T_T,T_T) => T_Boolean = t_T.v_object_id_less;
  /*
  class M__basic_9[T_Other](t_Other:) {
    val v__op_77 : (T_T,T_Other) => T_Boolean = t_T.M__basic_7[ T_Other](t_Other).v_ancestor;
    val v__op_770 : (T_T,T_Other) => T_Boolean = t_T.M__basic_7[ T_Other](t_Other).v_ancestor_equal;
};
*/
  val v_nil : T_T = t_T.v_nil;
  val v__op_w00 = f__op_w00 _;
  def f__op_w00(v_x : T_T, v_y : T_T):T_Boolean = v_not(t_T.v_identical(v_x,v_y));
};

/*
class M__basic_10[T_T,T_U](t_T:C_PHYLUM[T_T],t_U:C_PHYLUM[T_U]) {
  val v__op_zz = f__op_zz _;
  def f__op_zz(v_x : T_T, v_y : T_U):T_Boolean = t_T.M__basic_7[ T_U](t_U).v_precedes(v_x,v_y);
  val v__op_zz0 = f__op_zz0 _;
  def f__op_zz0(v_x : T_T, v_y : T_U):T_Boolean = t_T.M__basic_7[ T_U](t_U).v_precedes_equal(v_x,v_y);
  val v__op_11 = f__op_11 _;
  def f__op_11(v_x : T_T, v_y : T_U):T_Boolean = t_U.M__basic_7[ T_T](t_T).v_precedes(v_y,v_x);
  val v__op_110 = f__op_110 _;
  def f__op_110(v_x : T_T, v_y : T_U):T_Boolean = t_U.M__basic_7[ T_T](t_T).v_precedes_equal(v_y,v_x);
};
*/

trait C_COMBINABLE[T_Result] {
  val v_initial : T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
}

trait C_COMPLETE_PARTIAL_ORDER[T_Result] extends C_BASIC[T_Result] {
  val v_bottom : T_Result;
  val v_compare : (T_Result,T_Result) => T_Boolean;
  val v_compare_equal : (T_Result,T_Result) => T_Boolean;
}

class M__basic_11[T_T](t_T:C_COMPLETE_PARTIAL_ORDER[T_T]) {
  val v__op_BzB : (T_T,T_T) => T_Boolean = t_T.v_compare;
  val v__op_Bz0B : (T_T,T_T) => T_Boolean = t_T.v_compare_equal;
  val v__op_B1B = f__op_B1B _;
  def f__op_B1B(v_x : T_T, v_y : T_T):T_Boolean = t_T.v_compare(v_y,v_x);
  val v__op_B10B = f__op_B10B _;
  def f__op_B10B(v_x : T_T, v_y : T_T):T_Boolean = t_T.v_compare_equal(v_y,v_x);
};

trait C_LATTICE[T_Result] extends C_COMPLETE_PARTIAL_ORDER[T_Result] {
  val v_join : (T_Result,T_Result) => T_Result;
  val v_meet : (T_Result,T_Result) => T_Result;
}

class M__basic_12[T_T](t_T:C_LATTICE[T_T]) {
  val v__op_B5wB : (T_T,T_T) => T_T = t_T.v_join;
  val v__op_Bw5B : (T_T,T_T) => T_T = t_T.v_meet;
};

trait C_MAKE_LATTICE[T_Result, T_L] extends C_COMBINABLE[T_Result] with C_LATTICE[T_Result] {
}

class M_MAKE_LATTICE[T_L](t_L:C_BASIC[T_L],v_default : T_L,v_comparef : (T_L,T_L) => T_Boolean,v_compare_equalf : (T_L,T_L) => T_Boolean,v_joinf : (T_L,T_L) => T_L,v_meetf : (T_L,T_L) => T_L) extends Module("MAKE_LATTICE") {
  type T_tmp = T_L;
  val t_tmp = t_L;
  type T_Result = T_tmp;
  object t_Result extends C_MAKE_LATTICE[T_Result,T_L] {
    val v_equal = t_L.v_equal;

    val v_initial : T_L = v_default;
    val v_combine : (T_L,T_L) => T_L = v_joinf;
    val v_bottom : T_L = v_default;
    val v_compare : (T_L,T_L) => T_Boolean = v_comparef;
    val v_compare_equal : (T_L,T_L) => T_Boolean = v_compare_equalf;
    val v_join : (T_L,T_L) => T_L = v_joinf;
    val v_meet : (T_L,T_L) => T_L = v_meetf;
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

object implicit_28 {
  val v_cand = f_cand _;
  def f_cand(v_x : T_Boolean, v_y : T_Boolean):T_Boolean = v_and(v_not(v_x),v_y);
  val v_implies = f_implies _;
  def f_implies(v_x : T_Boolean, v_y : T_Boolean):T_Boolean = v_or(v_not(v_x),v_y);
  val v_andc = f_andc _;
  def f_andc(v_x : T_Boolean, v_y : T_Boolean):T_Boolean = v_and(v_x,v_not(v_y));
  val v_revimplies = f_revimplies _;
  def f_revimplies(v_x : T_Boolean, v_y : T_Boolean):T_Boolean = v_or(v_x,v_not(v_y));
  val m_OrLattice = new M_MAKE_LATTICE[T_Boolean](t_Boolean,v_false,v_cand,v_implies,v_or,v_and);
  type T_OrLattice = m_OrLattice.T_Result;
  val t_OrLattice = m_OrLattice.t_Result;
  val m_AndLattice = new M_MAKE_LATTICE[T_Boolean](t_Boolean,v_true,v_andc,v_revimplies,v_and,v_or);
  type T_AndLattice = m_AndLattice.T_Result;
  val t_AndLattice = m_AndLattice.t_Result;
}
import implicit_28._;

class M__basic_13[T_T](t_T:C_ORDERED[T_T]) {
  val v_max = f_max _;
  def f_max(v_x : T_T, v_y : T_T):T_T = {

    { val cond = new M__basic_3[ T_T](t_T).v__op_1(v_x,v_y);
      if (cond) {
        return v_x;
      }
      if (!cond) {
        return v_y;
      }
    }
    throw UndefinedAttributeException("local max");
  }

  val v_min = f_min _;
  def f_min(v_x : T_T, v_y : T_T):T_T = {

    { val cond = new M__basic_3[ T_T](t_T).v__op_z(v_x,v_y);
      if (cond) {
        return v_x;
      }
      if (!cond) {
        return v_y;
      }
    }
    throw UndefinedAttributeException("local min");
  }

};

trait C_MAX_LATTICE[T_Result, T_TO] extends C_MAKE_LATTICE[T_Result,T_TO] {
}

class M_MAX_LATTICE[T_TO](t_TO:C_ORDERED[T_TO],v_min_element : T_TO) extends Module("MAX_LATTICE") {
  val m_tmp = new M_MAKE_LATTICE[T_TO](t_TO,v_min_element,
				       new M__basic_3[ T_TO](t_TO).v__op_z,
				       new M__basic_3[ T_TO](t_TO).v__op_z0,
				       new M__basic_13[ T_TO](t_TO).v_max,
				       new M__basic_13[ T_TO](t_TO).v_min);
  type T_tmp = m_tmp.T_Result;
  val t_tmp = m_tmp.t_Result;

  type T_MaxLattice = T_tmp;
  object t_MaxLattice extends C_MAX_LATTICE[T_MaxLattice,T_TO] {
    val v_initial = t_tmp.v_initial;
    val v_combine = t_tmp.v_combine;
    val v_bottom = t_tmp.v_bottom;
    val v_compare = t_tmp.v_compare;
    val v_compare_equal = t_tmp.v_compare_equal;
    val v_join = t_tmp.v_join;
    val v_meet = t_tmp.v_meet;

    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_MaxLattice.finish();
    super.finish();
  }
}

trait C_MIN_LATTICE[T_Result, T_T] extends C_MAKE_LATTICE[T_Result,T_T] {
}

class M_MIN_LATTICE[T_T](t_T:C_ORDERED[T_T],v_max_element : T_T) extends Module("MIN_LATTICE") {
  val m_tmp = new M_MAKE_LATTICE[T_T](t_T,v_max_element,
				      new M__basic_3[ T_T](t_T).v__op_1,
				      new M__basic_3[ T_T](t_T).v__op_10,
				      new M__basic_13[ T_T](t_T).v_min,
				      new M__basic_13[ T_T](t_T).v_max);
  type T_tmp = m_tmp.T_Result;
  val t_tmp = m_tmp.t_Result;

  type T_MinLattice = T_tmp;
  object t_MinLattice extends C_MIN_LATTICE[T_MinLattice,T_T] {
    val v_initial = t_tmp.v_initial;
    val v_combine = t_tmp.v_combine;
    val v_bottom = t_tmp.v_bottom;
    val v_compare = t_tmp.v_compare;
    val v_compare_equal = t_tmp.v_compare_equal;
    val v_join = t_tmp.v_join;
    val v_meet = t_tmp.v_meet;

    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_MinLattice.finish();
    super.finish();
  }
}

trait C_READ_ONLY_COLLECTION[T_Result, T_ElemType] {
  val p__op_AC : PatternSeqFunction[(T_ElemType),T_Result];
  val p_append : PatternFunction[(T_Result,T_Result),T_Result];
  val p_single : PatternFunction[(T_ElemType),T_Result];
  val p_none : Pattern0Function[T_Result];
  val v_member : (T_ElemType,T_Result) => T_Boolean;
}

trait C_COLLECTION[T_Result, T_ElemType] extends C_READ_ONLY_COLLECTION[T_Result,T_ElemType] {
  val v_append : (T_Result,T_Result) => T_Result;
  val v_single : (T_ElemType) => T_Result;
  val v_none : () => T_Result;
  val v__op_AC : (T_ElemType*) => T_Result;
}

class M__basic_14[T_ElemType,T_T](t_ElemType:Any,t_T:C_READ_ONLY_COLLECTION[T_T,T_ElemType]) {
  val p__op_AC = t_T.p__op_AC;
  val v_member : (T_ElemType,T_T) => T_Boolean = t_T.v_member;
  val v_in : (T_ElemType,T_T) => T_Boolean = t_T.v_member;
};

class M__basic_15[T_ElemType,T_T](t_ElemType:Any,t_T:C_COLLECTION[T_T,T_ElemType]) {
  val v__op_AC : (T_ElemType*) => T_T = t_T.v__op_AC;
};

trait C_READ_ONLY_ORDERED_COLLECTION[T_Result, T_ElemType] extends C_READ_ONLY_COLLECTION[T_Result,T_ElemType] {
  val v_nth : (T_Integer,T_Result) => T_ElemType;
  val v_nth_from_end : (T_Integer,T_Result) => T_ElemType;
  val v_position : (T_ElemType,T_Result) => T_Integer;
  val v_position_from_end : (T_ElemType,T_Result) => T_Integer;
}

trait C_ORDERED_COLLECTION[T_Result, T_ElemType] extends C_READ_ONLY_ORDERED_COLLECTION[T_Result,T_ElemType] with C_COLLECTION[T_Result,T_ElemType] {
  val v_subseq : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_subseq_from_end : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_butsubseq : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_butsubseq_from_end : (T_Result,T_Integer,T_Integer) => T_Result;
}

class M__basic_16[T_E,T_T](t_E:Any,t_T:C_READ_ONLY_ORDERED_COLLECTION[T_T,T_E]) {
  val v_nth : (T_Integer,T_T) => T_E = t_T.v_nth;
  val v_nth_from_end : (T_Integer,T_T) => T_E = t_T.v_nth_from_end;
  val v_position : (T_E,T_T) => T_Integer = t_T.v_position;
  val v_position_from_end : (T_E,T_T) => T_Integer = t_T.v_position_from_end;
  val v_first = f_first _;
  def f_first(v_x : T_T):T_E = t_T.v_nth(0,v_x);
  val v_last = f_last _;
  def f_last(v_x : T_T):T_E = t_T.v_nth_from_end(0,v_x);
};

class M__basic_17[T_E,T_T](t_E:Any,t_T:C_ORDERED_COLLECTION[T_T,T_E]) {
  val v_subseq : (T_T,T_Integer,T_Integer) => T_T = t_T.v_subseq;
  val v_subseq_from_end : (T_T,T_Integer,T_Integer) => T_T = t_T.v_subseq_from_end;
  val v_butsubseq : (T_T,T_Integer,T_Integer) => T_T = t_T.v_butsubseq;
  val v_butsubseq_from_end : (T_T,T_Integer,T_Integer) => T_T = t_T.v_butsubseq_from_end;
  val v_firstn = f_firstn _;
  def f_firstn(v_n : T_Integer, v_x : T_T):T_T = t_T.v_subseq(v_x,0,v_n);
  val v_lastn = f_lastn _;
  def f_lastn(v_n : T_Integer, v_x : T_T):T_T = t_T.v_subseq_from_end(v_x,0,v_n);
  val v_butfirst = f_butfirst _;
  def f_butfirst(v_x : T_T):T_T = t_T.v_butsubseq(v_x,0,1);
  val v_butlast = f_butlast _;
  def f_butlast(v_x : T_T):T_T = t_T.v_butsubseq_from_end(v_x,0,1);
  val v_butfirstn = f_butfirstn _;
  def f_butfirstn(v_n : T_Integer, v_x : T_T):T_T = t_T.v_butsubseq(v_x,0,v_n);
  val v_butlastn = f_butlastn _;
  def f_butlastn(v_n : T_Integer, v_x : T_T):T_T = t_T.v_butsubseq_from_end(v_x,0,v_n);
  val v_butnth = f_butnth _;
  def f_butnth(v_n : T_Integer, v_x : T_T):T_T = t_T.v_butsubseq(v_x,v_n,new M__basic_4[ T_Integer](t_Integer).v__op_s(v_n,1));
  val v_butnth_from_end = f_butnth_from_end _;
  def f_butnth_from_end(v_n : T_Integer, v_x : T_T):T_T = t_T.v_butsubseq_from_end(v_x,v_n, new M__basic_4[ T_Integer](t_Integer).v__op_s(v_n,1));
};

trait C_SEQUENCE[T_Result, T_ElemType] extends C_READ_ONLY_ORDERED_COLLECTION[T_Result,T_ElemType] {
  val v_assert : (T_Result) => Unit;
  val p__op_AC : PatternSeqFunction[(T_ElemType),T_Result];
  val v_nth : (T_Integer,T_Result) => T_ElemType;
  val v_nth_from_end : (T_Integer,T_Result) => T_ElemType;
  val v_position : (T_ElemType,T_Result) => T_Integer;
  val v_position_from_end : (T_ElemType,T_Result) => T_Integer;
  val v_member : (T_ElemType,T_Result) => T_Boolean;
  val p_append : PatternFunction[(T_Result,T_Result),T_Result];
  val v_append : (T_Result,T_Result) => T_Result;
  val p_single : PatternFunction[(T_ElemType),T_Result];
  val v_single : (T_ElemType) => T_Result;
  val p_none : Pattern0Function[T_Result];
  val v_none : () => T_Result;
}

class M_SEQUENCE[T_ElemType <: Phylum](t_ElemType:C_PHYLUM[T_ElemType] with C_BASIC[T_ElemType]) extends Module("SEQUENCE") {
  class T_tmp extends Phylum {
    def getType() : C_PHYLUM[T_tmp] = t_tmp;
  }
  object t_tmp extends I_PHYLUM[T_tmp] {
    def isComplete : Boolean = complete;
  }

  type T_Result = T_tmp;
  object t_Result extends C_SEQUENCE[T_Result,T_ElemType] {
    val v_assert = t_tmp.v_assert;
    val v_identical = t_tmp.v_identical;
    val v_equal = t_tmp.v_equal;
    val v_string = t_tmp.v_string;
    val v_object_id = t_tmp.v_object_id;
    val v_object_id_less = t_tmp.v_object_id_less;
    val v_nil = t_tmp.v_nil;

    // we make things easier (but inefficient) by using lists.

    def toList(x : T_Result) : List[T_ElemType] = x match {
      case c_append(x1,x2) => toList(x1) ++ toList(x2)
      case c_single(e) => List(e)
      case c_none() => List()
    };
    def u__op_AC(x:T_Result) : Seq[T_ElemType] = toList(x);
    val p__op_AC = new PatternSeqFunction[T_ElemType,T_Result](u__op_AC);
    val v_nth = f_nth _;
    def f_nth(v_i : T_Integer, v_l : T_Result):T_ElemType = toList(v_l)(v_i);
    val v_nth_from_end = f_nth_from_end _;
    def f_nth_from_end(v_i : T_Integer, v_l : T_Result):T_ElemType =
      toList(v_l).reverse.apply(v_i);
    val v_position = f_position _;
    def f_position(v_x : T_ElemType, v_l : T_Result):T_Integer = 
      toList(v_l).indexOf(v_x);
    val v_position_from_end = f_position_from_end _;
    def f_position_from_end(v_x : T_ElemType, v_l : T_Result):T_Integer =
      toList(v_l).reverse.indexOf(v_x);
    val v_member = f_member _;
    def f_member(v_x : T_ElemType, v_l : T_Result):T_Boolean = 
      toList(v_l).contains(v_x);
    case class c_append(v_l1 : T_Result,v_l2 : T_Result) extends T_Result {
      def children : List[Phylum] = List(v_l1,v_l2);
    }
    val v_append = f_append _;
    def f_append(v_l1 : T_Result, v_l2 : T_Result):T_Result = c_append(v_l1,v_l2);
    def u_append(x:T_Result) : Option[(T_Result,T_Result)] = x match {
      case c_append(v_l1,v_l2) => Some((v_l1,v_l2));
      case _ => None };
    val p_append = new PatternFunction[(T_Result,T_Result),T_Result](u_append);

    case class c_single(v_x : T_ElemType) extends T_Result {
      def children : List[Phylum] = List(v_x);
    }
    val v_single = f_single _;
    def f_single(v_x : T_ElemType):T_Result = c_single(v_x);
    def u_single(x:T_Result) : Option[(T_ElemType)] = x match {
      case c_single(v_x) => Some((v_x));
      case _ => None };
    val p_single = new PatternFunction[(T_ElemType),T_Result](u_single);

    case class c_none() extends T_Result {
      def children : List[Phylum] = List();
    }
    val v_none = f_none _;
    def f_none():T_Result = c_none();
    def u_none(x:T_Result) : Option[Unit] = x match {
      case c_none() => Some(());
      case _ => None };
    val p_none = new Pattern0Function[T_Result](u_none);

    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

trait C_BAG[T_Result, T_ElemType] extends C_COLLECTION[T_Result,T_ElemType] with C_COMBINABLE[T_Result] {
  val v_assert : (T_Result) => Unit;
  val v__op_AC : (T_ElemType*) => T_Result;
  val p__op_AC : PatternFunction[(T_ElemType),T_Result];
  val v_member : (T_ElemType,T_Result) => T_Boolean;
  val p_append : PatternFunction[(T_Result,T_Result),T_Result];
  val v_append : (T_Result,T_Result) => T_Result;
  val p_single : PatternFunction[(T_ElemType),T_Result];
  val v_single : (T_ElemType) => T_Result;
  val p_none : Pattern0Function[T_Result];
  val v_none : () => T_Result;
  val v_initial : T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
}

class I_BAG[T_ElemType] extends C_BAG[List[T_ElemType],T_ElemType] {
  type T_Result = List[T_ElemType];

  val v_equal = f_equal _;
  def f_equal(x : T_Result, y : T_Result) = x == y;
  
  val v_node_equivalent = f_equal _;
  
  val v_string = f_string _;
  def f_string(v : T_Result) : String = v.toString();
  
  val v_assert = f_assert _;
  def f_assert(v__88 : T_Result) : Unit = {};
  
  val v__op_AC = f__op_AC _;
  def f__op_AC(v_l : T_ElemType*):T_Result = v_l.toList;
  
  val p__op_AC = new PatternSeqFunction[T_ElemType,T_Result](u__op_AC);
  def u__op_AC(x:T_Result) : Seq[T_ElemType] = x;
  
  val v_member = f_member _;
  def f_member(v_e : T_ElemType, v_l : T_Result):T_Boolean =
    v_l.contains(v_e);
  
  val v_append = f_append _;
  def f_append(v_l1 : T_Result, v_l2 : T_Result):T_Result = v_l1 ++ v_l2;
  def u_append(x:T_Result) : Option[(T_Result,T_Result)] = x match {
    case x::y::l => Some((List(x),y::l))
    case _ => None };
  val p_append = new PatternFunction[(T_Result,T_Result),T_Result](u_append);
  
  val v_single = f_single _;
  def f_single(v_x : T_ElemType):T_Result = List(v_x);
  def u_single(x:T_Result) : Option[(T_ElemType)] = x match {
    case v_x::Nil => Some((v_x));
    case _ => None };
  val p_single = new PatternFunction[(T_ElemType),T_Result](u_single);
  
  val v_none = f_none _;
  def f_none():T_Result = Nil;
  def u_none(x:T_Result) : Option[Unit] = x match {
    case Nil => Some(());
    case _ => None };
  val p_none = new Pattern0Function[T_Result](u_none);
  
  var v_initial:T_Result = v_none();
  val v_combine = f_combine _;
  def f_combine(v_l1 : T_Result, v_l2 : T_Result):T_Result = v_append(v_l1,v_l2);
}

class M_BAG[T_ElemType](t_ElemType:C_BASIC[T_ElemType]) extends Module("BAG") {
  type T_Result = List[T_ElemType];
  object t_Result extends I_BAG[T_ElemType] {
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

trait C_CONCATENATING[T_Result] {
  val v_concatenate : (T_Result,T_Result) => T_Result;
}

class M__basic_18[T_T](t_T:C_CONCATENATING[T_T]) {
  val v__op_ss : (T_T,T_T) => T_T = t_T.v_concatenate;
};

trait C_LIST[T_Result, T_ElemType] extends C_BASIC[T_Result] with C_CONCATENATING[T_Result] with C_ORDERED_COLLECTION[T_Result,T_ElemType] {
  val v_cons : (T_ElemType,T_Result) => T_Result;
  val p_single : PatternFunction[(T_ElemType),T_Result];
  val v_single : (T_ElemType) => T_Result;
  val p_append : PatternFunction[(T_Result,T_Result),T_Result];
  val v_append : (T_Result,T_Result) => T_Result;
  val p_none : Pattern0Function[T_Result];
  val v_none : () => T_Result;
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_concatenate : (T_Result,T_Result) => T_Result;
  val v_member : (T_ElemType,T_Result) => T_Boolean;
  val v_nth : (T_Integer,T_Result) => T_ElemType;
  val v_nth_from_end : (T_Integer,T_Result) => T_ElemType;
  val v_position : (T_ElemType,T_Result) => T_Integer;
  val v_position_from_end : (T_ElemType,T_Result) => T_Integer;
  val v_subseq : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_subseq_from_end : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_butsubseq : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_butsubseq_from_end : (T_Result,T_Integer,T_Integer) => T_Result;
  val v__op_AC : (T_ElemType*) => T_Result;
  val p__op_AC : PatternFunction[(T_ElemType),T_Result];
}

class I_LIST[E] extends I_BAG[E] with C_LIST[List[E],E] {
  type T_ElemType = E;
    val v_concatenate : (T_Result,T_Result) => T_Result = v_append;
    val v_nth = f_nth _;
    def f_nth(v_i : T_Integer, v_l : T_Result):T_ElemType = v_l(v_i);
    val v_nth_from_end = f_nth_from_end _;
    def f_nth_from_end(v_i : T_Integer, v_l : T_Result):T_ElemType =
      v_l.reverse.apply(v_i);
    val v_position = f_position _;
    def f_position(v_x : T_ElemType, v_l : T_Result):T_Integer =
      v_l.indexOf(v_l);
    val v_position_from_end = f_position_from_end _;
    def f_position_from_end(v_x : T_ElemType, v_l : T_Result):T_Integer =
      v_l.reverse.indexOf(v_l);
    val v_subseq = f_subseq _;
    def f_subseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
      v_l.slice(v_start,v_finish);
    val v_subseq_from_end = f_subseq_from_end _;
    def f_subseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
      throw new UnimplementedException("subseq_from_end");
    val v_butsubseq = f_butsubseq _;
    def f_butsubseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
      v_l.slice(0,v_start) ++ v_l.drop(v_finish);
    val v_butsubseq_from_end = f_butsubseq_from_end _;
    def f_butsubseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
      throw new UnimplementedException("but_subseq_from_end");
}

class M_LIST[T_ElemType](t_ElemType:C_BASIC[T_ElemType]) extends Module("LIST") {
  type T_Result = List[T_ElemType];
  object t_Result extends I_LIST[T_ElemType] {
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

trait C_ABSTRACT_SET[T_Result, T_ElemType] {
  val v_member : (T_ElemType,T_Result) => T_Boolean;
  val v_union : (T_Result,T_Result) => T_Result;
  val v_intersect : (T_Result,T_Result) => T_Result;
  val v_difference : (T_Result,T_Result) => T_Result;
}

class M__basic_19[T_E,T_T](t_E:Any,t_T:C_ABSTRACT_SET[T_T,T_E]) {
  val v__op_5w : (T_T,T_T) => T_T = t_T.v_union;
  val v__op_w5 : (T_T,T_T) => T_T = t_T.v_intersect;
  val v__op_w5D : (T_T,T_T) => T_T = t_T.v_difference;
};

class M__basic_20[T_E,T_T](t_E:Any,t_T:C_ABSTRACT_SET[T_T,T_E] with C_COLLECTION[T_T,T_E]) {
  val v__op_5 = f__op_5 _;
  def f__op_5(v_x : T_T, v_elem : T_E):T_T = t_T.v_difference(v_x,t_T.v_single(v_elem));
};

trait C_SET[T_Result, T_ElemType] extends C_BASIC[T_Result] with C_COMPARABLE[T_Result] with C_COLLECTION[T_Result,T_ElemType] with C_ABSTRACT_SET[T_Result,T_ElemType] with C_COMBINABLE[T_Result] {
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
  val v_none : () => T_Result;
  val v_single : (T_ElemType) => T_Result;
  val v_append : (T_Result,T_Result) => T_Result;
  val v__op_AC : (T_ElemType*) => T_Result;
  val p__op_AC : PatternFunction[(T_ElemType),T_Result];
  val v_member : (T_ElemType,T_Result) => T_Boolean;
  val v_union : (T_Result,T_Result) => T_Result;
  val v_intersect : (T_Result,T_Result) => T_Result;
  val v_difference : (T_Result,T_Result) => T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
}

class M_SET[T_ElemType](t_ElemType:C_BASIC[T_ElemType]) extends Module("SET") {
  import scala.collection.immutable.ListSet;
  type T_Result = ListSet[T_ElemType];
  object t_Result extends C_SET[T_Result,T_ElemType] {
    val v_equal = f_equal _;
    def f_equal(x : T_Result, y : T_Result) = x == y;
    
    val v_node_equivalent = f_equal _;
    
    val v_string = f_string _;
    def f_string(v : T_Result) : String = v.toString();
    
    val v_assert = f_assert _;
    def f_assert(v__88 : T_Result) : Unit = {};
    
    val v__op_AC = f__op_AC _;
    def f__op_AC(v_l : T_ElemType*):T_Result = ListSet(v_l:_*);
    
    val p__op_AC = new PatternSeqFunction[T_ElemType,T_Result](u__op_AC);
    def u__op_AC(x:T_Result) : Seq[T_ElemType] = x.toSeq;
    
    val v_member = f_member _;
    def f_member(v_e : T_ElemType, v_l : T_Result):T_Boolean =
      v_l.contains(v_e);
    
    val v_append = f_append _;
    def f_append(v_l1 : T_Result, v_l2 : T_Result):T_Result =
      ListSet(v_l1 ++ v_l2 : _*);
    def u_append(x:T_Result) : Option[(T_Result,T_Result)] =
      if (x.size > 1) {
	val y : T_ElemType = x.head;
	Some((ListSet(y),x - y))
      } else {
	None
      };
    val p_append = new PatternFunction[(T_Result,T_Result),T_Result](u_append);
    
    val v_single = f_single _;
    def f_single(v_x : T_ElemType):T_Result = ListSet(v_x);
    def u_single(x:T_Result) : Option[(T_ElemType)] =
      if (x.size == 1) Some(x.head) else None;
    val p_single = new PatternFunction[(T_ElemType),T_Result](u_single);
    
    val v_none = f_none _;
    def f_none():T_Result = Nil;
    def u_none(x:T_Result) : Option[Unit] =
      if (x.size == 0) Some(()) else None;
    val p_none = new Pattern0Function[T_Result](u_none);

    val v_initial = v_none;

    val v_less = f_less _;
    def f_less(v__99 : T_Result, v__100 : T_Result):T_Boolean =
      v__99.subsetOf(v_100) && v_99 != v_100;
    val v_less_equal = f_less_equal _;
    def f_less_equal(v__101 : T_Result, v__102 : T_Result):T_Boolean =
      v__99.subsetOf(v_100);

    val v_union = f_union _;
    def f_union(v__106 : T_Result, v__107 : T_Result):T_Result =
      v__106 ++ v__107;
    val v_intersect = f_intersect _;
    def f_intersect(v__108 : T_Result, v__109 : T_Result):T_Result =
      v__106 & v__107;
    val v_difference = f_difference _;
    def f_difference(v__110 : T_Result, v__111 : T_Result):T_Result =
      v__106 - v__107;
    val v_combine = f_combine _;
    def f_combine(v_x : T_Result, v_y : T_Result):T_Result = v_union(v_x,v_y);
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

/*
trait C_MULTISET[T_Result, T_ElemType] extends C_BASIC[T_Result] with C_COMPARABLE[T_Result] with C_COLLECTION[T_Result,T_ElemType] with C_ABSTRACT_SET[T_Result,T_ElemType] with C_COMBINABLE[T_Result] {
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
  val v__op_AC : (T_ElemType*) => T_Result;
  val p__op_AC : PatternFunction[(T_ElemType),T_Result];
  val v_member : (T_ElemType,T_Result) => T_Boolean;
  val v_count : (T_ElemType,T_Result) => T_Integer;
  val v_union : (T_Result,T_Result) => T_Result;
  val v_intersect : (T_Result,T_Result) => T_Result;
  val v_difference : (T_Result,T_Result) => T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
}

class M_MULTISET[T_ElemType](t_ElemType:C_BASIC[T_ElemType] extends Module("MULTISET") {
  val m_tmp = new M_BAG[T_ElemType](t_ElemType);
  type T_tmp = m_tmp.T_Result;
  val t_tmp = m_tmp.t_Result;

  type T_Result = T_tmp;
  object t_Result extends C_MULTISET[T_Result,T_ElemType] {
    val v_assert = t_tmp.v_assert;
    val p_append = t_tmp.p_append;
    val v_append = t_tmp.v_append;
    val p_single = t_tmp.p_single;
    val v_single = t_tmp.v_single;
    val p_none = t_tmp.p_none;
    val v_none = t_tmp.v_none;
    val v_initial = t_tmp.v_initial;

    val v_equal = f_equal _;
    def f_equal(v__112 : T_Result, v__113 : T_Result):T_Boolean;
    val v_less = f_less _;
    def f_less(v__114 : T_Result, v__115 : T_Result):T_Boolean;
    val v_less_equal = f_less_equal _;
    def f_less_equal(v__116 : T_Result, v__117 : T_Result):T_Boolean;
    val v__op_AC = f__op_AC _;
    def f__op_AC(v__118 : T_ElemType*):T_Result;
    def u__op_AC(x:T_Result) :  : Option[T_ElemType)] = x match {
      case _ => None }
;      val p__op_AC = new PatternFunction[T_ElemType),T_Result](u__op_AC);
    val v_member = f_member _;
    def f_member(v_x : T_ElemType, v_l : T_Result):T_Boolean;
    val v_count = f_count _;
    def f_count(v_x : T_ElemType, v_l : T_Result):T_Integer;
    val v_union = f_union _;
    def f_union(v__120 : T_Result, v__121 : T_Result):T_Result;
    val v_intersect = f_intersect _;
    def f_intersect(v__122 : T_Result, v__123 : T_Result):T_Result;
    val v_difference = f_difference _;
    def f_difference(v__124 : T_Result, v__125 : T_Result):T_Result;
    val v_combine = f_combine _;
    def f_combine(v_x : T_Result, v_y : T_Result):T_Result = v_union(v_x,v_y);
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}
*/

/*
trait C_ORDERED_SET[T_Result, T_ElemType] 
extends C_ORDERED_COLLECTION[T_Result,T_ElemType] 
with C_SET[T_Result,T_ElemType] 
{
}

class M_ORDERED_SET[T_ElemType](t_ElemType:C_ORDERED[T_ElemType] extends Module("ORDERED_SET") {
  val m_tmp = new M_SET[T_ElemType](t_ElemType);
  type T_tmp = m_tmp.T_Result;
  val t_tmp = m_tmp.t_Result;

  type T_Result = T_tmp;
  object t_Result extends C_ORDERED_SET[T_Result,T_ElemType] {
    val v_none = t_tmp.v_none;
    val v_single = t_tmp.v_single;
    val v_append = t_tmp.v_append;
    val p_{} = t_tmp.p_{};
    val v_member = t_tmp.v_member;

    val v_equal = f_equal _;
    def f_equal(v__126 : T_Result, v__127 : T_Result):T_Boolean;
    val v_less = f_less _;
    def f_less(v__128 : T_Result, v__129 : T_Result):T_Boolean;
    val v_less_equal = f_less_equal _;
    def f_less_equal(v__130 : T_Result, v__131 : T_Result):T_Boolean;
    val v__op_AC = f__op_AC _;
    def f__op_AC(v__132 : T_ElemType*):T_Result;
    val v_union = f_union _;
    def f_union(v__133 : T_Result, v__134 : T_Result):T_Result;
    val v_intersect = f_intersect _;
    def f_intersect(v__135 : T_Result, v__136 : T_Result):T_Result;
    val v_difference = f_difference _;
    def f_difference(v__137 : T_Result, v__138 : T_Result):T_Result;
    val v_combine = f_combine _;
    def f_combine(v_x : T_Result, v_y : T_Result):T_Result = v_union(v_x,v_y);
    val v_nth = f_nth _;
    def f_nth(v_i : T_Integer, v_l : T_Result):T_ElemType;
    val v_nth_from_end = f_nth_from_end _;
    def f_nth_from_end(v_i : T_Integer, v_l : T_Result):T_ElemType;
    val v_position = f_position _;
    def f_position(v_x : T_ElemType, v_l : T_Result):T_Integer;
    val v_position_from_end = f_position_from_end _;
    def f_position_from_end(v_x : T_ElemType, v_l : T_Result):T_Integer;
    val v_subseq = f_subseq _;
    def f_subseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result;
    val v_subseq_from_end = f_subseq_from_end _;
    def f_subseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result;
    val v_butsubseq = f_butsubseq _;
    def f_butsubseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result;
    val v_butsubseq_from_end = f_butsubseq_from_end _;
    def f_butsubseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result;
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

trait C_ORDERED_MULTISET[T_Result, T_ElemType] extends C_ORDERED_COLLECTION[T_Result,T_ElemType] {
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
  val v__op_AC : (T_ElemType*) => T_Result;
  val v_union : (T_Result,T_Result) => T_Result;
  val v_intersect : (T_Result,T_Result) => T_Result;
  val v_difference : (T_Result,T_Result) => T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
  val v_nth : (T_Integer,T_Result) => T_ElemType;
  val v_nth_from_end : (T_Integer,T_Result) => T_ElemType;
  val v_position : (T_ElemType,T_Result) => T_Integer;
  val v_position_from_end : (T_ElemType,T_Result) => T_Integer;
  val v_subseq : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_subseq_from_end : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_butsubseq : (T_Result,T_Integer,T_Integer) => T_Result;
  val v_butsubseq_from_end : (T_Result,T_Integer,T_Integer) => T_Result;
}

class M_ORDERED_MULTISET[T_ElemType](t_ElemType:C_ORDERED[T_ElemType] extends Module("ORDERED_MULTISET") {
  val m_tmp = new M_MULTISET[T_ElemType](t_ElemType);
  type T_tmp = m_tmp.T_Result;
  val t_tmp = m_tmp.t_Result;

  type T_Result = T_tmp;
  object t_Result extends C_ORDERED_MULTISET[T_Result,T_ElemType] {
    val p_{} = t_tmp.p_{};
    val v_member = t_tmp.v_member;
    val v_count = t_tmp.v_count;

    val v_equal = f_equal _;
    def f_equal(v__139 : T_Result, v__140 : T_Result):T_Boolean;
    val v_less = f_less _;
    def f_less(v__141 : T_Result, v__142 : T_Result):T_Boolean;
    val v_less_equal = f_less_equal _;
    def f_less_equal(v__143 : T_Result, v__144 : T_Result):T_Boolean;
    val v__op_AC = f__op_AC _;
    def f__op_AC(v__145 : T_ElemType*):T_Result;
    val v_union = f_union _;
    def f_union(v__146 : T_Result, v__147 : T_Result):T_Result;
    val v_intersect = f_intersect _;
    def f_intersect(v__148 : T_Result, v__149 : T_Result):T_Result;
    val v_difference = f_difference _;
    def f_difference(v__150 : T_Result, v__151 : T_Result):T_Result;
    val v_combine = f_combine _;
    def f_combine(v_x : T_Result, v_y : T_Result):T_Result = v_union(v_x,v_y);
    val v_nth = f_nth _;
    def f_nth(v_i : T_Integer, v_l : T_Result):T_ElemType;
    val v_nth_from_end = f_nth_from_end _;
    def f_nth_from_end(v_i : T_Integer, v_l : T_Result):T_ElemType;
    val v_position = f_position _;
    def f_position(v_x : T_ElemType, v_l : T_Result):T_Integer;
    val v_position_from_end = f_position_from_end _;
    def f_position_from_end(v_x : T_ElemType, v_l : T_Result):T_Integer;
    val v_subseq = f_subseq _;
    def f_subseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result;
    val v_subseq_from_end = f_subseq_from_end _;
    def f_subseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result;
    val v_butsubseq = f_butsubseq _;
    def f_butsubseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result;
    val v_butsubseq_from_end = f_butsubseq_from_end _;
    def f_butsubseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result;
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}
*/

trait C_UNION_LATTICE[T_Result, T_ElemType, T_ST] 
extends C_MAKE_LATTICE[T_Result] {
}

class M_UNION_LATTICE[T_ElemType, T_ST](t_ElemType:Any,t_ST:C_SET[T_ST,T_ElemType]) extends Module("UNION_LATTICE") {
  val m_tmp = new M_MAKE_LATTICE[T_ST](t_ST,t_ST.v_none(),M__basic_3[ T_ST](t_ST).v__op_z,M__basic_3[ T_ST](t_ST).v__op_z0,M__basic_19[ T_ElemType,T_ST](t_ElemType,t_ST).v__op_5w,M__basic_19[ T_ElemType,T_ST](t_ElemType,t_ST).v__op_w5);
  type T_tmp = m_tmp.T_Result;
  val t_tmp = m_tmp.t_Result;

  type T_UnionLattice = T_tmp;
  object t_UnionLattice extends C_UNION_LATTICE[T_UnionLattice,T_ElemType,T_ST] {
    val v_initial = t_tmp.v_initial;
    val v_combine = t_tmp.v_combine;
    val v_bottom = t_tmp.v_bottom;
    val v_compare = t_tmp.v_compare;
    val v_compare_equal = t_tmp.v_compare_equal;
    val v_join = t_tmp.v_join;
    val v_meet = t_tmp.v_meet;

    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_UnionLattice.finish();
    super.finish();
  }
}

trait C_INTERSECTION_LATTICE[T_Result, T_ElemType, T_ST]
extends C_MAKE_LATTICE[T_RESULT]
{
}

class M_INTERSECTION_LATTICE[T_ElemType, T_ST](t_ElemType:Any,t_ST:C_SET[T_ST,T_ElemType],v_universe : T_ST) extends Module("INTERSECTION_LATTICE") {
  val m_tmp = new M_MAKE_LATTICE[T_ST](t_ST,v_universe,M__basic_3[ T_ST](t_ST).v__op_1,M__basic_3[ T_ST](t_ST).v__op_10,M__basic_19[ T_ElemType,T_ST](t_ElemType,t_ST).v__op_w5,M__basic_19[ T_ElemType,T_ST](t_ElemType,t_ST).v__op_5w);
  type T_tmp = m_tmp.T_Result;
  val t_tmp = m_tmp.t_Result;

  type T_IntersectionLattice = T_tmp;
  object t_IntersectionLattice extends C_INTERSECTION_LATTICE[T_IntersectionLattice,T_ElemType,T_ST] {
    val v_initial = t_tmp.v_initial;
    val v_combine = t_tmp.v_combine;
    val v_bottom = t_tmp.v_bottom;
    val v_compare = t_tmp.v_compare;
    val v_compare_equal = t_tmp.v_compare_equal;
    val v_join = t_tmp.v_join;
    val v_meet = t_tmp.v_meet;

    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_IntersectionLattice.finish();
    super.finish();
  }
}

trait C_PAIR[T_Result, T_T1, T_T2] {
  val p_pair : PatternFunction[(T_T1,T_T2),T_Result];
  val v_pair : (T_T1,T_T2) => T_Result;
  val v_fst : (T_Result) => T_T1;
  val v_snd : (T_Result) => T_T2;
}

class M_PAIR[T_T1, T_T2](t_T1:C_BASIC[T_T1],t_T2:C_BASIC[T_T2]) extends Module("PAIR") {
  class T_tmp extends Type {
    def getType() = t_tmp;
  }
  object t_tmp extends I_TYPE[T_tmp] {}
  type T_Result = T_tmp;
  object t_Result extends C_PAIR[T_Result,T_T1,T_T2] {
    val v_assert = t_tmp.v_assert;
    val v_equal = t_tmp.v_equal;
    val v_node_equivalent = t_tmp.v_node_equivalent;
    val v_string = t_tmp.v_string;

    case class c_pair(v_x : T_T1,v_y : T_T2) extends T_Result {
      def children : List[Phylum] = List();
    }
    val v_pair = f_pair _;
    def f_pair(v_x : T_T1, v_y : T_T2):T_Result = c_pair(v_x,v_y);
    def u_pair(x:T_Result) : Option[(T_T1,T_T2)] = x match {
      case c_pair(v_x,v_y) => Some((v_x,v_y));
      case _ => None };
    val p_pair = new PatternFunction[(T_T1,T_T2),T_Result](u_pair);

    val v_fst = f_fst _;
    def f_fst(v_p : T_Result):T_T1 =
      v_p match {
        case p_pair(v_x,_) => v_x
	case _ => throw UndefinedAttributeException("local fst");
      };

    val v_snd = f_snd _;
    def f_snd(v_p : T_Result):T_T2 =
      v_p match {
	case p_pair(_,v_y) => return v_y
	case _ => throw UndefinedAttributeException("local snd")
      };

    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

trait C_STRING[T_Result] extends C_ORDERED[T_Result] with C_PRINTABLE[T_Result] {
}

class M_STRING extends Module("STRING") {
  // import scala.collection.immutable.WrappedString;
  import scala.runtime.RichString;
  type T_Result = String;
  object t_Result extends C_STRING[T_Result] {
    val v_equal = f_equal _;
    def f_equal(x : T_Result, y : T_result) = x == y;
    
    val v_node_equivalent = f_equal _;
    
    val v_string = f_string _;
    def f_string(v : T_Result) : String = v;
    
    val v_assert = f_assert _;
    def f_assert(v__88 : T_Result) : Unit = {};
    
    val v__op_AC = f__op_AC _;
    def f__op_AC(v_l : Char*):T_Result = "" ++ v_l;
    
    val p__op_AC = new PatternSeqFunction[Char,T_Result](u__op_AC);
    def u__op_AC(x:T_Result) : Seq[Char] = new RichString(v_l);
    
    val v_member = f_member _;
    def f_member(v_e : Char, v_l : T_Result):T_Boolean =
      RichString(v_l).contains(v_e);
    
    val v_append = f_append _;
    def f_append(v_l1 : T_Result, v_l2 : T_Result):T_Result = v_l1 ++ v_l2;
    def u_append(x:T_Result) : Option[(T_Result,T_Result)] =
      if (x.length() > 1) Some(x.substr(0,1),x.substr(1))
      else None;
    val p_append = new PatternFunction[(T_Result,T_Result),T_Result](u_append);
    
    val v_single = f_single _;
    def f_single(v_x : Char):T_Result = "" + v_x;
    def u_single(x:T_Result) : Option[(Char)] = 
      if (x.length() == 1) Some(x.charAt(0))
      else None;
    val p_single = new PatternFunction[(Char),T_Result](u_single);
    
    val v_none = f_none _;
    def f_none():T_Result = Nil;
    def u_none(x:T_Result) : Option[Unit] =
      if (x.length() == 0) Some(()) else None;
    val p_none = new Pattern0Function[T_Result](u_none);

    val v_concatenate : (T_Result,T_Result) => T_Result = v_append;
    val v_nth = f_nth _;
    def f_nth(v_i : T_Integer, v_l : T_Result):Char = v_l.charAt(v_i);
    val v_nth_from_end = f_nth_from_end _;
    def f_nth_from_end(v_i : T_Integer, v_l : T_Result):Char =
      v_l.charAt(v_l.length() - 1 - v_i);
    val v_position = f_position _;
    def f_position(v_x : Char, v_l : T_Result):T_Integer =
      v_l.indexOf(v_l);
    val v_position_from_end = f_position_from_end _;
    def f_position_from_end(v_x : Char, v_l : T_Result):T_Integer =
      v_l.reverse.indexOf(v_l);
    val v_subseq = f_subseq _;
    def f_subseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
      v_l.substr(v_start,v_finish);
    val v_subseq_from_end = f_subseq_from_end _;
    def f_subseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
      throw new UnimplementedException("subseq_from_end");
    val v_butsubseq = f_butsubseq _;
    def f_butsubseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
      v_l.substr(0,v_start) ++ v_l.substr(v_finish);
    val v_butsubseq_from_end = f_butsubseq_from_end _;
    def f_butsubseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
      throw new UnimplementedException("but_subseq_from_end");

    val v_less = f_less _;
    def f_less(v_x : T_Result, v_y : T_Result):T_Boolean =
      v_x.compareTo(v_y) < 0;
    val v_less_equal = f_less_equal _;
    def f_less_equal(v_x : T_Result, v_y : T_Result):T_Boolean =
      v_x.compareTo(v_y) <= 0;
    val v_string = f_string _;
    def f_string(v_x : T_Result):T_String = v_x;
    def finish() : Unit = {
    }

  }

  override def finish() : Unit = {
    t_Result.finish();
    super.finish();
  }
}

object implicit_34 {
  val m_String = new M_STRING;
  type T_String = m_String.T_Result;
  val t_String = m_String.t_Result;

}

class M__basic_21[T_T,T_U](t_T:C_PRINTABLE[T_T],t_U:C_PRINTABLE[T_U]) {
  val v__op_BB = f__op_BB _;
  def f__op_BB(v_x : T_T, v_y : T_U):T_String = new M__basic_18[ T_String](t_String).v__op_ss(t_T.v_string(v_x),t_U.v_string(v_y));
};

object implicit_35 {
  val m_Range = new M_LIST[T_Integer](t_Integer);
  type T_Range = m_Range.T_Result;
  val t_Range = m_Range.t_Result;

  val v__op_vv = f__op_vv _;
  def f__op_vv(v_x : T_Integer, v_y : T_Integer):T_Range = {

    { val cond = new M__basic_2[ T_Integer](t_Integer).v__op_0(v_x,v_y);
      if (cond) {
        return t_Range.v_single(v_x);
      }
      if (!cond) {
        { val cond = new M__basic_3[ T_Integer](t_Integer).v__op_1(v_x,v_y);
          if (cond) {
            return t_Range.v_none();
          }
          if (!cond) {
            var v_mid : T_Integer = new M__basic_4[ T_Integer](t_Integer).v__op_w(M__basic_4[ T_Integer](t_Integer).v__op_s(v_x,v_y),2);
            return M__basic_18[ T_Range](t_Range).v__op_ss(v__op_vv(v_x,v_mid),v__op_vv(M__basic_4[ T_Integer](t_Integer).v__op_s(v_mid,1),v_y));
          }
        }
      }
    }
    throw UndefinedAttributeException("local ..");
  }

}
import implicit_35._;

class M__basic_22[T_ElemType,T_T](t_ElemType:Any,t_T:C_READ_ONLY_COLLECTION[T_T,T_ElemType]) {
  val v_length = f_length _;
  def f_length(v_l : T_T):T_Integer = new M__basic_4[ T_Integer](t_Integer).v__op_s(0,);
};

class M__basic_23[T_T](t_T:Any) {
  val v_debug_output = f_debug_output _;
  def f_debug_output(v_x : T_T):T_String = v_x.toString();
};

class M__basic_24[T_Node](t_Node:C_PHYLUM[T_Node]) {
  val v_lineno = f_lineno _;
  def f_lineno(v_x : T_Node):T_Integer =
    v_x.asInstanceOf[Phylum].lineNumber;
};

