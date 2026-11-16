SHARED_SOURCES = tiny-parser.handcode.scala

DRIVERS = BroadFiberCycleDriver BelowFiberCycleDriver BelowSingleFiberCycleDriver \
	LocalFiberCycleDriver TestCollDriver TestUseCollDriver TestCycleDriver UseGlobal \
	TestFieldsDriver TestForDriver TestForPatternsDriver TestCasePatternsDriver \
	TestNestedPatternsDriver FinishDriver

DRIVER.BroadFiberCycleDriver = broad-fiber-cycle.aps DYNAMIC,STATIC broad-fiber-cycle-driver.scala

DRIVER.BelowFiberCycleDriver = below-fiber-cycle.aps DYNAMIC,STATIC below-fiber-cycle-driver.scala

DRIVER.BelowSingleFiberCycleDriver = below-single-fiber-cycle.aps DYNAMIC,STATIC below-single-fiber-cycle-driver.scala

DRIVER.LocalFiberCycleDriver = local-fiber-cycle.aps DYNAMIC,STATIC local-fiber-cycle-driver.scala

DRIVER.TestCollDriver = test-coll.aps DYNAMIC,STATIC test-coll-driver.scala

DRIVER.TestUseCollDriver = test-use-coll.aps DYNAMIC,STATIC test-use-coll-driver.scala

DRIVER.TestCycleDriver = test-cycle.aps DYNAMIC,STATIC test-cycle-driver.scala

DRIVER.UseGlobal = use-global.aps DYNAMIC,STATIC use-global-driver.scala

DRIVER.TestFieldsDriver = test-fields.aps DYNAMIC,STATIC test-fields-driver.scala

DRIVER.TestForDriver = test-for.aps DYNAMIC,STATIC test-for-driver.scala

DRIVER.TestForPatternsDriver = test-for-patterns.aps DYNAMIC,STATIC test-for-patterns-driver.scala

DRIVER.TestCasePatternsDriver = test-case-patterns.aps DYNAMIC,STATIC test-case-patterns-driver.scala

DRIVER.TestNestedPatternsDriver = test-nested-patterns.aps DYNAMIC,STATIC test-nested-patterns-driver.scala

DRIVER.FinishDriver = test-finish.aps DYNAMIC,STATIC finish-driver.scala
