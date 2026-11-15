include ../bigger/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST_SOURCES})

${OUT}/bigger-binding/%: SHARED_SOURCES := ${AST_SOURCES}
${OUT}/bigger-binding/%: PARSER_BASENAME := Bigger
${OUT}/bigger-binding/%: LEXER := bigger.lex
${OUT}/bigger-binding/%: PARSER := bigger.y

SOURCES.bigger := ${AST_SOURCES}

SOURCES.bigger-binding = bigger-binding.aps bigger-binding-driver.scala
DRIVER.BiggerBindingDriver = bigger-binding.aps bigger-binding-driver.scala

DRIVERS += BiggerBindingDriver
