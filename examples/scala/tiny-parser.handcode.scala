import java.util.StringTokenizer;

/**
 * Class to parse "tiny" trees from strings of comma-separated
 * integer values, left associative with parens as needed.
 * For example: <tt>1,2,(3,4)</tt> which is the same as <tt>(1,2),(3,4)</tt>
 */
class TinyParser(m_tree : M_TINY) {
	val t_Tree = m_tree.t_Result;

	def asRoot(s : String) : t_Tree.T_Root = {
      		val tok = new StringTokenizer(s,"(,)",true);
		parseRoot(tok)
	};

	def parseRoot(t : StringTokenizer) : t_Tree.T_Root =
		t_Tree.f_root(parseWood(t));

	def parseWood(t : StringTokenizer) : t_Tree.T_Wood = {
		val tok = t.nextToken();
		var start : t_Tree.T_Wood = null;
		if (tok == "(") {
			start = parseWood(t)
		} else {
			start = t_Tree.f_leaf(tok.toInt)
		};
		while (t.hasMoreTokens() && t.nextToken() == ",") {
			start = t_Tree.f_branch(start,parseWood(t))
		};
		start
	};
}
