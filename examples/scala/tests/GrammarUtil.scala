import scala.reflect.{ClassTag, classTag}

object GrammarUtil {

  def createItems(ls: List[String])(implicit grammar: M_GRAMMAR, symb: M_SYMBOL, map: List[(String, List[String])]): grammar.t_Items.T_Result = {
    implicit val m = grammar.t_Items
    asAPSSequence(ls.map(x => {
      if (map.exists { case (nt, _) => nt == x })
        grammar.v_nonterminal(symb.v_create(x))
      else
        grammar.v_terminal(symb.v_create(x))
    }))
  }

  def createProductions(ls: List[(String, List[String])])(implicit grammar: M_GRAMMAR, symb: M_SYMBOL): grammar.t_Productions.T_Result = {
    implicit val m = grammar.t_Productions
    asAPSSequence[grammar.t_Production.T_Result](ls.map { case (key, values) =>
      grammar.v_prod(symb.v_create(key), createItems(values)(grammar, symb, ls)
        .asInstanceOf[grammar.t_Items.T_Result])
    }.toList)
  }

  def asAPSSequence[T <: Node : ClassTag](ls: List[T])(implicit m: M_SEQUENCE[T]): m.T_Result = {
    ls match {
      case Nil => m.v_none()
      case x :: Nil => m.v_single(x)
      case (x: T) :: rest =>
        m.v_append(
          asAPSSequence(List(x)),
          asAPSSequence(rest))
    }
  }

  def buildGrammar(map: List[(String, List[String])]) = {
    implicit val symb = new M_SYMBOL("Symbol")
    symb.finish()

    implicit val grammar = new M_GRAMMAR("Grammar")
    val productions = createProductions(map).asInstanceOf[grammar.t_Productions.T_Result]
    grammar.v_grammar(productions)
    grammar.finish()

    grammar
  }
}
