include ../grammar/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST_SOURCES})

GRAMMAR_OUTPUTS := ${OUT}/grammar/% ${OUT}/first/% ${OUT}/follow/% ${OUT}/nullable/%
${GRAMMAR_OUTPUTS}: MODULE_CONFIG := ../grammar/scala.mk ../grammar/ast.mk
${GRAMMAR_OUTPUTS}: SHARED_SOURCES := ${AST_SOURCES} first.aps follow.aps nullable.aps
${GRAMMAR_OUTPUTS}: PARSER_BASENAME := Grammar
${GRAMMAR_OUTPUTS}: LEXER := grammar.lex
${GRAMMAR_OUTPUTS}: PARSER := grammar.y

SOURCES.grammar := ${AST_SOURCES} first-driver.scala follow-driver.scala nullable-driver.scala

SOURCES.first = first.aps
DRIVER.FirstDriver = grammar.aps

SOURCES.follow = follow.aps
DRIVER.FollowDriver = grammar.aps

SOURCES.nullable = nullable.aps
DRIVER.NullableDriver = grammar.aps

DRIVERS += FirstDriver FollowDriver NullableDriver
