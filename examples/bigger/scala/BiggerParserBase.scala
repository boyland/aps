class BiggerParserBase {
  def get_line_number() : Int = 0;

  def set_node_numbers() : Unit = {
    PARSE.lineNumber = get_line_number()
  };

  object m_Tree extends M_BIGGER("BiggerTree") {};
  val t_Tree = m_Tree.t_Result;
  type T_Tree = m_Tree.T_Result;

  def getTree() : M_BIGGER = m_Tree;

  type Program = t_Tree.T_Program;
  type Block = t_Tree.T_Block;
  type Decls = t_Tree.T_Decls;
  type Decl = t_Tree.T_Decl;
  type Type = t_Tree.T_Type;
  type Stmts = t_Tree.T_Stmts;
  type Stmt = t_Tree.T_Stmt;
  type Expr = t_Tree.T_Expr;

  def program(b : Block) : Program = {
    set_node_numbers();
    t_Tree.v_program(b)
  };

  def block(ds : Decls, ss : Stmts) : Block = {
    set_node_numbers();
    t_Tree.v_block(ds, ss)
  };

  def no_decls() : Decls = {
    set_node_numbers();
    t_Tree.v_no_decls()
  };

  def xcons_decls(ds : Decls, d : Decl) : Decls = {
    set_node_numbers();
    t_Tree.v_xcons_decls(ds, d)
  };

  def decl(id : String, ty : Type) : Decl = {
    set_node_numbers();
    t_Tree.v_decl(id, ty)
  };

  def integer() : Type = {
    set_node_numbers();
    t_Tree.v_integer()
  };

  def string() : Type = {
    set_node_numbers();
    t_Tree.v_string_type()
  };

  def boolean() : Type = {
    set_node_numbers();
    t_Tree.v_boolean()
  };

  def array(element : Type) : Type = {
    set_node_numbers();
    t_Tree.v_array(element)
  };

  def no_stmts() : Stmts = {
    set_node_numbers();
    t_Tree.v_no_stmts()
  };

  def xcons_stmts(ss : Stmts, s : Stmt) : Stmts = {
    set_node_numbers();
    t_Tree.v_xcons_stmts(ss, s)
  };

  def block_stmt(b : Block) : Stmt = {
    set_node_numbers();
    t_Tree.v_block_stmt(b)
  };

  def assign_stmt(e1 : Expr, e2 : Expr) : Stmt = {
    set_node_numbers();
    t_Tree.v_assign_stmt(e1, e2)
  };

  def if_stmt(cond : Expr, s1 : Stmt, s2 : Stmt) : Stmt = {
    set_node_numbers();
    t_Tree.v_if_stmt(cond, s1, s2)
  };

  def while_stmt(cond : Expr, body : Stmt) : Stmt = {
    set_node_numbers();
    t_Tree.v_while_stmt(cond, body)
  };

  def less(e1 : Expr, e2 : Expr) : Expr = {
    set_node_numbers();
    t_Tree.v_less(e1, e2)
  };

  def equalf(e1 : Expr, e2 : Expr) : Expr = {
    set_node_numbers();
    t_Tree.v_equalf(e1, e2)
  };

  def plus(e1 : Expr, e2 : Expr) : Expr = {
    set_node_numbers();
    t_Tree.v_plus(e1, e2)
  };

  def concat(e1 : Expr, e2 : Expr) : Expr = {
    set_node_numbers();
    t_Tree.v_concat(e1, e2)
  };

  def logor(e1 : Expr, e2 : Expr) : Expr = {
    set_node_numbers();
    t_Tree.v_logor(e1, e2)
  };

  def substring(s : Expr, i1 : Expr, i2 : Expr) : Expr = {
    set_node_numbers();
    t_Tree.v_substring(s, i1, i2)
  };

  def array_ref(a : Expr, index : Expr) : Expr = {
    set_node_numbers();
    t_Tree.v_array_ref(a, index)
  };

  def intconstant(i : Integer) : Expr = {
    set_node_numbers();
    t_Tree.v_intconstant(i)
  };

  def strconstant(s : String) : Expr = {
    set_node_numbers();
    t_Tree.v_strconstant(s)
  };

  def boolconstant(b : Boolean) : Expr = {
    set_node_numbers();
    t_Tree.v_boolconstant(b)
  };

  def variable(id : String) : Expr = {
    set_node_numbers();
    t_Tree.v_variable(id)
  };
}
