DRIVER_CLASSES += FarrowUbdDriver.class FarrowUbdFiberDriver.class
PARSER_LEXER_FarrowUbd = farrow-ubd.lex FarrowUbd.lex.scala
PARSER_GRAMMAR_FarrowUbd = farrow-ubd.y FarrowUbd.y.scala

SOURCES_farrow-ubd = farrow-ubd-tree.aps farrow-ubd.aps farrow-ubd-driver.scala
SOURCES_farrow-ubd-fiber = farrow-ubd-tree.aps farrow-ubd-fiber.aps farrow-ubd-fiber-driver.scala
PARSER_farrow-ubd = FarrowUbd
PARSER_farrow-ubd-fiber = FarrowUbd
FarrowUbdDriver.class FarrowUbdDriver.run: MODULE = farrow-ubd
FarrowUbdFiberDriver.class FarrowUbdFiberDriver.run: MODULE = farrow-ubd-fiber
