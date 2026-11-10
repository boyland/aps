PARSER_PREFIXES += FarrowUbd
EVALUATOR_EXAMPLES += farrow-ubd farrow-ubd-fiber
EVALUATOR_CLASSES += ${OUT}/FarrowUbd_implicit.class ${OUT}/FarrowUbdFiber_implicit.class
DRIVER_CLASSES += ${OUT}/FarrowUbdDriver.class ${OUT}/FarrowUbdFiberDriver.class

FarrowUbd.lex: farrow-ubd.lex FarrowUbd.lex.scala
FarrowUbd.y: farrow-ubd.y FarrowUbd.y.scala

${OUT}/FarrowUbd_implicit.class: farrow-ubd${IMPL_SUFFIX} farrow-ubd-tree.scala
${OUT}/FarrowUbdFiber_implicit.class: farrow-ubd-fiber${IMPL_SUFFIX} farrow-ubd-tree.scala
${OUT}/FarrowUbdParserBase.class: ${OUT}/FarrowUbd_implicit.class
${OUT}/FarrowUbdDriver.class: farrow-ubd-driver.scala ${OUT}/FarrowUbd_implicit.class ${OUT}/FarrowUbdParser.class
${OUT}/FarrowUbdFiberDriver.class: farrow-ubd-fiber-driver.scala ${OUT}/FarrowUbdFiber_implicit.class ${OUT}/FarrowUbdParser.class
