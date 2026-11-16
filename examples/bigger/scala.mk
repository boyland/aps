include ../bigger/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST})

${OUT}/bigger-binding/%: SHARED_SOURCES := ${AST}
${OUT}/bigger-binding/%: PARSER_BASENAME := Bigger
${OUT}/bigger-binding/%: LEXER := bigger.lex
${OUT}/bigger-binding/%: PARSER := bigger.y

SOURCES.bigger := ${AST}

SOURCES.bigger-binding = bigger-binding.aps bigger-binding-driver.scala
DRIVER.BiggerBindingDriver = bigger-binding.aps

DRIVERS += BiggerBindingDriver
