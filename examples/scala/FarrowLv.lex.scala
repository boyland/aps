%%

%class FarrowLvScanner
%type FarrowLvTokens.YYToken
%implements Iterator[FarrowLvTokens.YYToken]

%line

%{
  var lookahead : FarrowLvTokens.YYToken = null;
   
  override def hasNext() : Boolean = { 
    if (null == lookahead) lookahead = yylex();
    lookahead match {
      case x:FarrowLvTokens.YYEOF => false;
      case x:FarrowLvTokens.YYToken => true;
    }
  };
  
  override def next() : FarrowLvTokens.YYToken = {
    if (null == lookahead) lookahead = yylex();
    var result : FarrowLvTokens.YYToken = lookahead;
    lookahead = null;
    result
  };
  
  def getLineNumber() : Int = yyline+1;

  def YYCHAR(s : String) = FarrowLvTokens.YYCHAR(s.charAt(0));
  def ID(s : String) = FarrowLvTokens.ID(Symbol(s));
  def LITERAL(s : String) = FarrowLvTokens.LITERAL(Symbol(s));
  def EQ = FarrowLvTokens.EQ();
  def EQEQ = FarrowLvTokens.EQEQ();
  def NEQ = FarrowLvTokens.NEQ();
  def LT = FarrowLvTokens.LT();
  def SEMICOLON = FarrowLvTokens.SEMICOLON();
  def WHILE = FarrowLvTokens.WHILE();
  def IF = FarrowLvTokens.IF();
  def THEN = FarrowLvTokens.THEN();
  def ELSE = FarrowLvTokens.ELSE();
  def DO = FarrowLvTokens.DO();
  def END = FarrowLvTokens.END();
  def PLUS = FarrowLvTokens.PLUS();
  def MINUS = FarrowLvTokens.MINUS();
  def YYEOFT = FarrowLvTokens.YYEOF();

%}
