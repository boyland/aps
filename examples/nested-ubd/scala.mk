SOURCES.nested-ubd-tree = nested-ubd-tree.aps

SOURCES.nested-ubd = nested-ubd-tree.aps nested-ubd.aps nested-ubd-driver.scala
PARSER_BASENAME.nested-ubd = NestedUbd
LEXER.nested-ubd = nested-ubd.lex
PARSER.nested-ubd = nested-ubd.y
DRIVER.NestedUbdDriver = nested-ubd.aps nested-ubd-driver.scala

SOURCES.nested-ubd-fiber = nested-ubd-tree.aps nested-ubd-fiber.aps nested-ubd-fiber-driver.scala
PARSER_BASENAME.nested-ubd-fiber = NestedUbd
LEXER.nested-ubd-fiber = nested-ubd.lex
PARSER.nested-ubd-fiber = nested-ubd.y
DRIVER.NestedUbdFiberDriver = nested-ubd-fiber.aps nested-ubd-fiber-driver.scala

DRIVERS += NestedUbdDriver NestedUbdFiberDriver
