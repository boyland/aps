%%

%class BiggerScanner
%type BiggerTokens.YYToken
%implements Iterator[BiggerTokens.YYToken]

%line

%{
  var lookahead : BiggerTokens.YYToken = null;

  override def hasNext() : Boolean = {
    if (null == lookahead) lookahead = yylex();
    lookahead match {
      case _:BiggerTokens.YYEOF => false;
      case _:BiggerTokens.YYToken => true;
    }
  };

  override def next() : BiggerTokens.YYToken = {
    if (null == lookahead) lookahead = yylex();
    val result : BiggerTokens.YYToken = lookahead;
    lookahead = null;
    result
  };

  def getLineNumber() : Int = yyline + 1;

  def YYCHAR(s : String) = BiggerTokens.YYCHAR(s.charAt(0));
  def INTEGER = BiggerTokens.INTEGER();
  def STRING = BiggerTokens.STRING();
  def BOOLEAN = BiggerTokens.BOOLEAN();
  def ARRAY = BiggerTokens.ARRAY();
  def IF = BiggerTokens.IF();
  def ELSE = BiggerTokens.ELSE();
  def WHILE = BiggerTokens.WHILE();
  def SUBSTRING = BiggerTokens.SUBSTRING();
  def EQUAL = BiggerTokens.EQUAL();
  def CONCAT = BiggerTokens.CONCAT();
  def LOGOR = BiggerTokens.LOGOR();
  def ID(s : String) = BiggerTokens.ID(s);
  def INT_LITERAL(s : String) = BiggerTokens.INT_LITERAL(java.lang.Integer.parseInt(s));
  def STR_LITERAL(s : String) = BiggerTokens.STR_LITERAL(s);
  def BOOL_LITERAL(b : Boolean) = BiggerTokens.BOOL_LITERAL(b);
  def YYEOFT = BiggerTokens.YYEOF();
%}
