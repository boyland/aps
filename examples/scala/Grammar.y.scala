var scanner : GrammarScanner = null;
var filename : String = "<unknown>";
var result : Grammar = null;

def get_result() : Grammar = result;

def reset(sc : GrammarScanner, fn : String) : Unit = {
  filename = fn;
  scanner = sc;
  yyreset(sc)
};

override def get_line_number() : Int = scanner.getLineNumber();

/* This function is called automatically when Bison detects a parse error. */
def yyerror(message : String) : Unit = {
  print (filename + ":" + scanner.getLineNumber() + ": " +
	     message + ", at or near " + yycur + "\n");
};

override def grammar(b:Productions) : Grammar = {
  var res = super.grammar(b);
  result = res;
  res
}
