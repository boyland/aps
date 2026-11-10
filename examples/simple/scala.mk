PARSER_PREFIXES += Simple
EVALUATOR_EXAMPLES += classic-binding nested-cycles simple-snc simple-snc-fiber \
	simple-binding simple-binding1 simple-binding2 simple-binding3
SIMPLE_EVALUATOR_CLASSES = ${OUT}/classic_binding_implicit.class ${OUT}/nested_cycles_implicit.class \
	${OUT}/simple_snc_implicit.class ${OUT}/simple_snc_fiber_implicit.class ${OUT}/simple_binding_implicit.class
EVALUATOR_CLASSES += ${SIMPLE_EVALUATOR_CLASSES}
DRIVER_CLASSES += ${OUT}/Classic.class ${OUT}/NestedCyclesDriver.class ${OUT}/SimpleSncDriver.class \
	${OUT}/SimpleSncFiberDriver.class ${OUT}/SimpleBindingDriver.class
BINDING_VARIANT_CLASSES = SimpleBinding1Driver.class SimpleBinding2Driver.class SimpleBinding3Driver.class
BINDING_VARIANT_RUNS = SimpleBinding1Driver.run SimpleBinding2Driver.run SimpleBinding3Driver.run
BINDING_VARIANT_OUTPUTS = ${OUT}/simple-binding1/SimpleBindingDriver.class \
	${OUT}/simple-binding2/SimpleBindingDriver.class ${OUT}/simple-binding3/SimpleBindingDriver.class

Simple.lex: simple.lex Simple.lex.scala
Simple.y: simple.y Simple.y.scala

${SIMPLE_EVALUATOR_CLASSES}: ${OUT}/simple_implicit.class
${OUT}/classic_binding_implicit.class: classic-binding${IMPL_SUFFIX}
${OUT}/nested_cycles_implicit.class: nested-cycles${IMPL_SUFFIX}
${OUT}/simple_snc_implicit.class: simple-snc${IMPL_SUFFIX}
${OUT}/simple_snc_fiber_implicit.class: simple-snc-fiber${IMPL_SUFFIX}
${OUT}/simple_binding_implicit.class: simple-binding${IMPL_SUFFIX}
${OUT}/SimpleParserBase.class: ${OUT}/simple_implicit.class
${OUT}/Classic.class: classic-driver.scala ${OUT}/classic_binding_implicit.class ${OUT}/SimpleParser.class
${OUT}/NestedCyclesDriver.class: nested-cycles-driver.scala ${OUT}/nested_cycles_implicit.class ${OUT}/SimpleParser.class
${OUT}/SimpleSncDriver.class: simple-snc-driver.scala ${OUT}/simple_snc_implicit.class ${OUT}/SimpleParser.class
${OUT}/SimpleSncFiberDriver.class: simple-snc-fiber-driver.scala ${OUT}/simple_snc_fiber_implicit.class ${OUT}/SimpleParser.class
${OUT}/SimpleBindingDriver.class: simple-binding-driver.scala ${OUT}/simple_binding_implicit.class ${OUT}/SimpleParser.class

.PHONY: ${BINDING_VARIANT_CLASSES} ${BINDING_VARIANT_RUNS}
${BINDING_VARIANT_CLASSES}: SimpleBinding%Driver.class: ${OUT}/simple-binding%/SimpleBindingDriver.class ;

${BINDING_VARIANT_OUTPUTS}: ${OUT}/simple-binding%/SimpleBindingDriver.class: simple.scala simple-binding%${IMPL_SUFFIX} simple-binding-driver.scala ${OUT}/SimpleParser.class
	rm -rf $(@D)
	mkdir -p $(@D)
	${SCALAC} -cp $(@D):${OUT}:.:${APSLIB} -d $(@D) $(filter %.scala,$^)

${BINDING_VARIANT_RUNS}: SimpleBinding%Driver.run: ${OUT}/simple-binding%/SimpleBindingDriver.class
	${SCALA} ${JVMFLAGS} -cp ${OUT}/simple-binding$*:${OUT}:.:${APSLIB} SimpleBindingDriver ${ARGS}

SimpleBindingDriver.run: ${OUT}/SimpleBindingDriver.class
	${SCALA} ${JVMFLAGS} -cp ${OUT}:.:${APSLIB} SimpleBindingDriver ${ARGS}
