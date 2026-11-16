var scanner : BiggerScanner = null;
var filename : String = "<unknown>";
var result : Program = null;

def get_result() : Program = result;

def reset(sc : BiggerScanner, fn : String) : Unit = {
  filename = fn;
  scanner = sc;
  yyreset(sc)
};

override def get_line_number() : Int = scanner.getLineNumber();

def yyerror(message : String) : Unit = {
  print(filename + ":" + scanner.getLineNumber() + ": " +
        message + ", at or near " + yycur + "\n");
};

override def program(b : Block) : Program = {
  val res = super.program(b);
  result = res;
  res
}
