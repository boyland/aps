// Generated by aps2scala version 0.3.6
import basic_implicit._;
object classic_binding_implicit {
  val classic_binding_loaded = true;
import simple_implicit._;
type T_NAME_RESOLUTION[T_T] = T_T;
}
import classic_binding_implicit._;

import simple_implicit._;
trait C_NAME_RESOLUTION[T_Result, T_T] extends C_TYPE[T_Result] with C_SIMPLE[T_Result] {
  type T_ShapeStructure <: Node;
  val t_ShapeStructure : C_PHYLUM[T_ShapeStructure];
  val p_shape : PatternFunction[(T_ShapeStructure,T_String)];
  def v_shape : (T_String) => T_ShapeStructure;
  type T_Shape;
  val t_Shape : C_TYPE[T_Shape];
  val v_type_shape : (T_Type) => T_Shape;
  val v_expr_shape : (T_Expr) => T_Shape;
  type T_Entity;
  val t_Entity : C_TYPE[T_Entity]with C_PAIR[T_Entity,T_String,T_Shape];
  type T_Environment;
  val t_Environment : C_TYPE[T_Environment]with C_LIST[T_Environment,T_Entity];
  def v_initial_env : T_Environment;
  val v_block_env : (T_Block) => T_Environment;
  val v_decls_envin : (T_Decls) => T_Environment;
  val v_decls_envout : (T_Decls) => T_Environment;
  val v_decl_envin : (T_Decl) => T_Environment;
  val v_decl_envout : (T_Decl) => T_Environment;
  val v_stmts_env : (T_Stmts) => T_Environment;
  val v_stmt_env : (T_Stmt) => T_Environment;
  val v_expr_env : (T_Expr) => T_Environment;
  type T_Used;
  val t_Used : C_TYPE[T_Used]with C_SET[T_Used,T_String];
  val v_block_used : (T_Block) => T_Used;
  val v_decls_uin : (T_Decls) => T_Used;
  val v_decls_uout : (T_Decls) => T_Used;
  val v_decl_uin : (T_Decl) => T_Used;
  val v_decl_uout : (T_Decl) => T_Used;
  val v_stmts_used : (T_Stmts) => T_Used;
  val v_stmt_used : (T_Stmt) => T_Used;
  val v_expr_used : (T_Expr) => T_Used;
  type T_Messages;
  val t_Messages : C_TYPE[T_Messages]with C_BAG[T_Messages,T_String];
  val v_prog_msgs : (T_Program) => T_Messages;
  val v_block_msgs : (T_Block) => T_Messages;
  val v_decls_msgs : (T_Decls) => T_Messages;
  val v_decl_msgs : (T_Decl) => T_Messages;
  val v_stmts_msgs : (T_Stmts) => T_Messages;
  val v_stmt_msgs : (T_Stmt) => T_Messages;
  val v_expr_msgs : (T_Expr) => T_Messages;
  val v__op_nn : (T_Messages,T_Messages) => T_Messages;
  def v_not_found : T_Shape;
  def v_int_shape : T_Shape;
  def v_str_shape : T_Shape;
  val v_lookup : (T_String,T_Environment) => T_Shape;
}

class M_NAME_RESOLUTION[T_T](name : String,val t_T : C_TYPE[T_T] with C_SIMPLE[T_T])
  extends Module(name)
  with C_NAME_RESOLUTION[T_T,T_T]
{
  type T_Result = T_T;
  val v_equal = t_T.v_equal;
  val v_string = t_T.v_string;
  val v_assert = t_T.v_assert;
  val v_node_equivalent = t_T.v_node_equivalent;
  type T_Program = t_T.T_Program;
  val t_Program = t_T.t_Program;
  type T_Block = t_T.T_Block;
  val t_Block = t_T.t_Block;
  type T_Decls = t_T.T_Decls;
  val t_Decls = t_T.t_Decls;
  type T_Decl = t_T.T_Decl;
  val t_Decl = t_T.t_Decl;
  type T_Type = t_T.T_Type;
  val t_Type = t_T.t_Type;
  type T_Stmts = t_T.T_Stmts;
  val t_Stmts = t_T.t_Stmts;
  type T_Stmt = t_T.T_Stmt;
  val t_Stmt = t_T.t_Stmt;
  type T_Expr = t_T.T_Expr;
  val t_Expr = t_T.t_Expr;
  val p_program = t_T.p_program;
  val v_program = t_T.v_program;
  val p_block = t_T.p_block;
  val v_block = t_T.v_block;
  val p_no_decls = t_T.p_no_decls;
  val v_no_decls = t_T.v_no_decls;
  val p_xcons_decls = t_T.p_xcons_decls;
  val v_xcons_decls = t_T.v_xcons_decls;
  val p_decl = t_T.p_decl;
  val v_decl = t_T.v_decl;
  val p_integer_type = t_T.p_integer_type;
  val v_integer_type = t_T.v_integer_type;
  val p_string_type = t_T.p_string_type;
  val v_string_type = t_T.v_string_type;
  val p_no_stmts = t_T.p_no_stmts;
  val v_no_stmts = t_T.v_no_stmts;
  val p_xcons_stmts = t_T.p_xcons_stmts;
  val v_xcons_stmts = t_T.v_xcons_stmts;
  val p_block_stmt = t_T.p_block_stmt;
  val v_block_stmt = t_T.v_block_stmt;
  val p_assign_stmt = t_T.p_assign_stmt;
  val v_assign_stmt = t_T.v_assign_stmt;
  val p_intconstant = t_T.p_intconstant;
  val v_intconstant = t_T.v_intconstant;
  val p_strconstant = t_T.p_strconstant;
  val v_strconstant = t_T.v_strconstant;
  val p_variable = t_T.p_variable;
  val v_variable = t_T.v_variable;

  val t_Result : this.type = this;
  abstract class T_ShapeStructure(t : I_PHYLUM[T_ShapeStructure]) extends Node(t) {}
  val t_ShapeStructure = new I_PHYLUM[T_ShapeStructure]("ShapeStructure");

  case class c_shape(v_name : T_String) extends T_ShapeStructure(t_ShapeStructure) {
    override def children : List[Node] = List();
    override def toString() : String = Debug.with_level {
      "shape("+ v_name+ ")";
    }
  }
  def u_shape(x:Any) : Option[(T_ShapeStructure,T_String)] = x match {
    case x@c_shape(v_name) => Some((x,v_name));
    case _ => None };
  val v_shape = f_shape _;
  def f_shape(v_name : T_String):T_ShapeStructure = c_shape(v_name).register;
  val p_shape = new PatternFunction[(T_ShapeStructure,T_String)](u_shape);

  type T_Shape = T_ShapeStructure;
  val t_Shape = t_ShapeStructure;
  private class E_type_shape(anchor : T_Type) extends Evaluation[T_Type,T_Shape](anchor,anchor.toString()+"."+"type_shape") {
    override def compute : ValueType = c_type_shape(anchor);
  }
  private object a_type_shape extends Attribute[T_Type,T_Shape](t_Type,t_Shape,"type_shape") {
    override def createEvaluation(anchor : T_Type) : Evaluation[T_Type,T_Shape] = new E_type_shape(anchor);
  }
  val v_type_shape : T_Type => T_Shape = a_type_shape.get _;

  private class E_expr_shape(anchor : T_Expr) extends Evaluation[T_Expr,T_Shape](anchor,anchor.toString()+"."+"expr_shape") {
    override def compute : ValueType = c_expr_shape(anchor);
  }
  private object a_expr_shape extends Attribute[T_Expr,T_Shape](t_Expr,t_Shape,"expr_shape") {
    override def createEvaluation(anchor : T_Expr) : Evaluation[T_Expr,T_Shape] = new E_expr_shape(anchor);
  }
  val v_expr_shape : T_Expr => T_Shape = a_expr_shape.get _;

  val t_Entity = new M_PAIR[T_String,T_Shape]("Entity",t_String,t_Shape);
  type T_Entity = /*TI*/T_PAIR[T_String,T_Shape];
  val t_Environment = new M_LIST[T_Entity]("Environment",t_Entity);
  type T_Environment = /*TI*/T_LIST[T_Entity];
  val v_initial_env:T_Environment = t_Environment.v_none();
  private class E_block_env(anchor : T_Block) extends Evaluation[T_Block,T_Environment](anchor,anchor.toString()+"."+"block_env") {
    override def compute : ValueType = c_block_env(anchor);
  }
  private object a_block_env extends Attribute[T_Block,T_Environment](t_Block,t_Environment,"block_env") {
    override def createEvaluation(anchor : T_Block) : Evaluation[T_Block,T_Environment] = new E_block_env(anchor);
  }
  val v_block_env : T_Block => T_Environment = a_block_env.get _;

  private class E_decls_envin(anchor : T_Decls) extends Evaluation[T_Decls,T_Environment](anchor,anchor.toString()+"."+"decls_envin") {
    override def compute : ValueType = c_decls_envin(anchor);
  }
  private object a_decls_envin extends Attribute[T_Decls,T_Environment](t_Decls,t_Environment,"decls_envin") {
    override def createEvaluation(anchor : T_Decls) : Evaluation[T_Decls,T_Environment] = new E_decls_envin(anchor);
  }
  val v_decls_envin : T_Decls => T_Environment = a_decls_envin.get _;

  private class E_decls_envout(anchor : T_Decls) extends Evaluation[T_Decls,T_Environment](anchor,anchor.toString()+"."+"decls_envout") {
    override def compute : ValueType = c_decls_envout(anchor);
  }
  private object a_decls_envout extends Attribute[T_Decls,T_Environment](t_Decls,t_Environment,"decls_envout") {
    override def createEvaluation(anchor : T_Decls) : Evaluation[T_Decls,T_Environment] = new E_decls_envout(anchor);
  }
  val v_decls_envout : T_Decls => T_Environment = a_decls_envout.get _;

  private class E_decl_envin(anchor : T_Decl) extends Evaluation[T_Decl,T_Environment](anchor,anchor.toString()+"."+"decl_envin") {
    override def compute : ValueType = c_decl_envin(anchor);
  }
  private object a_decl_envin extends Attribute[T_Decl,T_Environment](t_Decl,t_Environment,"decl_envin") {
    override def createEvaluation(anchor : T_Decl) : Evaluation[T_Decl,T_Environment] = new E_decl_envin(anchor);
  }
  val v_decl_envin : T_Decl => T_Environment = a_decl_envin.get _;

  private class E_decl_envout(anchor : T_Decl) extends Evaluation[T_Decl,T_Environment](anchor,anchor.toString()+"."+"decl_envout") {
    override def compute : ValueType = c_decl_envout(anchor);
  }
  private object a_decl_envout extends Attribute[T_Decl,T_Environment](t_Decl,t_Environment,"decl_envout") {
    override def createEvaluation(anchor : T_Decl) : Evaluation[T_Decl,T_Environment] = new E_decl_envout(anchor);
  }
  val v_decl_envout : T_Decl => T_Environment = a_decl_envout.get _;

  private class E_stmts_env(anchor : T_Stmts) extends Evaluation[T_Stmts,T_Environment](anchor,anchor.toString()+"."+"stmts_env") {
    override def compute : ValueType = c_stmts_env(anchor);
  }
  private object a_stmts_env extends Attribute[T_Stmts,T_Environment](t_Stmts,t_Environment,"stmts_env") {
    override def createEvaluation(anchor : T_Stmts) : Evaluation[T_Stmts,T_Environment] = new E_stmts_env(anchor);
  }
  val v_stmts_env : T_Stmts => T_Environment = a_stmts_env.get _;

  private class E_stmt_env(anchor : T_Stmt) extends Evaluation[T_Stmt,T_Environment](anchor,anchor.toString()+"."+"stmt_env") {
    override def compute : ValueType = c_stmt_env(anchor);
  }
  private object a_stmt_env extends Attribute[T_Stmt,T_Environment](t_Stmt,t_Environment,"stmt_env") {
    override def createEvaluation(anchor : T_Stmt) : Evaluation[T_Stmt,T_Environment] = new E_stmt_env(anchor);
  }
  val v_stmt_env : T_Stmt => T_Environment = a_stmt_env.get _;

  private class E_expr_env(anchor : T_Expr) extends Evaluation[T_Expr,T_Environment](anchor,anchor.toString()+"."+"expr_env") {
    override def compute : ValueType = c_expr_env(anchor);
  }
  private object a_expr_env extends Attribute[T_Expr,T_Environment](t_Expr,t_Environment,"expr_env") {
    override def createEvaluation(anchor : T_Expr) : Evaluation[T_Expr,T_Environment] = new E_expr_env(anchor);
  }
  val v_expr_env : T_Expr => T_Environment = a_expr_env.get _;

  val t_Used = new M_SET[T_String]("Used",t_String);
  type T_Used = /*TI*/T_SET[T_String];
  private class E_block_used(anchor : T_Block) extends Evaluation[T_Block,T_Used](anchor,anchor.toString()+"."+"block_used") {
    override def compute : ValueType = c_block_used(anchor);
  }
  private object a_block_used extends Attribute[T_Block,T_Used](t_Block,t_Used,"block_used") {
    override def createEvaluation(anchor : T_Block) : Evaluation[T_Block,T_Used] = new E_block_used(anchor);
  }
  val v_block_used : T_Block => T_Used = a_block_used.get _;

  private class E_decls_uin(anchor : T_Decls) extends Evaluation[T_Decls,T_Used](anchor,anchor.toString()+"."+"decls_uin") {
    override def compute : ValueType = c_decls_uin(anchor);
  }
  private object a_decls_uin extends Attribute[T_Decls,T_Used](t_Decls,t_Used,"decls_uin") {
    override def createEvaluation(anchor : T_Decls) : Evaluation[T_Decls,T_Used] = new E_decls_uin(anchor);
  }
  val v_decls_uin : T_Decls => T_Used = a_decls_uin.get _;

  private class E_decls_uout(anchor : T_Decls) extends Evaluation[T_Decls,T_Used](anchor,anchor.toString()+"."+"decls_uout") {
    override def compute : ValueType = c_decls_uout(anchor);
  }
  private object a_decls_uout extends Attribute[T_Decls,T_Used](t_Decls,t_Used,"decls_uout") {
    override def createEvaluation(anchor : T_Decls) : Evaluation[T_Decls,T_Used] = new E_decls_uout(anchor);
  }
  val v_decls_uout : T_Decls => T_Used = a_decls_uout.get _;

  private class E_decl_uin(anchor : T_Decl) extends Evaluation[T_Decl,T_Used](anchor,anchor.toString()+"."+"decl_uin") {
    override def compute : ValueType = c_decl_uin(anchor);
  }
  private object a_decl_uin extends Attribute[T_Decl,T_Used](t_Decl,t_Used,"decl_uin") {
    override def createEvaluation(anchor : T_Decl) : Evaluation[T_Decl,T_Used] = new E_decl_uin(anchor);
  }
  val v_decl_uin : T_Decl => T_Used = a_decl_uin.get _;

  private class E_decl_uout(anchor : T_Decl) extends Evaluation[T_Decl,T_Used](anchor,anchor.toString()+"."+"decl_uout") {
    override def compute : ValueType = c_decl_uout(anchor);
  }
  private object a_decl_uout extends Attribute[T_Decl,T_Used](t_Decl,t_Used,"decl_uout") {
    override def createEvaluation(anchor : T_Decl) : Evaluation[T_Decl,T_Used] = new E_decl_uout(anchor);
  }
  val v_decl_uout : T_Decl => T_Used = a_decl_uout.get _;

  private class E_stmts_used(anchor : T_Stmts) extends Evaluation[T_Stmts,T_Used](anchor,anchor.toString()+"."+"stmts_used") {
    override def compute : ValueType = c_stmts_used(anchor);
  }
  private object a_stmts_used extends Attribute[T_Stmts,T_Used](t_Stmts,t_Used,"stmts_used") {
    override def createEvaluation(anchor : T_Stmts) : Evaluation[T_Stmts,T_Used] = new E_stmts_used(anchor);
  }
  val v_stmts_used : T_Stmts => T_Used = a_stmts_used.get _;

  private class E_stmt_used(anchor : T_Stmt) extends Evaluation[T_Stmt,T_Used](anchor,anchor.toString()+"."+"stmt_used") {
    override def compute : ValueType = c_stmt_used(anchor);
  }
  private object a_stmt_used extends Attribute[T_Stmt,T_Used](t_Stmt,t_Used,"stmt_used") {
    override def createEvaluation(anchor : T_Stmt) : Evaluation[T_Stmt,T_Used] = new E_stmt_used(anchor);
  }
  val v_stmt_used : T_Stmt => T_Used = a_stmt_used.get _;

  private class E_expr_used(anchor : T_Expr) extends Evaluation[T_Expr,T_Used](anchor,anchor.toString()+"."+"expr_used") {
    override def compute : ValueType = c_expr_used(anchor);
  }
  private object a_expr_used extends Attribute[T_Expr,T_Used](t_Expr,t_Used,"expr_used") {
    override def createEvaluation(anchor : T_Expr) : Evaluation[T_Expr,T_Used] = new E_expr_used(anchor);
  }
  val v_expr_used : T_Expr => T_Used = a_expr_used.get _;

  val t_Messages = new M_BAG[T_String]("Messages",t_String);
  type T_Messages = /*TI*/T_BAG[T_String];
  private class E_prog_msgs(anchor : T_Program) extends Evaluation[T_Program,T_Messages](anchor,anchor.toString()+"."+"prog_msgs") {
    override def compute : ValueType = c_prog_msgs(anchor);
  }
  private object a_prog_msgs extends Attribute[T_Program,T_Messages](t_Program,t_Messages,"prog_msgs") {
    override def createEvaluation(anchor : T_Program) : Evaluation[T_Program,T_Messages] = new E_prog_msgs(anchor);
  }
  val v_prog_msgs : T_Program => T_Messages = a_prog_msgs.get _;

  private class E_block_msgs(anchor : T_Block) extends Evaluation[T_Block,T_Messages](anchor,anchor.toString()+"."+"block_msgs") {
    override def compute : ValueType = c_block_msgs(anchor);
  }
  private object a_block_msgs extends Attribute[T_Block,T_Messages](t_Block,t_Messages,"block_msgs") {
    override def createEvaluation(anchor : T_Block) : Evaluation[T_Block,T_Messages] = new E_block_msgs(anchor);
  }
  val v_block_msgs : T_Block => T_Messages = a_block_msgs.get _;

  private class E_decls_msgs(anchor : T_Decls) extends Evaluation[T_Decls,T_Messages](anchor,anchor.toString()+"."+"decls_msgs") {
    override def compute : ValueType = c_decls_msgs(anchor);
  }
  private object a_decls_msgs extends Attribute[T_Decls,T_Messages](t_Decls,t_Messages,"decls_msgs") {
    override def createEvaluation(anchor : T_Decls) : Evaluation[T_Decls,T_Messages] = new E_decls_msgs(anchor);
  }
  val v_decls_msgs : T_Decls => T_Messages = a_decls_msgs.get _;

  private class E_decl_msgs(anchor : T_Decl) extends Evaluation[T_Decl,T_Messages](anchor,anchor.toString()+"."+"decl_msgs") {
    override def compute : ValueType = c_decl_msgs(anchor);
  }
  private object a_decl_msgs extends Attribute[T_Decl,T_Messages](t_Decl,t_Messages,"decl_msgs") {
    override def createEvaluation(anchor : T_Decl) : Evaluation[T_Decl,T_Messages] = new E_decl_msgs(anchor);
  }
  val v_decl_msgs : T_Decl => T_Messages = a_decl_msgs.get _;

  private class E_stmts_msgs(anchor : T_Stmts) extends Evaluation[T_Stmts,T_Messages](anchor,anchor.toString()+"."+"stmts_msgs") {
    override def compute : ValueType = c_stmts_msgs(anchor);
  }
  private object a_stmts_msgs extends Attribute[T_Stmts,T_Messages](t_Stmts,t_Messages,"stmts_msgs") {
    override def createEvaluation(anchor : T_Stmts) : Evaluation[T_Stmts,T_Messages] = new E_stmts_msgs(anchor);
  }
  val v_stmts_msgs : T_Stmts => T_Messages = a_stmts_msgs.get _;

  private class E_stmt_msgs(anchor : T_Stmt) extends Evaluation[T_Stmt,T_Messages](anchor,anchor.toString()+"."+"stmt_msgs") {
    override def compute : ValueType = c_stmt_msgs(anchor);
  }
  private object a_stmt_msgs extends Attribute[T_Stmt,T_Messages](t_Stmt,t_Messages,"stmt_msgs") {
    override def createEvaluation(anchor : T_Stmt) : Evaluation[T_Stmt,T_Messages] = new E_stmt_msgs(anchor);
  }
  val v_stmt_msgs : T_Stmt => T_Messages = a_stmt_msgs.get _;

  private class E_expr_msgs(anchor : T_Expr) extends Evaluation[T_Expr,T_Messages](anchor,anchor.toString()+"."+"expr_msgs") {
    override def compute : ValueType = c_expr_msgs(anchor);
  }
  private object a_expr_msgs extends Attribute[T_Expr,T_Messages](t_Expr,t_Messages,"expr_msgs") {
    override def createEvaluation(anchor : T_Expr) : Evaluation[T_Expr,T_Messages] = new E_expr_msgs(anchor);
  }
  val v_expr_msgs : T_Expr => T_Messages = a_expr_msgs.get _;

  val v__op_nn = f__op_nn _;
  def f__op_nn(v_x : T_Messages, v_y : T_Messages):T_Messages = {
    try {
      Debug.begin("&&("+v_x+","+v_y+")");
      return t_Messages.v_combine(v_x,v_y);
    } finally { Debug.end(); }
  };
  val v_not_found:T_Shape = new M__basic_8[ T_Shape](t_Shape).v_nil;
  val v_int_shape:T_Shape = v_shape("integer");
  val v_str_shape:T_Shape = v_shape("string");
  private class E1_sh(anchor : t_Result.T_Expr) extends Evaluation[t_Result.T_Expr,T_Shape](anchor,anchor.toString()+"."+"sh") {
    override def compute : ValueType = c1_sh(anchor);
  }
  private object a1_sh extends Attribute[t_Result.T_Expr,T_Shape](t_Result.t_Expr,t_Shape,"sh") {
    override def createEvaluation(anchor : t_Result.T_Expr) : Evaluation[t_Result.T_Expr,T_Shape] = new E1_sh(anchor);
  }
  val v_lookup = f_lookup _;
  def f_lookup(v_name : T_String, v_env : T_Environment):T_Shape = {
    try {
      Debug.begin("lookup("+v_name+","+v_env+")");

      { val cond = new M__basic_2[ T_Integer](t_Integer).v__op_0(new M__basic_22[ T_Entity,T_Environment](t_Entity,t_Environment).v_length(v_env),0);
        if (cond) {
          return v_not_found;
        }
        if (!cond) {
          { val cond = new M__basic_2[ T_String](t_String).v__op_0(t_Entity.v_fst(new M__basic_16[ T_Entity,T_Environment](t_Entity,t_Environment).v_first(v_env)),v_name);
            if (cond) {
              return t_Entity.v_snd(new M__basic_16[ T_Entity,T_Environment](t_Entity,t_Environment).v_first(v_env));
            }
            if (!cond) {
              return v_lookup(v_name,new M__basic_17[ T_Entity,T_Environment](t_Entity,t_Environment).v_butfirst(v_env));
            }
          }
        }
      }
      throw Evaluation.UndefinedAttributeException("local lookup");
    } finally { Debug.end(); }
  }

  def c1_sh(anchor : Any) : T_Shape = {
    anchor match {
      case p_variable(v_expr,v_id) => {
        return v_lookup(v_id,v_expr_env(v_expr));
      }
    }
  }
  def c_type_shape(anode : T_Type) : T_Shape = {
    val anchor = anode;
    anchor match {
      case p_integer_type(v_t) => {
        if (anode eq v_t) return v_int_shape;
      }
      case _ => {}
    }
    anchor match {
      case p_string_type(v_t) => {
        if (anode eq v_t) return v_str_shape;
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".type_shape");
  }
  def c_expr_shape(anode : T_Expr) : T_Shape = {
    val anchor = anode;
    anchor match {
      case p_intconstant(v_e,_) => {
        if (anode eq v_e) return v_int_shape;
      }
      case _ => {}
    }
    anchor match {
      case p_strconstant(v_e,_) => {
        if (anode eq v_e) return v_str_shape;
      }
      case _ => {}
    }
    anchor match {
      case p_variable(v_expr,v_id) => {
        if (anode eq v_expr) return a1_sh.get(anchor);
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".expr_shape");
  }
  def c_block_env(anode : T_Block) : T_Environment = {
    val anchor = anode.parent;
    if (!(anchor eq null)) {
      val anchorNodes = anchor.myType.nodes;
      if (anchorNodes == t_Result.t_Program.nodes) anchor match {
        case p_program(v_p,v_b) => {
          if (anode eq v_b) return v_initial_env;
        }
        case _ => {}
      }
      if (anchorNodes == t_Result.t_Stmt.nodes) anchor match {
        case p_block_stmt(v_stmt,v_block) => {
          if (anode eq v_block) return v_stmt_env(v_stmt);
        }
        case _ => {}
      }
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".block_env");
  }
  def c_decls_envin(anode : T_Decls) : T_Environment = {
    val anchor = anode.parent;
    if (!(anchor eq null)) {
      val anchorNodes = anchor.myType.nodes;
      if (anchorNodes == t_Result.t_Block.nodes) anchor match {
        case p_block(v_block,v_decls,v_stmts) => {
          if (anode eq v_decls) return v_block_env(v_block);
        }
        case _ => {}
      }
      if (anchorNodes == t_Result.t_Decls.nodes) anchor match {
        case p_xcons_decls(v_decls0,v_decls1,v_decl) => {
          if (anode eq v_decls1) return v_decls_envin(v_decls0);
        }
        case _ => {}
      }
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decls_envin");
  }
  def c_decls_envout(anode : T_Decls) : T_Environment = {
    val anchor = anode;
    anchor match {
      case p_no_decls(v_decls) => {
        if (anode eq v_decls) return v_decls_envin(v_decls);
      }
      case _ => {}
    }
    anchor match {
      case p_xcons_decls(v_decls0,v_decls1,v_decl) => {
        if (anode eq v_decls0) return v_decl_envout(v_decl);
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decls_envout");
  }
  def c_decl_envin(anode : T_Decl) : T_Environment = {
    val anchor = anode.parent;
    if (!(anchor eq null)) {
      val anchorNodes = anchor.myType.nodes;
      if (anchorNodes == t_Result.t_Decls.nodes) anchor match {
        case p_xcons_decls(v_decls0,v_decls1,v_decl) => {
          if (anode eq v_decl) return v_decls_envout(v_decls1);
        }
        case _ => {}
      }
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decl_envin");
  }
  def c_decl_envout(anode : T_Decl) : T_Environment = {
    val anchor = anode;
    anchor match {
      case p_decl(v_d,v_id,v_ty) => {
        if (anode eq v_d) return t_Environment.v_cons(t_Entity.v_pair(v_id,v_type_shape(v_ty)),v_decl_envin(v_d));
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decl_envout");
  }
  def c_stmts_env(anode : T_Stmts) : T_Environment = {
    val anchor = anode.parent;
    if (!(anchor eq null)) {
      val anchorNodes = anchor.myType.nodes;
      if (anchorNodes == t_Result.t_Block.nodes) anchor match {
        case p_block(v_block,v_decls,v_stmts) => {
          if (anode eq v_stmts) return v_decls_envout(v_decls);
        }
        case _ => {}
      }
      if (anchorNodes == t_Result.t_Stmts.nodes) anchor match {
        case p_xcons_stmts(v_stmts0,v_stmts1,v_stmt) => {
          if (anode eq v_stmts1) return v_stmts_env(v_stmts0);
        }
        case _ => {}
      }
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".stmts_env");
  }
  def c_stmt_env(anode : T_Stmt) : T_Environment = {
    val anchor = anode.parent;
    if (!(anchor eq null)) {
      val anchorNodes = anchor.myType.nodes;
      if (anchorNodes == t_Result.t_Stmts.nodes) anchor match {
        case p_xcons_stmts(v_stmts0,v_stmts1,v_stmt) => {
          if (anode eq v_stmt) return v_stmts_env(v_stmts0);
        }
        case _ => {}
      }
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".stmt_env");
  }
  def c_expr_env(anode : T_Expr) : T_Environment = {
    val anchor = anode.parent;
    if (!(anchor eq null)) {
      val anchorNodes = anchor.myType.nodes;
      if (anchorNodes == t_Result.t_Stmt.nodes) anchor match {
        case p_assign_stmt(v_stmt,v_expr1,v_expr2) => {
          if (anode eq v_expr1) return v_stmt_env(v_stmt);
          if (anode eq v_expr2) return v_stmt_env(v_stmt);
        }
        case _ => {}
      }
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".expr_env");
  }
  def c_block_used(anode : T_Block) : T_Used = {
    val anchor = anode;
    anchor match {
      case p_block(v_block,v_decls,v_stmts) => {
        if (anode eq v_block) return v_decls_uout(v_decls);
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".block_used");
  }
  def c_decls_uin(anode : T_Decls) : T_Used = {
    val anchor = anode.parent;
    if (!(anchor eq null)) {
      val anchorNodes = anchor.myType.nodes;
      if (anchorNodes == t_Result.t_Block.nodes) anchor match {
        case p_block(v_block,v_decls,v_stmts) => {
          if (anode eq v_decls) return v_stmts_used(v_stmts);
        }
        case _ => {}
      }
      if (anchorNodes == t_Result.t_Decls.nodes) anchor match {
        case p_xcons_decls(v_decls0,v_decls1,v_decl) => {
          if (anode eq v_decls1) return v_decl_uout(v_decl);
        }
        case _ => {}
      }
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decls_uin");
  }
  def c_decls_uout(anode : T_Decls) : T_Used = {
    val anchor = anode;
    anchor match {
      case p_no_decls(v_decls) => {
        if (anode eq v_decls) return v_decls_uin(v_decls);
      }
      case _ => {}
    }
    anchor match {
      case p_xcons_decls(v_decls0,v_decls1,v_decl) => {
        if (anode eq v_decls0) return v_decls_uout(v_decls1);
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decls_uout");
  }
  def c_decl_uin(anode : T_Decl) : T_Used = {
    val anchor = anode.parent;
    if (!(anchor eq null)) {
      val anchorNodes = anchor.myType.nodes;
      if (anchorNodes == t_Result.t_Decls.nodes) anchor match {
        case p_xcons_decls(v_decls0,v_decls1,v_decl) => {
          if (anode eq v_decl) return v_decls_uin(v_decls0);
        }
        case _ => {}
      }
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decl_uin");
  }
  def c_decl_uout(anode : T_Decl) : T_Used = {
    val anchor = anode;
    anchor match {
      case p_decl(v_d,v_id,v_ty) => {
        { val cond = new M__basic_14[ T_String,T_Used](t_String,t_Used).v_in(v_id,v_decl_uin(v_d));
          if (cond) {
            if (anode eq v_d) return new M__basic_20[ T_String,T_Used](t_String,t_Used).v__op_5(v_decl_uin(v_d),v_id);
          }
          if (!cond) {
            if (anode eq v_d) return v_decl_uin(v_d);
          }
        }
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decl_uout");
  }
  def c_stmts_used(anode : T_Stmts) : T_Used = {
    val anchor = anode;
    anchor match {
      case p_no_stmts(v_stmts) => {
        if (anode eq v_stmts) return t_Used.v_none();
      }
      case _ => {}
    }
    anchor match {
      case p_xcons_stmts(v_stmts0,v_stmts1,v_stmt) => {
        if (anode eq v_stmts0) return new M__basic_19[ T_String,T_Used](t_String,t_Used).v__op_5w(v_stmts_used(v_stmts1),v_stmt_used(v_stmt));
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".stmts_used");
  }
  def c_stmt_used(anode : T_Stmt) : T_Used = {
    val anchor = anode;
    anchor match {
      case p_block_stmt(v_stmt,v_block) => {
        if (anode eq v_stmt) return v_block_used(v_block);
      }
      case _ => {}
    }
    anchor match {
      case p_assign_stmt(v_stmt,v_expr1,v_expr2) => {
        if (anode eq v_stmt) return new M__basic_19[ T_String,T_Used](t_String,t_Used).v__op_5w(v_expr_used(v_expr1),v_expr_used(v_expr2));
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".stmt_used");
  }
  def c_expr_used(anode : T_Expr) : T_Used = {
    val anchor = anode;
    anchor match {
      case p_intconstant(v_e,_) => {
        if (anode eq v_e) return t_Used.v_none();
      }
      case _ => {}
    }
    anchor match {
      case p_strconstant(v_e,_) => {
        if (anode eq v_e) return t_Used.v_none();
      }
      case _ => {}
    }
    anchor match {
      case p_variable(v_expr,v_id) => {
        if (anode eq v_expr) return t_Used.v_single(v_id);
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".expr_used");
  }
  def c_prog_msgs(anode : T_Program) : T_Messages = {
    val anchor = anode;
    anchor match {
      case p_program(v_p,v_b) => {
        if (anode eq v_p) return v_block_msgs(v_b);
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".prog_msgs");
  }
  def c_block_msgs(anode : T_Block) : T_Messages = {
    val anchor = anode;
    anchor match {
      case p_block(v_block,v_decls,v_stmts) => {
        if (anode eq v_block) return t_Messages.v_append(v_decls_msgs(v_decls),v_stmts_msgs(v_stmts));
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".block_msgs");
  }
  def c_decls_msgs(anode : T_Decls) : T_Messages = {
    val anchor = anode;
    anchor match {
      case p_no_decls(v_decls) => {
        if (anode eq v_decls) return t_Messages.v_none();
      }
      case _ => {}
    }
    anchor match {
      case p_xcons_decls(v_decls0,v_decls1,v_decl) => {
        if (anode eq v_decls0) return t_Messages.v_append(v_decls_msgs(v_decls1),v_decl_msgs(v_decl));
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decls_msgs");
  }
  def c_decl_msgs(anode : T_Decl) : T_Messages = {
    val anchor = anode;
    anchor match {
      case p_decl(v_d,v_id,v_ty) => {
        { val cond = new M__basic_14[ T_String,T_Used](t_String,t_Used).v_in(v_id,v_decl_uin(v_d));
          if (cond) {
            if (anode eq v_d) return t_Messages.v_none();
          }
          if (!cond) {
            if (anode eq v_d) return t_Messages.v_single(new M__basic_18[ T_String](t_String).v__op_ss(v_id," is unused"));
          }
        }
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".decl_msgs");
  }
  def c_stmts_msgs(anode : T_Stmts) : T_Messages = {
    val anchor = anode;
    anchor match {
      case p_no_stmts(v_stmts) => {
        if (anode eq v_stmts) return t_Messages.v_none();
      }
      case _ => {}
    }
    anchor match {
      case p_xcons_stmts(v_stmts0,v_stmts1,v_stmt) => {
        if (anode eq v_stmts0) return v__op_nn(v_stmts_msgs(v_stmts1),v_stmt_msgs(v_stmt));
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".stmts_msgs");
  }
  def c_stmt_msgs(anode : T_Stmt) : T_Messages = {
    val anchor = anode;
    anchor match {
      case p_block_stmt(v_stmt,v_block) => {
        if (anode eq v_stmt) return v_block_msgs(v_block);
      }
      case _ => {}
    }
    anchor match {
      case p_assign_stmt(v_stmt,v_expr1,v_expr2) => {
        { val cond = new M__basic_2[ T_Shape](t_Shape).v__op_w0(v_expr_shape(v_expr1),v_expr_shape(v_expr2));
          if (cond) {
            if (anode eq v_stmt) return v__op_nn(t_Messages.v_single("type mismatch"),v__op_nn(v_expr_msgs(v_expr1),v_expr_msgs(v_expr2)));
          }
          if (!cond) {
            if (anode eq v_stmt) return v__op_nn(v_expr_msgs(v_expr1),v_expr_msgs(v_expr2));
          }
        }
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".stmt_msgs");
  }
  def c_expr_msgs(anode : T_Expr) : T_Messages = {
    val anchor = anode;
    anchor match {
      case p_intconstant(v_e,_) => {
        if (anode eq v_e) return t_Messages.v_none();
      }
      case _ => {}
    }
    anchor match {
      case p_strconstant(v_e,_) => {
        if (anode eq v_e) return t_Messages.v_none();
      }
      case _ => {}
    }
    anchor match {
      case p_variable(v_expr,v_id) => {
        { val cond = new M__basic_2[ T_Shape](t_Shape).v__op_0(a1_sh.get(anchor),v_not_found);
          if (cond) {
            if (anode eq v_expr) return t_Messages.v_single(new M__basic_18[ T_String](t_String).v__op_ss(v_id," not declared"));
          }
          if (!cond) {
            if (anode eq v_expr) return t_Messages.v_none();
          }
        }
      }
      case _ => {}
    }
    throw Evaluation.UndefinedAttributeException(anode.toString()+".expr_msgs");
  }
  override def finish() : Unit = {
    a_type_shape.finish;
    a_expr_shape.finish;
    a_block_env.finish;
    a_decls_envin.finish;
    a_decls_envout.finish;
    a_decl_envin.finish;
    a_decl_envout.finish;
    a_stmts_env.finish;
    a_stmt_env.finish;
    a_expr_env.finish;
    a_block_used.finish;
    a_decls_uin.finish;
    a_decls_uout.finish;
    a_decl_uin.finish;
    a_decl_uout.finish;
    a_stmts_used.finish;
    a_stmt_used.finish;
    a_expr_used.finish;
    a_prog_msgs.finish;
    a_block_msgs.finish;
    a_decls_msgs.finish;
    a_decl_msgs.finish;
    a_stmts_msgs.finish;
    a_stmt_msgs.finish;
    a_expr_msgs.finish;
    super.finish();
  }

}
