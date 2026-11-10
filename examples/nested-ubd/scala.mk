DRIVER_CLASSES += NestedUbdDriver.class NestedUbdFiberDriver.class
PARSER_LEXER_NestedUbd = nested-ubd.lex NestedUbd.lex.scala
PARSER_GRAMMAR_NestedUbd = nested-ubd.y NestedUbd.y.scala

SOURCES_nested-ubd = nested-ubd-tree.aps nested-ubd.aps nested-ubd-driver.scala
SOURCES_nested-ubd-fiber = nested-ubd-tree.aps nested-ubd-fiber.aps nested-ubd-fiber-driver.scala
PARSER_nested-ubd = NestedUbd
PARSER_nested-ubd-fiber = NestedUbd
NestedUbdDriver.class NestedUbdDriver.run: MODULE = nested-ubd
NestedUbdFiberDriver.class NestedUbdFiberDriver.run: MODULE = nested-ubd-fiber
