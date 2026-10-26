class SimpleParserBase {
  def get_line_number() : Int = 0;

  def set_node_numbers() : Unit = {
    PARSE.lineNumber = get_line_number()
  };
  
  object m_Tree extends M_SIMPLE("SimpleTree") {};
  val t_Tree = m_Tree.t_Result;
  type T_Tree = m_Tree.T_Result;

  def getTree() : M_SIMPLE = m_Tree;
  
  type Program = t_Tree.T_Program;
  type Block = t_Tree.T_Block;
  type Decl = t_Tree.T_Decl;
  type Decls = t_Tree.T_Decls;
  type Type = t_Tree.T_Type;
  type Stmt = t_Tree.T_Stmt;
  type Stmts = t_Tree.T_Stmts;
  type Expr = t_Tree.T_Expr;

  def program(b : Block) : Program = {
    set_node_numbers();
    var n = t_Tree.v_program(b);
    n
  };

  def block(ds: Decls, ss : Stmts) : Block = {
    set_node_numbers();
    var n = t_Tree.v_block(ds,ss);
    n
  };
  def no_decls() : Decls = {
    set_node_numbers();
    var n = t_Tree.v_no_decls();
    n
  };
  def xcons_decls(ds : Decls, d : Decl) : Decls = {
    set_node_numbers();
    var n = t_Tree.v_xcons_decls(ds,d);
    n
  };
  def decl(id : String, ty : Type) : Decl = {
    set_node_numbers();
    var n = t_Tree.v_decl(id,ty);
    n
  };
  def integer_type() : Type = {
    set_node_numbers();
    var n = t_Tree.v_integer_type();
    n
  };
  def string_type() : Type = {
    set_node_numbers();
    var n = t_Tree.v_string_type();
    n
  };
  def no_stmts() : Stmts = {
    set_node_numbers();
    var n = t_Tree.v_no_stmts();
    n
  };
  def xcons_stmts(ss : Stmts, s : Stmt) : Stmts = {
    set_node_numbers();
    var n = t_Tree.v_xcons_stmts(ss,s);
    n
  };
  def block_stmt(block : Block) : Stmt = {
    set_node_numbers();
    var n = t_Tree.v_block_stmt(block);
    n
  };
  def assign_stmt(e1:Expr, e2 : Expr) : Stmt = {
    set_node_numbers();
    var n = t_Tree.v_assign_stmt(e1,e2);
    n
  };
  def intconstant(i : Integer) : Expr = {
    set_node_numbers();
    var n = t_Tree.v_intconstant(i);
    n
  };
  def strconstant(s : String) : Expr = {
    set_node_numbers();
    var n = t_Tree.v_strconstant(s);
    n
  };
  def variable(id : String) : Expr = {
    set_node_numbers();
    var n = t_Tree.v_variable(id);
    n
  };

}
