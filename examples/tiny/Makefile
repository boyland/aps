AST_MODULES = tiny ntuple-lattice
SCALA_DRIVERS = BroadFiberCycleDriver BelowFiberCycleDriver BelowSingleFiberCycleDriver \
	LocalFiberCycleDriver TestCollDriver TestUseCollDriver TestCycleDriver UseGlobal \
	TestFieldsDriver TestForDriver TestForPatternsDriver TestCasePatternsDriver \
	TestNestedPatternsDriver FinishDriver
TEST_DRIVERS = $(filter-out FinishDriver,$(SCALA_DRIVERS))

include ../common.mk
