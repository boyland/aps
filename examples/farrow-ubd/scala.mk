SOURCES.farrow-ubd-tree = farrow-ubd-tree.aps

SOURCES.farrow-ubd = farrow-ubd-tree.aps farrow-ubd.aps farrow-ubd-driver.scala
PARSER_BASENAME.farrow-ubd = FarrowUbd
LEXER.farrow-ubd = farrow-ubd.lex
PARSER.farrow-ubd = farrow-ubd.y
DRIVER.FarrowUbdDriver = farrow-ubd.aps farrow-ubd-driver.scala

SOURCES.farrow-ubd-fiber = farrow-ubd-tree.aps farrow-ubd-fiber.aps farrow-ubd-fiber-driver.scala
PARSER_BASENAME.farrow-ubd-fiber = FarrowUbd
LEXER.farrow-ubd-fiber = farrow-ubd.lex
PARSER.farrow-ubd-fiber = farrow-ubd.y
DRIVER.FarrowUbdFiberDriver = farrow-ubd-fiber.aps farrow-ubd-fiber-driver.scala

DRIVERS += FarrowUbdDriver FarrowUbdFiberDriver
