var scanner : SimpleScanner = null;
var filename : String = "<unknown>";
var result : Program = null;

def get_result() : Program = result;

def reset(sc : SimpleScanner, fn : String) : Unit = {
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

override def program(b:Block) : Program = {
  var res = super.program(b);
  result = res;
  res
}
