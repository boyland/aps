SOURCES.farrow-lv-tree = farrow-lv-tree.aps

SOURCES.farrow-lv = farrow-lv-tree.aps farrow-lv.aps farrow-lv-driver.scala
PARSER_BASENAME.farrow-lv = FarrowLv
LEXER.farrow-lv = farrow-lv.lex
PARSER.farrow-lv = farrow-lv.y
DRIVER.FarrowLvDriver = farrow-lv.aps farrow-lv-driver.scala

DRIVERS += FarrowLvDriver
