include ../farrow-lv/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST})

${OUT}/farrow-lv/%: SHARED_SOURCES := ${AST}
${OUT}/farrow-lv/%: PARSER_BASENAME := FarrowLv
${OUT}/farrow-lv/%: LEXER := farrow-lv.lex
${OUT}/farrow-lv/%: PARSER := farrow-lv.y

SOURCES.farrow-lv-tree := ${AST}

SOURCES.farrow-lv = farrow-lv.aps farrow-lv-driver.scala
DRIVER.FarrowLvDriver = farrow-lv.aps

DRIVERS += FarrowLvDriver
