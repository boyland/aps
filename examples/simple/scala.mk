SOURCES.simple = simple.aps

SOURCES.classic-binding = simple.aps classic-binding.aps classic-driver.scala
PARSER_BASENAME.classic-binding = Simple
LEXER.classic-binding = simple.lex
PARSER.classic-binding = simple.y
DRIVER.Classic = classic-binding.aps classic-driver.scala

SOURCES.nested-cycles = simple.aps nested-cycles.aps nested-cycles-driver.scala
PARSER_BASENAME.nested-cycles = Simple
LEXER.nested-cycles = simple.lex
PARSER.nested-cycles = simple.y
DRIVER.NestedCyclesDriver = nested-cycles.aps nested-cycles-driver.scala

SOURCES.simple-snc = simple.aps simple-snc.aps simple-snc-driver.scala
PARSER_BASENAME.simple-snc = Simple
LEXER.simple-snc = simple.lex
PARSER.simple-snc = simple.y
DRIVER.SimpleSncDriver = simple-snc.aps simple-snc-driver.scala

SOURCES.simple-snc-fiber = simple.aps simple-snc-fiber.aps simple-snc-fiber-driver.scala
PARSER_BASENAME.simple-snc-fiber = Simple
LEXER.simple-snc-fiber = simple.lex
PARSER.simple-snc-fiber = simple.y
DRIVER.SimpleSncFiberDriver = simple-snc-fiber.aps simple-snc-fiber-driver.scala

SOURCES.simple-binding = simple.aps simple-binding.aps simple-binding-driver.scala
PARSER_BASENAME.simple-binding = Simple
LEXER.simple-binding = simple.lex
PARSER.simple-binding = simple.y
DRIVER.SimpleBindingDriver = simple-binding.aps simple-binding-driver.scala

SOURCES.simple-binding1 = simple.aps simple-binding1.aps simple-binding-driver.scala
PARSER_BASENAME.simple-binding1 = Simple
LEXER.simple-binding1 = simple.lex
PARSER.simple-binding1 = simple.y
DRIVER.SimpleBinding1Driver = simple-binding1.aps simple-binding-driver.scala SimpleBindingDriver

SOURCES.simple-binding2 = simple.aps simple-binding2.aps simple-binding-driver.scala
PARSER_BASENAME.simple-binding2 = Simple
LEXER.simple-binding2 = simple.lex
PARSER.simple-binding2 = simple.y
DRIVER.SimpleBinding2Driver = simple-binding2.aps simple-binding-driver.scala SimpleBindingDriver

SOURCES.simple-binding3 = simple.aps simple-binding3.aps simple-binding-driver.scala
PARSER_BASENAME.simple-binding3 = Simple
LEXER.simple-binding3 = simple.lex
PARSER.simple-binding3 = simple.y
DRIVER.SimpleBinding3Driver = simple-binding3.aps simple-binding-driver.scala SimpleBindingDriver

DRIVERS += Classic NestedCyclesDriver SimpleSncDriver SimpleSncFiberDriver \
	SimpleBindingDriver SimpleBinding1Driver SimpleBinding2Driver SimpleBinding3Driver
