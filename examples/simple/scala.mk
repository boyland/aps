DRIVER_CLASSES += Classic.class NestedCyclesDriver.class SimpleSncDriver.class \
	SimpleSncFiberDriver.class SimpleBindingDriver.class SimpleBinding1Driver.class \
	SimpleBinding2Driver.class SimpleBinding3Driver.class
PARSER_LEXER_Simple = simple.lex Simple.lex.scala
PARSER_GRAMMAR_Simple = simple.y Simple.y.scala

SOURCES_classic-binding = simple.aps classic-binding.aps classic-driver.scala
SOURCES_nested-cycles = simple.aps nested-cycles.aps nested-cycles-driver.scala
SOURCES_simple-snc = simple.aps simple-snc.aps simple-snc-driver.scala
SOURCES_simple-snc-fiber = simple.aps simple-snc-fiber.aps simple-snc-fiber-driver.scala
SOURCES_simple-binding = simple.aps simple-binding.aps simple-binding-driver.scala
SOURCES_simple-binding1 = simple.aps simple-binding1.aps simple-binding-driver.scala
SOURCES_simple-binding2 = simple.aps simple-binding2.aps simple-binding-driver.scala
SOURCES_simple-binding3 = simple.aps simple-binding3.aps simple-binding-driver.scala
PARSER_classic-binding = Simple
PARSER_nested-cycles = Simple
PARSER_simple-snc = Simple
PARSER_simple-snc-fiber = Simple
PARSER_simple-binding = Simple
PARSER_simple-binding1 = Simple
PARSER_simple-binding2 = Simple
PARSER_simple-binding3 = Simple

Classic.class Classic.run: MODULE = classic-binding
NestedCyclesDriver.class NestedCyclesDriver.run: MODULE = nested-cycles
SimpleSncDriver.class SimpleSncDriver.run: MODULE = simple-snc
SimpleSncFiberDriver.class SimpleSncFiberDriver.run: MODULE = simple-snc-fiber
SimpleBindingDriver.class SimpleBindingDriver.run: MODULE = simple-binding
SimpleBinding1Driver.class SimpleBinding1Driver.run: MODULE = simple-binding1
SimpleBinding2Driver.class SimpleBinding2Driver.run: MODULE = simple-binding2
SimpleBinding3Driver.class SimpleBinding3Driver.run: MODULE = simple-binding3
SimpleBinding1Driver.class SimpleBinding1Driver.run SimpleBinding2Driver.class \
	SimpleBinding2Driver.run SimpleBinding3Driver.class SimpleBinding3Driver.run: MAIN_CLASS = SimpleBindingDriver
