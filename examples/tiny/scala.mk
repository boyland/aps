include ../tiny/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST_SOURCES})

TINY_OUTPUTS := ${OUT}/tiny/% ${OUT}/broad-fiber-cycle/% ${OUT}/below-fiber-cycle/% \
	${OUT}/below-single-fiber-cycle/% ${OUT}/local-fiber-cycle/% ${OUT}/test-coll/% \
	${OUT}/test-use-coll/% ${OUT}/test-cycle/% ${OUT}/use-global/% ${OUT}/test-fields/% \
	${OUT}/test-for/% ${OUT}/test-for-patterns/% ${OUT}/test-case-patterns/% \
	${OUT}/test-nested-patterns/% ${OUT}/test-finish/%
${TINY_OUTPUTS}: SHARED_SOURCES := ${AST_SOURCES} tiny-parser.handcode.scala

SOURCES.tiny := ${AST_SOURCES}

SOURCES.broad-fiber-cycle = broad-fiber-cycle.aps broad-fiber-cycle-driver.scala
DRIVER.BroadFiberCycleDriver = broad-fiber-cycle.aps broad-fiber-cycle-driver.scala

SOURCES.below-fiber-cycle = below-fiber-cycle.aps below-fiber-cycle-driver.scala
DRIVER.BelowFiberCycleDriver = below-fiber-cycle.aps below-fiber-cycle-driver.scala

SOURCES.below-single-fiber-cycle = below-single-fiber-cycle.aps below-single-fiber-cycle-driver.scala
DRIVER.BelowSingleFiberCycleDriver = below-single-fiber-cycle.aps below-single-fiber-cycle-driver.scala

SOURCES.local-fiber-cycle = local-fiber-cycle.aps local-fiber-cycle-driver.scala
DRIVER.LocalFiberCycleDriver = local-fiber-cycle.aps local-fiber-cycle-driver.scala

SOURCES.test-coll = test-coll.aps test-coll-driver.scala
DRIVER.TestCollDriver = test-coll.aps test-coll-driver.scala

SOURCES.test-use-coll = test-use-coll.aps test-use-coll-driver.scala
DRIVER.TestUseCollDriver = test-use-coll.aps test-use-coll-driver.scala

SOURCES.test-cycle = test-cycle.aps test-cycle-driver.scala
DRIVER.TestCycleDriver = test-cycle.aps test-cycle-driver.scala

SOURCES.use-global = use-global.aps use-global-driver.scala
DRIVER.UseGlobal = use-global.aps use-global-driver.scala

SOURCES.test-fields = test-fields.aps test-fields-driver.scala
DRIVER.TestFieldsDriver = test-fields.aps test-fields-driver.scala

SOURCES.test-for = test-for.aps test-for-driver.scala
DRIVER.TestForDriver = test-for.aps test-for-driver.scala

SOURCES.test-for-patterns = test-for-patterns.aps test-for-patterns-driver.scala
DRIVER.TestForPatternsDriver = test-for-patterns.aps test-for-patterns-driver.scala

SOURCES.test-case-patterns = test-case-patterns.aps test-case-patterns-driver.scala
DRIVER.TestCasePatternsDriver = test-case-patterns.aps test-case-patterns-driver.scala

SOURCES.test-nested-patterns = test-nested-patterns.aps test-nested-patterns-driver.scala
DRIVER.TestNestedPatternsDriver = test-nested-patterns.aps test-nested-patterns-driver.scala

SOURCES.test-finish = test-finish.aps finish-driver.scala
DRIVER.FinishDriver = test-finish.aps finish-driver.scala

DRIVERS += BroadFiberCycleDriver BelowFiberCycleDriver BelowSingleFiberCycleDriver \
	LocalFiberCycleDriver TestCollDriver TestUseCollDriver TestCycleDriver UseGlobal \
	TestFieldsDriver TestForDriver TestForPatternsDriver TestCasePatternsDriver \
	TestNestedPatternsDriver FinishDriver
