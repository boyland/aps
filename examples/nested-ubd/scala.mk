include ../nested-ubd/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST})

NESTED_UBD_OUTPUTS := ${OUT}/nested-ubd/% ${OUT}/nested-ubd-fiber/%
${NESTED_UBD_OUTPUTS} ${OUT}/nested-ubd-tree/%: MODULE_CONFIG := ../nested-ubd/scala.mk ../nested-ubd/ast.mk
${NESTED_UBD_OUTPUTS}: SHARED_SOURCES := ${AST}
${NESTED_UBD_OUTPUTS}: PARSER_BASENAME := NestedUbd
${NESTED_UBD_OUTPUTS}: LEXER := nested-ubd.lex
${NESTED_UBD_OUTPUTS}: PARSER := nested-ubd.y

SOURCES.nested-ubd-tree := ${AST}

SOURCES.nested-ubd = nested-ubd.aps nested-ubd-driver.scala
DRIVER.NestedUbdDriver = nested-ubd.aps

SOURCES.nested-ubd-fiber = nested-ubd-fiber.aps nested-ubd-fiber-driver.scala
DRIVER.NestedUbdFiberDriver = nested-ubd-fiber.aps

DRIVERS += NestedUbdDriver NestedUbdFiberDriver
