DRIVER_CLASSES += FarrowLvDriver.class
PARSER_LEXER_FarrowLv = farrow-lv.lex FarrowLv.lex.scala
PARSER_GRAMMAR_FarrowLv = farrow-lv.y FarrowLv.y.scala

SOURCES_farrow-lv = farrow-lv-tree.aps farrow-lv.aps farrow-lv-driver.scala
PARSER_farrow-lv = FarrowLv
FarrowLvDriver.class FarrowLvDriver.run: MODULE = farrow-lv
