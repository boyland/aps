PARSERS += NestedUbd
EVALUATOR_EXAMPLES += nested-ubd nested-ubd-fiber
EVALUATOR_CLASSES += ${OUT}/NestedUbd_implicit.class ${OUT}/NestedUbdFiber_implicit.class
ALL_CLASSES += ${OUT}/NestedUbdDriver.class ${OUT}/NestedUbdFiberDriver.class

NestedUbd.lex: nested-ubd.lex NestedUbd.lex.scala
NestedUbd.y: nested-ubd.y NestedUbd.y.scala

${OUT}/NestedUbd_implicit.class: nested-ubd${IMPL_SUFFIX} nested-ubd-tree.scala
${OUT}/NestedUbdFiber_implicit.class: nested-ubd-fiber${IMPL_SUFFIX} nested-ubd-tree.scala
${OUT}/NestedUbdParserBase.class: ${OUT}/NestedUbd_implicit.class
${OUT}/NestedUbdDriver.class: nested-ubd-driver.scala ${OUT}/NestedUbd_implicit.class ${OUT}/NestedUbdParser.class
${OUT}/NestedUbdFiberDriver.class: nested-ubd-fiber-driver.scala ${OUT}/NestedUbdFiber_implicit.class ${OUT}/NestedUbdParser.class
