include ../farrow-ubd/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST})

FARROW_UBD_OUTPUTS := ${OUT}/farrow-ubd/% ${OUT}/farrow-ubd-fiber/%
${FARROW_UBD_OUTPUTS}: SHARED_SOURCES := ${AST}
${FARROW_UBD_OUTPUTS}: PARSER_BASENAME := FarrowUbd
${FARROW_UBD_OUTPUTS}: LEXER := farrow-ubd.lex
${FARROW_UBD_OUTPUTS}: PARSER := farrow-ubd.y

SOURCES.farrow-ubd-tree := ${AST}

SOURCES.farrow-ubd = farrow-ubd.aps farrow-ubd-driver.scala
DRIVER.FarrowUbdDriver = farrow-ubd.aps

SOURCES.farrow-ubd-fiber = farrow-ubd-fiber.aps farrow-ubd-fiber-driver.scala
DRIVER.FarrowUbdFiberDriver = farrow-ubd-fiber.aps

DRIVERS += FarrowUbdDriver FarrowUbdFiberDriver
