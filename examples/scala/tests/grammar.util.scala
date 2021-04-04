import scala.reflect.{ClassTag, classTag}

object GrammarUtil {

  def toItems(ls: List[String])(implicit grammar: M_GRAMMAR, symb: M_SYMBOL, map: List[(String, List[String])]): grammar.t_Items.T_Result = {
    implicit val m = grammar.t_Items
    toSequenceTree(ls.map(x => {
      if (map.exists { case (nt, _) => nt == x })
        grammar.v_nonterminal(symb.v_create(x))
      else
        grammar.v_terminal(symb.v_create(x))
    }))
  }

  def toProductions(ls: List[(String, List[String])])(implicit grammar: M_GRAMMAR, symb: M_SYMBOL): grammar.t_Productions.T_Result = {
    implicit val m = grammar.t_Productions
    toSequenceTree[grammar.t_Production.T_Result](ls.map { case (key, values) =>
      grammar.v_prod(symb.v_create(key), toItems(values)(grammar, symb, ls)
        .asInstanceOf[grammar.t_Items.T_Result])
    }.toList)
  }

  def toSequenceTree[T <: Node : ClassTag](ls: List[T])(implicit m: M_SEQUENCE[T]): m.T_Result = {
    ls match {
      case x :: Nil => m.v_single(x)
      case (x: T) :: rest =>
        m.v_append(
          toSequenceTree(List(x)),
          toSequenceTree(rest))
      case Nil => m.v_none()
    }
  }

  def toGrammar(map: List[(String, List[String])]) = {
    implicit val symb = new M_SYMBOL("Symbol")
    symb.finish()

    implicit val grammar = new M_GRAMMAR("Grammar")
    val productions = toProductions(map).asInstanceOf[grammar.t_Productions.T_Result]
    grammar.v_grammar(productions)
    grammar.finish()

    grammar
  }
}