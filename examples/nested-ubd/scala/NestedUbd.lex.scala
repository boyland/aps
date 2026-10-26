%%

%class NestedUbdScanner
%type NestedUbdTokens.YYToken
%implements Iterator[NestedUbdTokens.YYToken]

%line

%{
  var lookahead : NestedUbdTokens.YYToken = null;
   
  override def hasNext() : Boolean = { 
    if (null == lookahead) lookahead = yylex();
    lookahead match {
      case x:NestedUbdTokens.YYEOF => false;
      case x:NestedUbdTokens.YYToken => true;
    }
  };
  
  override def next() : NestedUbdTokens.YYToken = {
    if (null == lookahead) lookahead = yylex();
    var result : NestedUbdTokens.YYToken = lookahead;
    lookahead = null;
    result
  };
  
  def getLineNumber() : Int = yyline+1;

  def YYCHAR(s : String) = NestedUbdTokens.YYCHAR(s.charAt(0));
  def ID(s : String) = NestedUbdTokens.ID(Symbol(s));
  def LITERAL(s : String) = NestedUbdTokens.LITERAL(Symbol(s));
  def EQ = NestedUbdTokens.EQ();
  def SEMICOLON = NestedUbdTokens.SEMICOLON();
  def PLUS = NestedUbdTokens.PLUS();
  def MINUS = NestedUbdTokens.MINUS();
  def MUL = NestedUbdTokens.MUL();
  def DIV = NestedUbdTokens.DIV();
  def OPN_BRACE = NestedUbdTokens.OPN_BRACE();
  def CLS_BRACE = NestedUbdTokens.CLS_BRACE();
  def YYEOFT = NestedUbdTokens.YYEOF();

%}
