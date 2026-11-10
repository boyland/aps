EVALUATOR_EXAMPLES += broad-fiber-cycle below-fiber-cycle below-single-fiber-cycle \
	local-fiber-cycle test-coll test-use-coll test-cycle use-global test-fields \
	test-for test-for-patterns test-case-patterns test-nested-patterns test-finish
TINY_EVALUATOR_CLASSES = $(addprefix ${OUT}/,broad_fiber_cycle_implicit.class \
	below_fiber_cycle_implicit.class below_single_fiber_cycle_implicit.class \
	local_fiber_cycle_implicit.class test_coll_implicit.class test_use_coll_implicit.class \
	test_cycle_implicit.class use_global_implicit.class test_fields_implicit.class \
	test_for_implicit.class test_for_patterns_implicit.class test_case_patterns_implicit.class \
	test_nested_patterns_implicit.class)
EVALUATOR_CLASSES += ${TINY_EVALUATOR_CLASSES} ${OUT}/test_finish_implicit.class
TINY_DRIVER_CLASSES = $(addprefix ${OUT}/,BroadFiberCycleDriver.class BelowFiberCycleDriver.class \
	BelowSingleFiberCycleDriver.class LocalFiberCycleDriver.class TestCollDriver.class \
	TestUseCollDriver.class TestCycleDriver.class UseGlobal.class TestFieldsDriver.class \
	TestForDriver.class TestForPatternsDriver.class TestCasePatternsDriver.class TestNestedPatternsDriver.class)
ALL_CLASSES += ${TINY_DRIVER_CLASSES} ${OUT}/TinyParser.class ${OUT}/FinishDriver.class

${TINY_EVALUATOR_CLASSES} ${OUT}/TinyParser.class: ${OUT}/tiny_implicit.class
${TINY_DRIVER_CLASSES}: ${OUT}/TinyParser.class

${OUT}/broad_fiber_cycle_implicit.class: broad-fiber-cycle${IMPL_SUFFIX}
${OUT}/below_fiber_cycle_implicit.class: below-fiber-cycle${IMPL_SUFFIX}
${OUT}/below_single_fiber_cycle_implicit.class: below-single-fiber-cycle${IMPL_SUFFIX}
${OUT}/local_fiber_cycle_implicit.class: local-fiber-cycle${IMPL_SUFFIX}
${OUT}/test_coll_implicit.class: test-coll${IMPL_SUFFIX}
${OUT}/test_use_coll_implicit.class: test-use-coll${IMPL_SUFFIX}
${OUT}/test_cycle_implicit.class: test-cycle${IMPL_SUFFIX}
${OUT}/use_global_implicit.class: use-global${IMPL_SUFFIX}
${OUT}/test_fields_implicit.class: test-fields${IMPL_SUFFIX}
${OUT}/test_for_implicit.class: test-for${IMPL_SUFFIX}
${OUT}/test_for_patterns_implicit.class: test-for-patterns${IMPL_SUFFIX}
${OUT}/test_case_patterns_implicit.class: test-case-patterns${IMPL_SUFFIX}
${OUT}/test_nested_patterns_implicit.class: test-nested-patterns${IMPL_SUFFIX}
${OUT}/test_finish_implicit.class: test-finish${IMPL_SUFFIX}

${OUT}/TinyParser.class: tiny-parser.handcode.scala
${OUT}/BroadFiberCycleDriver.class: broad-fiber-cycle-driver.scala ${OUT}/broad_fiber_cycle_implicit.class
${OUT}/BelowFiberCycleDriver.class: below-fiber-cycle-driver.scala ${OUT}/below_fiber_cycle_implicit.class
${OUT}/BelowSingleFiberCycleDriver.class: below-single-fiber-cycle-driver.scala ${OUT}/below_single_fiber_cycle_implicit.class
${OUT}/LocalFiberCycleDriver.class: local-fiber-cycle-driver.scala ${OUT}/local_fiber_cycle_implicit.class
${OUT}/TestCollDriver.class: test-coll-driver.scala ${OUT}/test_coll_implicit.class
${OUT}/TestUseCollDriver.class: test-use-coll-driver.scala ${OUT}/test_use_coll_implicit.class
${OUT}/TestCycleDriver.class: test-cycle-driver.scala ${OUT}/test_cycle_implicit.class
${OUT}/UseGlobal.class: use-global-driver.scala ${OUT}/use_global_implicit.class
${OUT}/TestFieldsDriver.class: test-fields-driver.scala ${OUT}/test_fields_implicit.class
${OUT}/TestForDriver.class: test-for-driver.scala ${OUT}/test_for_implicit.class
${OUT}/TestForPatternsDriver.class: test-for-patterns-driver.scala ${OUT}/test_for_patterns_implicit.class
${OUT}/TestCasePatternsDriver.class: test-case-patterns-driver.scala ${OUT}/test_case_patterns_implicit.class
${OUT}/TestNestedPatternsDriver.class: test-nested-patterns-driver.scala ${OUT}/test_nested_patterns_implicit.class
${OUT}/FinishDriver.class: finish-driver.scala ${OUT}/test_finish_implicit.class
