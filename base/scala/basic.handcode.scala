import Evaluation._;

object basic_implicit {
  val t_Boolean = new M_BOOLEAN("Boolean");
  type T_Boolean = Boolean;
  val v_true:T_Boolean = true;
  val v_false:T_Boolean = false;
  val v_and = f_and _;
  def f_and(v__23 : T_Boolean, v__24 : T_Boolean):T_Boolean = v__23 && v__24;
  val v_or = f_or _;
  def f_or(v__25 : T_Boolean, v__26 : T_Boolean):T_Boolean = v__25 || v__26;
  val v_not = f_not _;
  def f_not(v__27 : T_Boolean):T_Boolean = !v__27;

  val t_Integer = new M_INTEGER("Integer");
  type T_Integer = Int;
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

  val t_Character = new M_CHARACTER("Character");
  type T_Character = Char;
  val v_char_code = f_char_code _;
  def f_char_code(v_x : T_Character):T_Integer = v_x;
  val v_int_char = f_int_char _;
  def f_int_char(v_x : T_Integer):T_Character = v_x.asInstanceOf[Char];
  val v_tab:T_Character = v_int_char(9);
  val v_newline:T_Character = v_int_char(10);

  val v_cand = f_cand _;
  def f_cand(v_x : T_Boolean, v_y : T_Boolean):T_Boolean = v_and(v_not(v_x),v_y);
  val v_implies = f_implies _;
  def f_implies(v_x : T_Boolean, v_y : T_Boolean):T_Boolean = v_or(v_not(v_x),v_y);
  val v_andc = f_andc _;
  def f_andc(v_x : T_Boolean, v_y : T_Boolean):T_Boolean = v_and(v_x,v_not(v_y));
  val v_revimplies = f_revimplies _;
  def f_revimplies(v_x : T_Boolean, v_y : T_Boolean):T_Boolean = v_or(v_x,v_not(v_y));

  type T_MAKE_LATTICE[L] = L;

  val t_OrLattice = new M_MAKE_LATTICE[T_Boolean]("OrLattice",t_Boolean,v_false,v_cand,v_implies,v_or,v_and)
    with C_TYPE[Boolean]
    with C_COMBINABLE[Boolean]
    with C_LATTICE[Boolean] {
        override val v_assert = t_Boolean.v_assert
        override val v_node_equivalent = t_Boolean.v_node_equivalent
        override val v_string = t_Boolean.v_string
  }
  type T_OrLattice = T_Boolean;

  val t_AndLattice = new M_MAKE_LATTICE[T_Boolean]("AndLattice",t_Boolean,v_true,v_andc,v_revimplies,v_and,v_or)
    with C_TYPE[Boolean]
    with C_COMBINABLE[Boolean]
    with C_LATTICE[Boolean] {
       override val v_assert = t_Boolean.v_assert
       override val v_node_equivalent = t_Boolean.v_node_equivalent
       override val v_string = t_Boolean.v_string
  }
  type T_AndLattice = T_Boolean;

  type T_MAX_LATTICE[T] = T;
  type T_MIN_LATTICE[T] = T;


  type T_BAG[T] = List[T];
  type T_LIST[T] = List[T];
  type T_SET[T] = scala.collection.immutable.Set[T];
  type T_MULTISET[T] = T_BAG[T];

  type T_UNION_LATTICE[T_E,T_T] = T_T;
  type T_INTERSECTION_LATTICE[T_E,T_T] = T_T;
  
  val t_String = new M_STRING("String");
  type T_String = String;

  val t_Range = new M_LIST[T_Integer]("Range",t_Integer);
  type T_Range = t_Range.T_Result;

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
            var v_mid : T_Integer = new M__basic_4[ T_Integer](t_Integer).v__op_w(new M__basic_4[ T_Integer](t_Integer).v__op_s(v_x,v_y),2);
            return new M__basic_18[ T_Range](t_Range).v__op_ss(v__op_vv(v_x,v_mid),v__op_vv(new M__basic_4[ T_Integer](t_Integer).v__op_s(v_mid,1),v_y));
          }
        }
      }
    }
    throw UndefinedAttributeException("local ..");
  }

}
import basic_implicit._;

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

trait C_PRINTABLE[T_Result] extends C_NULL[T_Result] {
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
  def v_zero : T_Result;
  def v_one : T_Result;
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

trait C_BOOLEAN[T_Result] extends C_TYPE[T_Result] {}

class M_BOOLEAN(name : String) extends I_TYPE[Boolean](name) {}

trait C_INTEGER[T_Result] extends C_TYPE[T_Result] with C_NUMERIC[T_Result] with C_ORDERED[T_Result] with C_PRINTABLE[T_Result] {}

class M_INTEGER(name : String) extends I_TYPE[Int](name) with C_INTEGER[Int]
{
  override def f_assert(v__28 : T_Result) : Unit = {};
  val v_zero:T_Result = 0;
  val v_one:T_Result = 1;
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
}


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

/*
class M_IEEE extends Module("IEEE") {
  class T_tmp extends Value {
    def getType = t_tmp;
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

class M_CHARACTER(name : String) extends I_TYPE[Char](name) {
  override def f_assert(v__33 : T_Result) : Unit = {}
  val v_less = f_less _;
  def f_less(v_x : T_Result, v_y : T_Result):T_Boolean = v_x < v_y;
  val v_less_equal = f_less_equal _;
  def f_less_equal(v_x : T_Result, v_y : T_Result):T_Boolean = v_x <= v_y;
}

class M__basic_8[T_T <: Node](t_T:C_PHYLUM[T_T]) {
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
  def v_initial : T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
}

trait C_COMPLETE_PARTIAL_ORDER[T_Result] extends C_BASIC[T_Result] {
  def v_bottom : T_Result;
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

class M_MAKE_LATTICE[T_L](name : String, t_L:C_BASIC[T_L],v_default : T_L,v_comparef : (T_L,T_L) => T_Boolean,v_compare_equalf : (T_L,T_L) => T_Boolean,v_joinf : (T_L,T_L) => T_L,v_meetf : (T_L,T_L) => T_L) 
extends Module(name) with C_MAKE_LATTICE[T_L,T_L]
{
  val v_equal = t_L.v_equal;
  val v_initial : T_L = v_default;
  val v_combine : (T_L,T_L) => T_L = v_joinf;
  val v_bottom : T_L = v_default;
  val v_compare : (T_L,T_L) => T_Boolean = v_comparef;
  val v_compare_equal : (T_L,T_L) => T_Boolean = v_compare_equalf;
  val v_join : (T_L,T_L) => T_L = v_joinf;
  val v_meet : (T_L,T_L) => T_L = v_meetf;
}

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

trait C_MAX_LATTICE[T_Result, T_TO] extends
 C_MAKE_LATTICE[T_Result,T_TO] // with C_ORDERED[T_TO] 
{
}

class M_MAX_LATTICE[T_TO]
      (name : String, t_TO:C_ORDERED[T_TO],v_min_element : T_TO) 
      extends M_MAKE_LATTICE[T_TO](name,t_TO,v_min_element,
				   new M__basic_3[ T_TO](t_TO).v__op_z,
				   new M__basic_3[ T_TO](t_TO).v__op_z0,
				   new M__basic_13[ T_TO](t_TO).v_max,
				   new M__basic_13[ T_TO](t_TO).v_min)
      with C_MAX_LATTICE[T_TO,T_TO] with C_ORDERED[T_TO]
{
  val v_less = t_TO.v_less;
  val v_less_equal = t_TO.v_less_equal;
}

trait C_MIN_LATTICE[T_Result, T_T] extends
C_MAKE_LATTICE[T_Result,T_T] // with C_ORDERED[T_T]
{
}

class M_MIN_LATTICE[T_T]
	(name : String, t_T:C_ORDERED[T_T],v_max_element : T_T) 
extends M_MAKE_LATTICE[T_T](name,t_T,v_max_element,
			    new M__basic_3[ T_T](t_T).v__op_1,
			    new M__basic_3[ T_T](t_T).v__op_10,
			    new M__basic_13[ T_T](t_T).v_min,
			    new M__basic_13[ T_T](t_T).v_max)
      with C_MIN_LATTICE[T_T,T_T] with C_ORDERED[T_T]
{
  val v_less = t_T.v_less;
  val v_less_equal = t_T.v_less_equal;
}

trait C_READ_ONLY_COLLECTION[T_Result, T_ElemType] {
  val p__op_AC : PatternSeqFunction[T_Result,T_ElemType];
  val p_append : PatternFunction[(T_Result,T_Result,T_Result)];
  val p_single : PatternFunction[(T_Result,T_ElemType)];
  val p_none : PatternFunction[T_Result];
  val v_member : (T_ElemType,T_Result) => T_Boolean;
}

trait C_COLLECTION[T_Result, T_ElemType] extends C_READ_ONLY_COLLECTION[T_Result,T_ElemType] {
  val v_append : (T_Result,T_Result) => T_Result;
  val v_single : (T_ElemType) => T_Result;
  val v_none : () => T_Result;
  val v__op_AC : (Seq[T_ElemType]) => T_Result;
}

class M__basic_14[T_ElemType,T_T](t_ElemType:Any,t_T:C_READ_ONLY_COLLECTION[T_T,T_ElemType]) {
  val p__op_AC = t_T.p__op_AC;
  val v_member : (T_ElemType,T_T) => T_Boolean = t_T.v_member;
  val v_in : (T_ElemType,T_T) => T_Boolean = t_T.v_member;
};

class M__basic_15[T_ElemType,T_T](t_ElemType:Any,t_T:C_COLLECTION[T_T,T_ElemType]) {
  val v__op_AC : (Seq[T_ElemType]) => T_T = t_T.v__op_AC;
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

trait C_SEQUENCE[T_Result <: Node, T_ElemType] extends C_READ_ONLY_ORDERED_COLLECTION[T_Result,T_ElemType] with C_PHYLUM[T_Result] {
  val v_assert : (T_Result) => Unit;
  val p__op_AC : PatternSeqFunction[T_Result,T_ElemType];
  val v_nth : (T_Integer,T_Result) => T_ElemType;
  val v_nth_from_end : (T_Integer,T_Result) => T_ElemType;
  val v_position : (T_ElemType,T_Result) => T_Integer;
  val v_position_from_end : (T_ElemType,T_Result) => T_Integer;
  val v_member : (T_ElemType,T_Result) => T_Boolean;
  val p_append : PatternFunction[(T_Result,T_Result,T_Result)];
  val v_append : (T_Result,T_Result) => T_Result;
  val p_single : PatternFunction[(T_Result,T_ElemType)];
  val v_single : (T_ElemType) => T_Result;
  val p_none : PatternFunction[T_Result];
  val v_none : () => T_Result;
}

abstract class T_SEQUENCE[T_ElemType <: Node]
(t_Result : C_SEQUENCE[T_SEQUENCE[T_ElemType],T_ElemType])
extends Node(t_Result)
{
  type T_Result = T_SEQUENCE[T_ElemType];
  def getType : C_PHYLUM[T_Result] = t_Result;
  // backward compatability to simple CS 654 level APS:
  def size() : Int = 0;
  def nth(i : Int) : T_ElemType = {
    throw new java.util.NoSuchElementException("no more elements: " + i);
  }
  def concat(l : T_Result) : T_Result = t_Result.v_append(this,l);
  def addcopy(x : T_ElemType) : T_Result = concat(t_Result.v_single(x));
}

class M_SEQUENCE[T_ElemType <: Node](name : String, t_ElemType:C_PHYLUM[T_ElemType]) 
extends I_PHYLUM[T_SEQUENCE[T_ElemType]](name) 
with C_SEQUENCE[T_SEQUENCE[T_ElemType],T_ElemType]
{
  // we make things easier (but inefficient) by using lists.
  val t_Result : this.type = this;
  def toList(x : T_Result) : List[T_ElemType] = x match {
    case c_append(x1,x2) => toList(x1) ++ toList(x2)
    case c_single(e) => List(e)
    case c_none() => List()
  };
  def u__op_AC(x:Any) : Option[(T_Result,Seq[T_ElemType])] = x match {
    case x : T_Result => Some((x,toList(x)));
    case _ => None
  };
  val p__op_AC = new PatternSeqFunction[T_Result,T_ElemType](u__op_AC);
  val v_nth = f_nth _;
  def f_nth(v_i : T_Integer, v_l : T_Result):T_ElemType = v_l.nth(v_i);
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
  case class c_append(v_l1 : T_Result,v_l2 : T_Result) extends T_Result(t_Result) {
    private val n1 : Int = v_l1.size();
    private val n2 : Int = v_l2.size();
    def children : List[Node] = List(v_l1,v_l2);
    override def size() : Int = n1 + n2;
    override def nth(i : Int) : T_ElemType =
      if (i < n1) v_l1.nth(i) else v_l2.nth(i-n1);
    override def toString() : String =
      Debug.with_level {
	"append(" + v_l1 + "," + v_l2 + ")"
      };
  }
  val v_append = f_append _;
  def f_append(v_l1 : T_Result, v_l2 : T_Result):T_Result = c_append(v_l1,v_l2).register;
  def u_append(x:Any) : Option[(T_Result,T_Result,T_Result)] = x match {
    case x@c_append(v_l1,v_l2) => Some((x,v_l1,v_l2));
    case _ => None };
  val p_append = new PatternFunction[(T_Result,T_Result,T_Result)](u_append);
  
  case class c_single(v_x : T_ElemType) extends T_Result(t_Result) {
    def children : List[Node] = List(v_x);
    override def size() : Int = 1;
    override def nth(i : Int) : T_ElemType = 
      if (i == 0) v_x else super.nth(i);
    override def toString() : String =
      Debug.with_level {
	"single(" + v_x + ")"
      };
  }
  val v_single = f_single _;
  def f_single(v_x : T_ElemType):T_Result = c_single(v_x).register;
  def u_single(x:Any) : Option[(T_Result,T_ElemType)] = x match {
    case x@c_single(v_x) => Some((x,v_x));
    case _ => None };
  val p_single = new PatternFunction[(T_Result,T_ElemType)](u_single);
  
  case class c_none() extends T_Result(t_Result) {
    def children : List[Node] = List();
    override def toString() : String =
      Debug.with_level {
	"none()"
      };
  }
  val v_none = f_none _;
  def f_none():T_Result = c_none().register;
  def u_none(x:Any) : Option[T_Result] = x match {
    case x@c_none() => Some((x));
    case _ => None };
  val p_none = new PatternFunction[T_Result](u_none);
  
}

trait C_BAG[T_Result, T_ElemType] extends C_TYPE[T_Result] with C_COLLECTION[T_Result,T_ElemType] with C_COMBINABLE[T_Result] {
  def v_initial : T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
}

class I_BAG[T_ElemType](name : String) 
extends Module(name)
with C_BAG[List[T_ElemType],T_ElemType] 
{
  type T_Result = List[T_ElemType];

  val v_equal = f_equal _;
  def f_equal(x : T_Result, y : T_Result) = x == y;
  
  val v_node_equivalent = f_equal _;
  
  val v_string = f_string _;
  def f_string(v : T_Result) : String = v.toString();
  
  val v_assert = f_assert _;
  def f_assert(v__88 : T_Result) : Unit = {};
  
  val v__op_AC = f__op_AC _;
  def f__op_AC(v_l : Seq[T_ElemType]):T_Result = v_l.toList;
  
  val p__op_AC = new PatternSeqFunction[T_Result,T_ElemType](u__op_AC);
  def u__op_AC(x:Any) : Option[(T_Result,Seq[T_ElemType])] = x match {
    case x:T_Result => Some((x,x))
    case _ => None
  };
  
  val v_member = f_member _;
  def f_member(v_e : T_ElemType, v_l : T_Result):T_Boolean =
    v_l.contains(v_e);
  
  val v_append = f_append _;
  def f_append(v_l1 : T_Result, v_l2 : T_Result):T_Result = v_l1 ++ v_l2;
  def u_append(x:Any) : Option[(T_Result,T_Result,T_Result)] = x match {
    case x:T_Result => x match {
      case x1::x2::l => Some((x,List(x1),x2::l))
      case _ => None 
    };
    case _ => None;
  }
  val p_append = new PatternFunction[(T_Result,T_Result,T_Result)](u_append);
  
  val v_single = f_single _;
  def f_single(v_x : T_ElemType):T_Result = List(v_x);
  def u_single(x:Any) : Option[(T_Result,T_ElemType)] = x match {
    case x:T_Result => x match {
      case v_x::Nil => Some((x,v_x));
      case _ => None };
    case _ => None };
  val p_single = new PatternFunction[(T_Result,T_ElemType)](u_single);
  
  val v_none = f_none _;
  def f_none():T_Result = Nil;
  def u_none(x:Any) : Option[T_Result] = x match {
    case x@Nil => Some((x));
    case _ => None };
  val p_none = new PatternFunction[T_Result](u_none);
  
  val v_initial:T_Result = v_none();
  val v_combine = f_combine _;
  def f_combine(v_l1 : T_Result, v_l2 : T_Result):T_Result = v_append(v_l1,v_l2);
}

class M_BAG[T_ElemType](name : String, t_ElemType:C_BASIC[T_ElemType]) 
extends I_BAG[T_ElemType](name)
{
}

trait C_CONCATENATING[T_Result] {
  val v_concatenate : (T_Result,T_Result) => T_Result;
}

class M__basic_18[T_T](t_T:C_CONCATENATING[T_T]) {
  val v__op_ss : (T_T,T_T) => T_T = t_T.v_concatenate;
};

trait C_LIST[T_Result, T_ElemType] extends C_TYPE[T_Result] with C_CONCATENATING[T_Result] with C_ORDERED_COLLECTION[T_Result,T_ElemType] {
  val v_cons : (T_ElemType,T_Result) => T_Result;
}

class I_LIST[E](name : String) extends I_BAG[E](name) with C_LIST[List[E],E] {
  type T_ElemType = E;
  val v_cons = f_cons _;
  def f_cons(x : E, l : List[E]) : List[E] = x::l;
  val v_concatenate : (T_Result,T_Result) => T_Result = v_append;
  val v_nth = f_nth _;
  def f_nth(v_i : T_Integer, v_l : T_Result):T_ElemType = v_l(v_i);
  val v_nth_from_end = f_nth_from_end _;
  def f_nth_from_end(v_i : T_Integer, v_l : T_Result):T_ElemType =
    v_l.reverse.apply(v_i);
  val v_position = f_position _;
  def f_position(v_x : T_ElemType, v_l : T_Result):T_Integer =
    v_l.indexOf(v_x);
  val v_position_from_end = f_position_from_end _;
  def f_position_from_end(v_x : T_ElemType, v_l : T_Result):T_Integer =
    v_l.reverse.indexOf(v_l);
  val v_subseq = f_subseq _;
  def f_subseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
    v_l.slice(v_start,v_finish);
  val v_subseq_from_end = f_subseq_from_end _;
  def f_subseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
    throw new UnsupportedOperationException("subseq_from_end");
  val v_butsubseq = f_butsubseq _;
  def f_butsubseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
    v_l.slice(0,v_start) ++ v_l.drop(v_finish);
  val v_butsubseq_from_end = f_butsubseq_from_end _;
  def f_butsubseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
    throw new UnsupportedOperationException("but_subseq_from_end");
}

class M_LIST[T_ElemType](name : String, t_ElemType:C_BASIC[T_ElemType]) extends I_LIST[T_ElemType](name) {
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

trait C_SET[T_Result, T_ElemType] extends C_TYPE[T_Result] with C_COMPARABLE[T_Result] with C_COLLECTION[T_Result,T_ElemType] with C_ABSTRACT_SET[T_Result,T_ElemType] with C_COMBINABLE[T_Result] {
}

import scala.collection.immutable.Set;
import scala.collection.immutable.ListSet;

class M_SET[T_ElemType](name : String, t_ElemType:C_BASIC[T_ElemType]) 
extends I_TYPE[Set[T_ElemType]](name)
with C_SET[Set[T_ElemType],T_ElemType]
{
  override def f_assert(v__88 : T_Result) : Unit = {};
  
  val v__op_AC = f__op_AC _;
  def f__op_AC(v_l : Seq[T_ElemType]):T_Result = ListSet(v_l:_*);
  
  val p__op_AC = new PatternSeqFunction[T_Result,T_ElemType](u__op_AC);
  def u__op_AC(x:Any) : Option[(T_Result,Seq[T_ElemType])] = x match {
    case x:T_Result => Some((x,x.toSeq));
    case _ => None
  };
  
  val v_member = f_member _;
  def f_member(v_e : T_ElemType, v_l : T_Result):T_Boolean =
    v_l.contains(v_e);
  
  val v_append = f_append _;
  def f_append(v_l1 : T_Result, v_l2 : T_Result):T_Result =
    v_l1 ++ v_l2;
  def u_append(x:Any) : Option[(T_Result,T_Result,T_Result)] = x match {
    case x:T_Result =>
    if (x.size > 1) {
      val y : T_ElemType = x.iterator.next();
      Some((x,ListSet(y),x - y))
    } else {
      None
    };
    case _ => None;
  }
  val p_append = new PatternFunction[(T_Result,T_Result,T_Result)](u_append);
  
  val v_single = f_single _;
  def f_single(v_x : T_ElemType):T_Result = ListSet(v_x);
  def u_single(x:Any) : Option[(T_Result,T_ElemType)] = x match {
    case x:T_Result => if (x.size == 1) Some(x,x.iterator.next()) else None;
    case _ => None
  };
  val p_single = new PatternFunction[(T_Result,T_ElemType)](u_single);
  
  val v_none = f_none _;
  def f_none():T_Result = ListSet();
  def u_none(x:Any) : Option[T_Result] = x match {
    case x:T_Result =>
    if (x.size == 0) Some(x) else None;
    case _ => None
  };
  val p_none = new PatternFunction[T_Result](u_none);
  
  val v_initial = v_none();
  
  val v_less = f_less _;
  def f_less(v__99 : T_Result, v__100 : T_Result):T_Boolean =
    v__99.subsetOf(v__100) && v__99 != v__100;
  val v_less_equal = f_less_equal _;
  def f_less_equal(v__101 : T_Result, v__102 : T_Result):T_Boolean =
    v__101.subsetOf(v__102);
  
  val v_union = f_union _;
  def f_union(v__106 : T_Result, v__107 : T_Result):T_Result =
    v__106 ++ v__107;
  val v_intersect = f_intersect _;
  def f_intersect(v__108 : T_Result, v__109 : T_Result):T_Result =
    v__108 intersect v__109;
  val v_difference = f_difference _;
  def f_difference(v__110 : T_Result, v__111 : T_Result):T_Result =
    v__110 -- v__111;
  val v_combine = f_union _;
}


trait C_MULTISET[T_Result, T_ElemType] extends C_TYPE[T_Result]with C_BAG[T_Result,T_ElemType] with C_BASIC[T_Result] with C_COMPARABLE[T_Result] with C_COLLECTION[T_Result,T_ElemType] with C_ABSTRACT_SET[T_Result,T_ElemType] with C_COMBINABLE[T_Result] {
  val v_equal : (T_Result,T_Result) => T_Boolean;
  val v_less : (T_Result,T_Result) => T_Boolean;
  val v_less_equal : (T_Result,T_Result) => T_Boolean;
  val v__op_AC : (Seq[T_ElemType]) => T_Result;
  val p__op_AC : PatternSeqFunction[T_Result,T_ElemType];
  val v_member : (T_ElemType,T_Result) => T_Boolean;
  val v_count : (T_ElemType,T_Result) => T_Integer;
  val v_union : (T_Result,T_Result) => T_Result;
  val v_intersect : (T_Result,T_Result) => T_Result;
  val v_difference : (T_Result,T_Result) => T_Result;
  val v_combine : (T_Result,T_Result) => T_Result;
}

class M_MULTISET[T_ElemType](name : String,val t_ElemType : C_TYPE[T_ElemType] with C_BASIC[T_ElemType])
  extends M_BAG[T_ElemType](name,t_ElemType)
  with C_MULTISET[T_BAG[T_ElemType],T_ElemType]
{
  val t_Result : this.type = this;

  // An inefficient implementation
  override
  val v_equal = f_equal _;
  override
  def f_equal(v__112 : T_Result, v__113 : T_Result):T_Boolean =
    f_less_equal(v__112,v__113) && f_less_equal(v__113,v__112);

  val v_less = f_less _;
  def f_less(v__114 : T_Result, v__115 : T_Result):T_Boolean =
    f_less_equal(v__114,v__115) && !f_less_equal(v__115,v__114);

  val v_less_equal = f_less_equal _;
  def f_less_equal(l1 : T_Result, l2 : T_Result):T_Boolean = {
    l1 match {
      case Nil => true;
      case x::r => (f_count(x,l1)<=f_count(x,l2)) && 
	f_less_equal(r,l2)
    }
  }

  val v_count = f_count _;
  def f_count(v_x : T_ElemType, v_l : T_Result):T_Integer = {
    v_l match {
      case x::r => (if (t_ElemType.v_equal(v_x,x)) 1 else 0) +
	           f_count(v_x,r)
      case Nil => 0
    }
  }

  val v_union = f_union _;
  def f_union(l1 : T_Result, l2 : T_Result):T_Result = {
    l1 match {
      case x::r => {
	val rp = f_union(r,l2);
	if (f_count(x,r) >= f_count(x,l2)) x::rp else rp
      }
      case Nil => l2
    }
  }

  val v_intersect = f_intersect _;
  def f_intersect(l1 : T_Result, l2 : T_Result):T_Result = {
    l1 match {
      case x::r => {
	val rp = f_intersect(r,l2);
	if (f_count(x,r) < f_count(x,l2)) x::rp else rp
      }
      case Nil => Nil;
    }
  }

  val v_difference = f_difference _;
  def f_difference(l1 : T_Result, l2 : T_Result):T_Result = {
    l1 match {
      case x::r => {
	val rp = f_difference(r,l2);
	if (f_count(x,r) >= f_count(x,l2)) x::rp else rp
      }
      case Nil => Nil;
    }
  }

/*
  override
  val v_combine = f_combine _;
  override
  def f_combine(v_x : T_Result, v_y : T_Result):T_Result = v_union(v_x,v_y);
  override def finish() : Unit = {
    super.finish();
  }
*/

}

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
    def f__op_AC(v__132 : Seq[T_ElemType]):T_Result;
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
  val v__op_AC : (Seq[T_ElemType]) => T_Result;
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
    def f__op_AC(v__145 : Seq[T_ElemType]):T_Result;
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
extends C_MAKE_LATTICE[T_Result, T_ST] with C_SET[T_Result,T_ElemType]
{
}

class M_UNION_LATTICE[T_ElemType, T_ST]
                     (name : String, t_ElemType:Any,
		      t_ST:C_SET[T_ST,T_ElemType]) 
extends M_MAKE_LATTICE[T_ST](name,t_ST,t_ST.v_none(),
			     new M__basic_3[ T_ST](t_ST).v__op_z,
			     new M__basic_3[ T_ST](t_ST).v__op_z0,
			     new M__basic_19[ T_ElemType,T_ST](t_ElemType,t_ST).v__op_5w,
			     new M__basic_19[ T_ElemType,T_ST](t_ElemType,t_ST).v__op_w5)
with C_UNION_LATTICE[T_ST,T_ElemType,T_ST]
{
  val v_less = t_ST.v_less;
  val v_less_equal = t_ST.v_less_equal;
  val v__op_AC = t_ST.v__op_AC;
  val p__op_AC = t_ST.p__op_AC;
  val p_append = t_ST.p_append;
  val p_single = t_ST.p_single;
  val p_none = t_ST.p_none;
  val v_member = t_ST.v_member;
  val v_append = t_ST.v_append;
  val v_single = t_ST.v_single;
  val v_none = t_ST.v_none;
  val v_union = t_ST.v_union;
  val v_intersect = t_ST.v_intersect;
  val v_difference = t_ST.v_difference;
  val v_string = t_ST.v_string;
  val v_assert = t_ST.v_assert;
  val v_node_equivalent = t_ST.v_node_equivalent;
}


trait C_INTERSECTION_LATTICE[T_Result, T_ElemType, T_ST]
extends C_MAKE_LATTICE[T_Result, T_ST] with C_SET[T_Result,T_ElemType]
{
}

class M_INTERSECTION_LATTICE[T_ElemType, T_ST]
(name : String, t_ElemType:Any,t_ST:C_SET[T_ST,T_ElemType],v_universe : T_ST)
extends M_MAKE_LATTICE[T_ST](name,t_ST,v_universe,
			     new M__basic_3[ T_ST](t_ST).v__op_1,
			     new M__basic_3[ T_ST](t_ST).v__op_10,
			     new M__basic_19[ T_ElemType,T_ST](t_ElemType,t_ST).v__op_w5,
			     new M__basic_19[ T_ElemType,T_ST](t_ElemType,t_ST).v__op_5w)
with C_INTERSECTION_LATTICE[T_ST,T_ElemType,T_ST] 
{
  val v_less = t_ST.v_less;
  val v_less_equal = t_ST.v_less_equal;
  val v__op_AC = t_ST.v__op_AC;
  val p__op_AC = t_ST.p__op_AC;
  val p_append = t_ST.p_append;
  val p_single = t_ST.p_single;
  val p_none = t_ST.p_none;
  val v_member = t_ST.v_member;
  val v_append = t_ST.v_append;
  val v_single = t_ST.v_single;
  val v_none = t_ST.v_none;
  val v_union = t_ST.v_union;
  val v_intersect = t_ST.v_intersect;
  val v_difference = t_ST.v_difference;
  val v_string = t_ST.v_string;
  val v_assert = t_ST.v_assert;
  val v_node_equivalent = t_ST.v_node_equivalent;
}

trait C_PAIR[T_Result, T_T1, T_T2] extends C_BASIC[T_Result] with C_TYPE[T_Result] {
  val p_pair : PatternFunction[(T_Result,T_T1,T_T2)];
  val v_pair : (T_T1,T_T2) => T_Result;
  val v_fst : (T_Result) => T_T1;
  val v_snd : (T_Result) => T_T2;
}

class T_PAIR[T_T1,T_T2]
(t_PAIR : C_PAIR[T_PAIR[T_T1,T_T2],T_T1,T_T2]) 
extends Value(t_PAIR) { }

class M_PAIR[T_T1, T_T2]
      (name : String, t_T1:C_BASIC[T_T1],t_T2:C_BASIC[T_T2]) 
extends I_TYPE[T_PAIR[T_T1,T_T2]](name)
with    C_PAIR[T_PAIR[T_T1,T_T2],T_T1,T_T2]
{
  val t_Result : this.type = this;
  case class c_pair(v_x : T_T1,v_y : T_T2) extends T_Result(t_Result) {
    def children : List[Node] = List();
    override def toString() : String =
      Debug.with_level {
	"pair(" + v_x + "," + v_y + ")"
      }
  }
  val v_pair = f_pair _;
  def f_pair(v_x : T_T1, v_y : T_T2):T_Result = c_pair(v_x,v_y);
  def u_pair(x:Any) : Option[(T_Result,T_T1,T_T2)] = x match {
    case x@c_pair(v_x,v_y) => Some((x,v_x,v_y));
    case _ => None };
  val p_pair = new PatternFunction[(T_Result,T_T1,T_T2)](u_pair);
  
  val v_fst = f_fst _;
  def f_fst(v_p : T_Result):T_T1 =
    v_p match {
      case p_pair(_,v_x,_) => v_x
      case _ => throw UndefinedAttributeException("local fst");
    };
  
  val v_snd = f_snd _;
  def f_snd(v_p : T_Result):T_T2 =
    v_p match {
      case p_pair(_,_,v_y) => return v_y
      case _ => throw UndefinedAttributeException("local snd")
    };
  
  override def f_equal(v_x : T_Result, v_y : T_Result):T_Boolean = {
    v_x match {
      case p_pair(_,v_x1,v_x2) => {
        v_y match {
          case p_pair(_,v_y1,v_y2) => {
            return v_and(new M__basic_2[ T_T1](t_T1).v__op_0(v_x1,v_y1),new 
			 M__basic_2[ T_T2](t_T2).v__op_0(v_x2,v_y2));
          }
        }
      }
    }
    throw UndefinedAttributeException("local equal");
  }
}

trait C_STRING[T_Result] extends C_ORDERED[T_Result] 
	 with C_PRINTABLE[T_Result] with C_LIST[T_Result,Char] {
}

class M_STRING(name : String)
extends I_TYPE[String](name)
with C_STRING[String]
{
  override def f_assert(v__88 : T_Result) : Unit = {};
  
  val v__op_AC = f__op_AC _;
  def f__op_AC(v_l : Char*):T_Result = (v_l :\ "")((c,s) => c + s);
  
  val p__op_AC = new PatternSeqFunction[T_Result,Char](u__op_AC);
  def u__op_AC(x:Any) : Option[(T_Result,Seq[Char])] = x match {
    case x:String => Some((x,x));
    case _ => None
  };
  
  val v_member = f_member _;
  def f_member(v_e : Char, v_l : T_Result):T_Boolean =
    v_l.indexOf(v_e) != -1;
  
  val v_append = f_append _;
  def f_append(v_l1 : T_Result, v_l2 : T_Result):T_Result = v_l1 + v_l2;
  def u_append(x:Any) : Option[(T_Result,T_Result,T_Result)] = x match {
    case x:String  =>
    if (x.length() > 1) Some((x,x.substring(0,1),x.substring(1)))
    else None;
    case _ => None
  };
  val p_append = new PatternFunction[(T_Result,T_Result,T_Result)](u_append);
  
  val v_single = f_single _;
  def f_single(v_x : Char):T_Result = "" + v_x;
  def u_single(x:Any) : Option[(T_Result,Char)] = x match {
    case x:String =>
    if (x.length() == 1) Some((x,x.charAt(0)))
    else None;
    case _ => None
  };
  val p_single = new PatternFunction[(T_Result,Char)](u_single);
  
  val v_none = f_none _;
  def f_none():T_Result = "";
  def u_none(x:Any) : Option[T_Result] = x match {
    case x:String =>
    if (x.length() == 0) Some((x)) else None;
    case _ => None
  };
  val p_none = new PatternFunction[T_Result](u_none);
  
  val v_cons = f_cons _;
  def f_cons(x : Char, s : String) : String = x + s;
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
    v_l.substring(v_start,v_finish);
  val v_subseq_from_end = f_subseq_from_end _;
  def f_subseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
    throw new UnsupportedOperationException("subseq_from_end");
  val v_butsubseq = f_butsubseq _;
  def f_butsubseq(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
    v_l.substring(0,v_start) + v_l.substring(v_finish);
  val v_butsubseq_from_end = f_butsubseq_from_end _;
  def f_butsubseq_from_end(v_l : T_Result, v_start : T_Integer, v_finish : T_Integer):T_Result =
    throw new UnsupportedOperationException("but_subseq_from_end");
  
  val v_less = f_less _;
  def f_less(v_x : T_Result, v_y : T_Result):T_Boolean =
    v_x.compareTo(v_y) < 0;
  val v_less_equal = f_less_equal _;
  def f_less_equal(v_x : T_Result, v_y : T_Result):T_Boolean =
    v_x.compareTo(v_y) <= 0;
}

class M__basic_21[T_T,T_U](t_T:C_PRINTABLE[T_T],t_U:C_PRINTABLE[T_U]) {
  val v__op_BB = f__op_BB _;
  def f__op_BB(v_x : T_T, v_y : T_U):T_String = new M__basic_18[ T_String](t_String).v__op_ss(t_T.v_string(v_x),t_U.v_string(v_y));
};

class M__basic_22[T_ElemType,T_T](t_ElemType:Any,t_T:C_READ_ONLY_COLLECTION[T_T,T_ElemType]) {
  val v_length = f_length _;
  def f_length(v_l : T_T):T_Integer = v_l match {
    case t_T.p_none(_) => 0
    case t_T.p_single(_,_) => 1
    case t_T.p_append(_,l1,l2) => f_length(l1) + f_length(l2)
  };
};

class M__basic_23[T_T](t_T:Any) {
  val v_debug_output = f_debug_output _;
  def f_debug_output(v_x : T_T):T_String = v_x.toString();
};

class M__basic_24[T_Node <: Node](t_Node:C_PHYLUM[T_Node]) {
  val v_lineno = f_lineno _;
  def f_lineno(v_x : T_Node):T_Integer =
    v_x.asInstanceOf[Node].lineNumber;
};

