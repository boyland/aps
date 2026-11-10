PARSERS += FarrowLv
EVALUATOR_EXAMPLES += farrow-lv
EVALUATOR_CLASSES += ${OUT}/FarrowLv_implicit.class
DRIVER_CLASSES += ${OUT}/FarrowLvDriver.class

FarrowLv.lex: farrow-lv.lex FarrowLv.lex.scala
FarrowLv.y: farrow-lv.y FarrowLv.y.scala

${OUT}/FarrowLv_implicit.class: farrow-lv${IMPL_SUFFIX} farrow-lv-tree.scala
${OUT}/FarrowLvParserBase.class: ${OUT}/FarrowLv_implicit.class
${OUT}/FarrowLvDriver.class: farrow-lv-driver.scala ${OUT}/FarrowLv_implicit.class ${OUT}/FarrowLvParser.class
