%%

%class GrammarScanner
%type GrammarTokens.YYToken
%implements Iterator[GrammarTokens.YYToken]

%line

%{
  var lookahead : GrammarTokens.YYToken = null;
   
  override def hasNext() : Boolean = { 
    if (null == lookahead) lookahead = yylex();
    lookahead match {
      case x:GrammarTokens.YYEOF => false;
      case x:GrammarTokens.YYToken => true;
    }
  };
  
  override def next() : GrammarTokens.YYToken = {
    if (null == lookahead) lookahead = yylex();
    var result : GrammarTokens.YYToken = lookahead;
    lookahead = null;
    result
  };
  
  def getLineNumber() : Int = yyline+1;

  def YYCHAR(s : String) = GrammarTokens.YYCHAR(s.charAt(0));
  def TERMINAL(s : String) = GrammarTokens.TERMINAL(Symbol(s));
  def NONTERMINAL(s : String) = GrammarTokens.NONTERMINAL(Symbol(s));
  def PIPE = GrammarTokens.PIPE();
  def COLON = GrammarTokens.COLON();
  def SEMICOLON = GrammarTokens.SEMICOLON();
  def YYEOFT = GrammarTokens.YYEOF();

%}
