#define yylineno aps_yylineno

extern int yylineno;

Symbol copy_Symbol(Symbol s) {
  return s;
}

String copy_String(String s) {
  return s;
}

Boolean copy_Boolean(Boolean b) {
  return b;
}

void assert_Symbol(Symbol s) {
}

void assert_String(String s) {
}

void assert_Boolean(Boolean b) {
}

