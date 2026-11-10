%%

%class FarrowUbdScanner
%type FarrowUbdTokens.YYToken
%implements Iterator[FarrowUbdTokens.YYToken]

%line

%{
  var lookahead : FarrowUbdTokens.YYToken = null;
   
  override def hasNext() : Boolean = { 
    if (null == lookahead) lookahead = yylex();
    lookahead match {
      case x:FarrowUbdTokens.YYEOF => false;
      case x:FarrowUbdTokens.YYToken => true;
    }
  };
  
  override def next() : FarrowUbdTokens.YYToken = {
    if (null == lookahead) lookahead = yylex();
    var result : FarrowUbdTokens.YYToken = lookahead;
    lookahead = null;
    result
  };
  
  def getLineNumber() : Int = yyline+1;

  def YYCHAR(s : String) = FarrowUbdTokens.YYCHAR(s.charAt(0));
  def ID(s : String) = FarrowUbdTokens.ID(Symbol(s));
  def LITERAL(s : String) = FarrowUbdTokens.LITERAL(Symbol(s));
  def EQ = FarrowUbdTokens.EQ();
  def SEMICOLON = FarrowUbdTokens.SEMICOLON();
  def PLUS = FarrowUbdTokens.PLUS();
  def MINUS = FarrowUbdTokens.MINUS();
  def MUL = FarrowUbdTokens.MUL();
  def DIV = FarrowUbdTokens.DIV();
  def OPN_BRACE = FarrowUbdTokens.OPN_BRACE();
  def CLS_BRACE = FarrowUbdTokens.CLS_BRACE();
  def YYEOFT = FarrowUbdTokens.YYEOF();

%}
