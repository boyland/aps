include ../farrow-lv/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST_SOURCES})

${OUT}/farrow-lv/% ${OUT}/farrow-lv-tree/%: MODULE_CONFIG := ../farrow-lv/scala.mk ../farrow-lv/ast.mk
${OUT}/farrow-lv/%: SHARED_SOURCES := ${AST_SOURCES}
${OUT}/farrow-lv/%: PARSER_BASENAME := FarrowLv
${OUT}/farrow-lv/%: LEXER := farrow-lv.lex
${OUT}/farrow-lv/%: PARSER := farrow-lv.y

SOURCES.farrow-lv-tree := ${AST_SOURCES}

SOURCES.farrow-lv = farrow-lv.aps farrow-lv-driver.scala
DRIVER.FarrowLvDriver = farrow-lv.aps

DRIVERS += FarrowLvDriver
