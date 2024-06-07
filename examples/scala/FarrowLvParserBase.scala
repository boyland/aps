class FarrowLvParserBase {
  def get_line_number() : Int = 0;

  def set_node_numbers() : Unit = {
    PARSE.lineNumber = get_line_number()
  };
  
  object m_Tree extends M_FARROW_LV_TREE("FarrowLvTree") {};
  val t_Tree = m_Tree.t_Result;
  type T_Tree = m_Tree.T_Result;

  def getTree() : M_FARROW_LV_TREE = m_Tree;
  
  type Stmt = t_Tree.T_Stmt;
  type Stmts = t_Tree.T_Stmts;
  type Expression = t_Tree.T_Expression;
  type Program = t_Tree.T_Program;

  def program(ss : Stmts) : Program = {
    set_node_numbers();
    var n = t_Tree.v_program(ss);
    n
  };

  def stmt_assign(s: Symbol, e: Expression) : Stmt = {
    set_node_numbers();
    var n = t_Tree.v_stmt_assign(s, e);
    n
  };

  def stmt_if(e: Expression, s1: Stmts, s2: Stmts) : Stmt = {
    set_node_numbers();
    var n = t_Tree.v_stmt_if(e, s1, s2);
    n
  };

  def stmt_while(e: Expression, s: Stmts) : Stmt = {
    set_node_numbers();
    var n = t_Tree.v_stmt_while(e, s);
    n
  };

  def stmts_append(s: Stmt, ss: Stmts) : Stmts = {
    set_node_numbers();
    var n = t_Tree.v_stmts_append(s, ss);
    n
  };

  def stmts_empty() : Stmts = {
    set_node_numbers();
    var n = t_Tree.v_stmts_empty();
    n
  };

  def expr_var(s: Symbol) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_var(s);
    n
  };

  def expr_add(e1: Expression, e2: Expression) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_add(e1, e2);
    n
  };

  def expr_subtract(e1: Expression, e2: Expression) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_subtract(e1, e2);
    n
  };

  def expr_equals(e1: Expression, e2: Expression) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_equals(e1, e2);
    n
  };

  def expr_not_equals(e1: Expression, e2: Expression) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_not_equals(e1, e2);
    n
  };

  def expr_less_than(e1: Expression, e2: Expression) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_less_than(e1, e2);
    n
  };

  def expr_lit(s: Symbol) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_lit(s);
    n
  };
}
