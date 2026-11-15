include ../nested-ubd/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST_SOURCES})

NESTED_UBD_OUTPUTS := ${OUT}/nested-ubd/% ${OUT}/nested-ubd-fiber/%
${NESTED_UBD_OUTPUTS}: SHARED_SOURCES := ${AST_SOURCES}
${NESTED_UBD_OUTPUTS}: PARSER_BASENAME := NestedUbd
${NESTED_UBD_OUTPUTS}: LEXER := nested-ubd.lex
${NESTED_UBD_OUTPUTS}: PARSER := nested-ubd.y

SOURCES.nested-ubd-tree := ${AST_SOURCES}

SOURCES.nested-ubd = nested-ubd.aps nested-ubd-driver.scala
DRIVER.NestedUbdDriver = nested-ubd.aps nested-ubd-driver.scala

SOURCES.nested-ubd-fiber = nested-ubd-fiber.aps nested-ubd-fiber-driver.scala
DRIVER.NestedUbdFiberDriver = nested-ubd-fiber.aps nested-ubd-fiber-driver.scala

DRIVERS += NestedUbdDriver NestedUbdFiberDriver
