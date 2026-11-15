include ../simple/ast.mk
TREE_MODULES := ${TREE_MODULES} $(basename ${AST_SOURCES})

SIMPLE_OUTPUTS := ${OUT}/classic-binding/% ${OUT}/nested-cycles/% \
	${OUT}/simple-snc/% ${OUT}/simple-snc-fiber/% ${OUT}/simple-binding/% \
	${OUT}/simple-binding1/% ${OUT}/simple-binding2/% ${OUT}/simple-binding3/%
${SIMPLE_OUTPUTS}: SHARED_SOURCES := ${AST_SOURCES}
${SIMPLE_OUTPUTS}: PARSER_BASENAME := Simple
${SIMPLE_OUTPUTS}: LEXER := simple.lex
${SIMPLE_OUTPUTS}: PARSER := simple.y

SOURCES.simple := ${AST_SOURCES}

SOURCES.classic-binding = classic-binding.aps classic-driver.scala
DRIVER.Classic = classic-binding.aps classic-driver.scala

SOURCES.nested-cycles = nested-cycles.aps nested-cycles-driver.scala
DRIVER.NestedCyclesDriver = nested-cycles.aps nested-cycles-driver.scala

SOURCES.simple-snc = simple-snc.aps simple-snc-driver.scala
DRIVER.SimpleSncDriver = simple-snc.aps simple-snc-driver.scala

SOURCES.simple-snc-fiber = simple-snc-fiber.aps simple-snc-fiber-driver.scala
DRIVER.SimpleSncFiberDriver = simple-snc-fiber.aps simple-snc-fiber-driver.scala

SOURCES.simple-binding = simple-binding.aps simple-binding-driver.scala
DRIVER.SimpleBindingDriver = simple-binding.aps simple-binding-driver.scala

SOURCES.simple-binding1 = simple-binding1.aps simple-binding-driver.scala
DRIVER.SimpleBinding1Driver = simple-binding1.aps simple-binding-driver.scala SimpleBindingDriver

SOURCES.simple-binding2 = simple-binding2.aps simple-binding-driver.scala
DRIVER.SimpleBinding2Driver = simple-binding2.aps simple-binding-driver.scala SimpleBindingDriver

SOURCES.simple-binding3 = simple-binding3.aps simple-binding-driver.scala
DRIVER.SimpleBinding3Driver = simple-binding3.aps simple-binding-driver.scala SimpleBindingDriver

DRIVERS += Classic NestedCyclesDriver SimpleSncDriver SimpleSncFiberDriver \
	SimpleBindingDriver SimpleBinding1Driver SimpleBinding2Driver SimpleBinding3Driver
