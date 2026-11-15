SOURCES.bigger = bigger.aps

SOURCES.bigger-binding = bigger.aps bigger-binding.aps bigger-binding-driver.scala
PARSER_BASENAME.bigger-binding = Bigger
LEXER.bigger-binding = bigger.lex
PARSER.bigger-binding = bigger.y
DRIVER.BiggerBindingDriver = bigger-binding.aps bigger-binding-driver.scala

DRIVERS += BiggerBindingDriver
