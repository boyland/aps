class NestedUbdParserBase {
  def get_line_number() : Int = 0;

  def set_node_numbers() : Unit = {
    PARSE.lineNumber = get_line_number()
  };
  
  object m_Tree extends M_NESTED_UBD_TREE("NestedUbdTree") {};
  val t_Tree = m_Tree.t_Result;
  type T_Tree = m_Tree.T_Result;

  def getTree() : M_NESTED_UBD_TREE = m_Tree;
  
  type Block = t_Tree.T_Block;
  type Declaration = t_Tree.T_Declaration;
  type Declarations = t_Tree.T_Declarations;
  type Expression = t_Tree.T_Expression;
  type Term = t_Tree.T_Term;
  type Operation = t_Tree.T_Operation;
  type Program = t_Tree.T_Program;

  def program(b : Block) : Program = {
    set_node_numbers();
    var n = t_Tree.v_program(b);
    n
  };

  def scope(ds: Declarations) : Block = {
    set_node_numbers();
    var n = t_Tree.v_scope(ds);
    n
  };

  def decl_assign(s: Symbol, e: Expression) : Declaration = {
    set_node_numbers();
    var n = t_Tree.v_decl_assign(s, e);
    n
  };

  def decls_empty() : Declarations = {
    set_node_numbers();
    var n = t_Tree.v_decls_empty();
    n
  };

  def decls_append(ds: Declarations, d: Declaration) : Declarations = {
    set_node_numbers();
    var n = t_Tree.v_decls_append(ds, d);
    n
  };

  def decls_add(ds: Declarations, b: Block) : Declarations = {
    set_node_numbers();
    var n = t_Tree.v_decls_add(ds, b);
    n
  };

  def expr_term(t: Term) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_term(t);
    n
  };

  def op_add() : Operation = {
    set_node_numbers();
    var n = t_Tree.v_op_add();
    n
  };

  def op_mul() : Operation = {
    set_node_numbers();
    var n = t_Tree.v_op_mul();
    n
  };

  def op_sub() : Operation = {
    set_node_numbers();
    var n = t_Tree.v_op_sub();
    n
  };

  def op_div() : Operation = {
    set_node_numbers();
    var n = t_Tree.v_op_div();
    n
  };

  def expr_apply(e: Expression, op: Operation, t: Term) : Expression = {
    set_node_numbers();
    var n = t_Tree.v_expr_apply(e, op, t);
    n
  };

  def term_variable(s: Symbol) : Term = {
    set_node_numbers();
    var n = t_Tree.v_term_variable(s);
    n
  };

  def term_literal(s: Symbol) : Term = {
    set_node_numbers();
    var n = t_Tree.v_term_literal(java.lang.Integer.parseInt(s.name));
    n
  };
}
