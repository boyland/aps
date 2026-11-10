DRIVER_CLASSES += BiggerBindingDriver.class
PARSER_LEXER_Bigger = bigger.lex Bigger.lex.scala
PARSER_GRAMMAR_Bigger = bigger.y Bigger.y.scala

SOURCES_bigger-binding = bigger.aps bigger-binding.aps bigger-binding-driver.scala
PARSER_bigger-binding = Bigger
BiggerBindingDriver.class BiggerBindingDriver.run: MODULE = bigger-binding
