PARSERS += Grammar
EVALUATOR_EXAMPLES += first follow nullable
EVALUATOR_CLASSES += ${OUT}/grammar_implicit.class
DRIVER_CLASSES += ${OUT}/FirstDriver.class ${OUT}/FollowDriver.class ${OUT}/NullableDriver.class

Grammar.lex: grammar.lex Grammar.lex.scala
Grammar.y: grammar.y Grammar.y.scala

${OUT}/grammar_implicit.class: grammar.scala first${IMPL_SUFFIX} follow${IMPL_SUFFIX} nullable${IMPL_SUFFIX}
${OUT}/GrammarParserBase.class: ${OUT}/grammar_implicit.class
${OUT}/FirstDriver.class: first-driver.scala ${OUT}/grammar_implicit.class ${OUT}/GrammarParser.class
${OUT}/FollowDriver.class: follow-driver.scala ${OUT}/grammar_implicit.class ${OUT}/GrammarParser.class
${OUT}/NullableDriver.class: nullable-driver.scala ${OUT}/grammar_implicit.class ${OUT}/GrammarParser.class
