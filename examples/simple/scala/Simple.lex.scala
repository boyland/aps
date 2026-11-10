%%

%class SimpleScanner
%type SimpleTokens.YYToken
%implements Iterator[SimpleTokens.YYToken]

%line

%{
  var lookahead : SimpleTokens.YYToken = null;
   
  override def hasNext() : Boolean = { 
    if (null == lookahead) lookahead = yylex();
    lookahead match {
      case x:SimpleTokens.YYEOF => false;
      case x:SimpleTokens.YYToken => true;
    }
  };
  
  override def next() : SimpleTokens.YYToken = {
    if (null == lookahead) lookahead = yylex();
    var result : SimpleTokens.YYToken = lookahead;
    lookahead = null;
    result
  };
  
  def getLineNumber() : Int = yyline+1;

  def YYCHAR(s : String) = SimpleTokens.YYCHAR(s.charAt(0));
  def INT = SimpleTokens.INT();
  def STRING = SimpleTokens.STRING();
  def ID(s : String) = SimpleTokens.ID(s);
  def INT_LITERAL(i:String) = SimpleTokens.INT_LITERAL(Integer.parseInt(i));
  def STR_LITERAL(s:String) = SimpleTokens.STR_LITERAL(s);
  def YYEOFT = SimpleTokens.YYEOF();
  
%}
