class GrammarParserBase {
  def get_line_number() : Int = 0;

  def set_node_numbers() : Unit = {
    PARSE.lineNumber = get_line_number()
  };
  
  object m_Tree extends M_GRAMMAR("GrammarTree") {};
  val t_Tree = m_Tree.t_Result;
  type T_Tree = m_Tree.T_Result;

  def getTree() : M_GRAMMAR = m_Tree;
  
  type Item = t_Tree.T_Item;
  type Items = t_Tree.T_Items;
  type Production = t_Tree.T_Production;
  type Productions = t_Tree.T_Productions;
  type Grammar = t_Tree.T_Grammar;
  type ItemsMany = Seq[Items];

  def grammar(prods : Productions) : Grammar = {
    set_node_numbers();
    var n = t_Tree.v_grammar(prods);
    n
  };

  def prod(nt: Symbol, rhs: Items) : Production = {
    set_node_numbers();
    var n = t_Tree.v_prod(nt, rhs);
    n
  };

  def prods(nt: Symbol, rhs_many: Seq[Items]) : Productions = {
    set_node_numbers();
    var n = rhs_many
      .map(prod(nt, _))
      .map(productions_single(_))
      .fold(productions_none())(productions_append(_, _));
    n
  };
  def nonterminal(s: Symbol) : Item = {
    set_node_numbers();
    var n = t_Tree.v_nonterminal(s);
    n
  };
  def terminal(s: Symbol) : Item = {
    set_node_numbers();
    var n = t_Tree.v_terminal(s);
    n
  };

  def productions_none() : Productions = {
    set_node_numbers();
    var n = t_Tree.t_Productions.v_none();
    n
  };

  def productions_single(v1: Production) : Productions = {
    set_node_numbers();
    var n = t_Tree.t_Productions.v_single(v1);
    n
  };

  def productions_append(v1: Productions, v2: Productions) : Productions = {
    set_node_numbers();
    var n = t_Tree.t_Productions.v_append(v1, v2);
    n
  };

  def items_none() : Items = {
    set_node_numbers();
    var n = t_Tree.t_Items.v_none();
    n
  };

  def items_single(v1: Item) : Items = {
    set_node_numbers();
    var n = t_Tree.t_Items.v_single(v1);
    n
  };

  def items_append(v1: Items, v2: Items) : Items = {
    set_node_numbers();
    var n = t_Tree.t_Items.v_append(v1, v2);
    n
  };

  def items_many_single(v1: Items) : ItemsMany = {
    Seq(v1)
  }

  def items_many_append(v1: ItemsMany, v2: ItemsMany) : ItemsMany = {
    v1 ++ v2
  }

}
