DRIVER_CLASSES += BroadFiberCycleDriver.class BelowFiberCycleDriver.class BelowSingleFiberCycleDriver.class \
	LocalFiberCycleDriver.class TestCollDriver.class TestUseCollDriver.class TestCycleDriver.class \
	UseGlobal.class TestFieldsDriver.class TestForDriver.class TestForPatternsDriver.class \
	TestCasePatternsDriver.class TestNestedPatternsDriver.class FinishDriver.class

SOURCES_broad-fiber-cycle = tiny.aps broad-fiber-cycle.aps tiny-parser.handcode.scala broad-fiber-cycle-driver.scala
SOURCES_below-fiber-cycle = tiny.aps below-fiber-cycle.aps tiny-parser.handcode.scala below-fiber-cycle-driver.scala
SOURCES_below-single-fiber-cycle = tiny.aps below-single-fiber-cycle.aps tiny-parser.handcode.scala below-single-fiber-cycle-driver.scala
SOURCES_local-fiber-cycle = tiny.aps local-fiber-cycle.aps tiny-parser.handcode.scala local-fiber-cycle-driver.scala
SOURCES_test-coll = tiny.aps test-coll.aps tiny-parser.handcode.scala test-coll-driver.scala
SOURCES_test-use-coll = tiny.aps test-use-coll.aps tiny-parser.handcode.scala test-use-coll-driver.scala
SOURCES_test-cycle = tiny.aps test-cycle.aps tiny-parser.handcode.scala test-cycle-driver.scala
SOURCES_use-global = tiny.aps use-global.aps tiny-parser.handcode.scala use-global-driver.scala
SOURCES_test-fields = tiny.aps test-fields.aps tiny-parser.handcode.scala test-fields-driver.scala
SOURCES_test-for = tiny.aps test-for.aps tiny-parser.handcode.scala test-for-driver.scala
SOURCES_test-for-patterns = tiny.aps test-for-patterns.aps tiny-parser.handcode.scala test-for-patterns-driver.scala
SOURCES_test-case-patterns = tiny.aps test-case-patterns.aps tiny-parser.handcode.scala test-case-patterns-driver.scala
SOURCES_test-nested-patterns = tiny.aps test-nested-patterns.aps tiny-parser.handcode.scala test-nested-patterns-driver.scala
SOURCES_test-finish = tiny.aps test-finish.aps finish-driver.scala

BroadFiberCycleDriver.class BroadFiberCycleDriver.run: MODULE = broad-fiber-cycle
BelowFiberCycleDriver.class BelowFiberCycleDriver.run: MODULE = below-fiber-cycle
BelowSingleFiberCycleDriver.class BelowSingleFiberCycleDriver.run: MODULE = below-single-fiber-cycle
LocalFiberCycleDriver.class LocalFiberCycleDriver.run: MODULE = local-fiber-cycle
TestCollDriver.class TestCollDriver.run: MODULE = test-coll
TestUseCollDriver.class TestUseCollDriver.run: MODULE = test-use-coll
TestCycleDriver.class TestCycleDriver.run: MODULE = test-cycle
UseGlobal.class UseGlobal.run: MODULE = use-global
TestFieldsDriver.class TestFieldsDriver.run: MODULE = test-fields
TestForDriver.class TestForDriver.run: MODULE = test-for
TestForPatternsDriver.class TestForPatternsDriver.run: MODULE = test-for-patterns
TestCasePatternsDriver.class TestCasePatternsDriver.run: MODULE = test-case-patterns
TestNestedPatternsDriver.class TestNestedPatternsDriver.run: MODULE = test-nested-patterns
FinishDriver.class FinishDriver.run: MODULE = test-finish
