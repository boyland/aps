PARSERS += Bigger
EVALUATOR_EXAMPLES += bigger-binding
EVALUATOR_CLASSES += ${OUT}/bigger_binding_implicit.class
ALL_CLASSES += ${OUT}/BiggerBindingDriver.class

Bigger.lex: bigger.lex Bigger.lex.scala
Bigger.y: bigger.y Bigger.y.scala

${OUT}/bigger_binding_implicit.class: bigger-binding${IMPL_SUFFIX} ${OUT}/bigger_implicit.class
${OUT}/BiggerParserBase.class: ${OUT}/bigger_implicit.class
${OUT}/BiggerBindingDriver.class: bigger-binding-driver.scala ${OUT}/bigger_binding_implicit.class ${OUT}/BiggerParser.class
