include ../farrow-lv/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST_SOURCES})

${OUT}/farrow-lv/%: SHARED_SOURCES := ${AST_SOURCES}
${OUT}/farrow-lv/%: PARSER_BASENAME := FarrowLv
${OUT}/farrow-lv/%: LEXER := farrow-lv.lex
${OUT}/farrow-lv/%: PARSER := farrow-lv.y

SOURCES.farrow-lv-tree := ${AST_SOURCES}

SOURCES.farrow-lv = farrow-lv.aps farrow-lv-driver.scala
DRIVER.FarrowLvDriver = farrow-lv.aps farrow-lv-driver.scala

DRIVERS += FarrowLvDriver
