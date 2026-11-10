var scanner : FarrowUbdScanner = null;
var filename : String = "<unknown>";
var result : Program = null;

def get_result() : Program = result;

def reset(sc : FarrowUbdScanner, fn : String) : Unit = {
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

override def program(ds:Declarations) : Program = {
  var res = super.program(ds);
  result = res;
  res
}
