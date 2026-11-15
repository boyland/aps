SOURCES.tiny = tiny.aps tiny-parser.handcode.scala

SOURCES.broad-fiber-cycle = tiny.aps broad-fiber-cycle.aps tiny-parser.handcode.scala broad-fiber-cycle-driver.scala
DRIVER.BroadFiberCycleDriver = broad-fiber-cycle.aps broad-fiber-cycle-driver.scala

SOURCES.below-fiber-cycle = tiny.aps below-fiber-cycle.aps tiny-parser.handcode.scala below-fiber-cycle-driver.scala
DRIVER.BelowFiberCycleDriver = below-fiber-cycle.aps below-fiber-cycle-driver.scala

SOURCES.below-single-fiber-cycle = tiny.aps below-single-fiber-cycle.aps tiny-parser.handcode.scala below-single-fiber-cycle-driver.scala
DRIVER.BelowSingleFiberCycleDriver = below-single-fiber-cycle.aps below-single-fiber-cycle-driver.scala

SOURCES.local-fiber-cycle = tiny.aps local-fiber-cycle.aps tiny-parser.handcode.scala local-fiber-cycle-driver.scala
DRIVER.LocalFiberCycleDriver = local-fiber-cycle.aps local-fiber-cycle-driver.scala

SOURCES.test-coll = tiny.aps test-coll.aps tiny-parser.handcode.scala test-coll-driver.scala
DRIVER.TestCollDriver = test-coll.aps test-coll-driver.scala

SOURCES.test-use-coll = tiny.aps test-use-coll.aps tiny-parser.handcode.scala test-use-coll-driver.scala
DRIVER.TestUseCollDriver = test-use-coll.aps test-use-coll-driver.scala

SOURCES.test-cycle = tiny.aps test-cycle.aps tiny-parser.handcode.scala test-cycle-driver.scala
DRIVER.TestCycleDriver = test-cycle.aps test-cycle-driver.scala

SOURCES.use-global = tiny.aps use-global.aps tiny-parser.handcode.scala use-global-driver.scala
DRIVER.UseGlobal = use-global.aps use-global-driver.scala

SOURCES.test-fields = tiny.aps test-fields.aps tiny-parser.handcode.scala test-fields-driver.scala
DRIVER.TestFieldsDriver = test-fields.aps test-fields-driver.scala

SOURCES.test-for = tiny.aps test-for.aps tiny-parser.handcode.scala test-for-driver.scala
DRIVER.TestForDriver = test-for.aps test-for-driver.scala

SOURCES.test-for-patterns = tiny.aps test-for-patterns.aps tiny-parser.handcode.scala test-for-patterns-driver.scala
DRIVER.TestForPatternsDriver = test-for-patterns.aps test-for-patterns-driver.scala

SOURCES.test-case-patterns = tiny.aps test-case-patterns.aps tiny-parser.handcode.scala test-case-patterns-driver.scala
DRIVER.TestCasePatternsDriver = test-case-patterns.aps test-case-patterns-driver.scala

SOURCES.test-nested-patterns = tiny.aps test-nested-patterns.aps tiny-parser.handcode.scala test-nested-patterns-driver.scala
DRIVER.TestNestedPatternsDriver = test-nested-patterns.aps test-nested-patterns-driver.scala

SOURCES.test-finish = tiny.aps test-finish.aps tiny-parser.handcode.scala finish-driver.scala
DRIVER.FinishDriver = test-finish.aps finish-driver.scala

DRIVERS += BroadFiberCycleDriver BelowFiberCycleDriver BelowSingleFiberCycleDriver \
	LocalFiberCycleDriver TestCollDriver TestUseCollDriver TestCycleDriver UseGlobal \
	TestFieldsDriver TestForDriver TestForPatternsDriver TestCasePatternsDriver \
	TestNestedPatternsDriver FinishDriver
