APSTOP= ../..
SCALAC= scalac
SCALA= scala
SCALAV= 2.13
JVMFLAGS= -J-Xss64m -J-Xmx16g 
APSLIB = ${APSTOP}/lib/aps-library.jar
SCALAFLEX= ${APSTOP}/lib/JFlex+scala.jar
SCALABISON= ${APSTOP}/lib/scala-bison.jar
SCALAFLAGS= -cp .:${APSLIB}
SCALACFLAGS= ${SCALAFLAGS}
APS2SCALA = ${APSTOP}/bin/aps2scala
APS2SCALAFLAGS = -p ..:${APSTOP}/base -G
SCALABISON= ${HOME}/git/scala-bison/scala-bison-2.13.jar
SCALAFLEX= ${HOME}/Documents/Courses/sp26-cs654/etc/JFlex.jar

EVALUATOR ?= DYNAMIC

ifeq (${EVALUATOR},STATIC)
IMPL_SUFFIX = .static.scala
else
IMPL_SUFFIX = .scala
endif

SCALAGEN = simple.scala classic-binding.scala tiny.scala broad-fiber-cycle.scala \
	below-fiber-cycle.scala below-single-fiber-cycle.scala local-fiber-cycle.scala \
	test-coll.scala test-use-coll.scala test-cycle.scala use-global.scala \
	grammar.scala first.scala follow.scala nullable.scala	\
	farrow-lv-tree.scala farrow-lv.static.scala \
	farrow-ubd-tree.scala farrow-ubd.static.scala farrow-ubd-fiber.static.scala \
	broad-fiber-cycle.static.scala below-fiber-cycle.static.scala \
	below-single-fiber-cycle.static.scala local-fiber-cycle.static.scala \
	classic-binding.static.scala \
	grammar.scala first.static.scala follow.static.scala nullable.static.scala \
	nested-cycles.static.scala \
	test-coll.static.scala test-use-coll.static.scala test-cycle.static.scala use-global.static.scala

MISCGEN = nested-cycles.scala SimpleParser.scala SimpleScanner.scala \
	GrammarTokens.scala SimpleTokens.scala GrammarScanner.scala GrammarParser.scala \
	FarrowLvTokens.scala FarrowLvTokens.scala FarrowLvScanner.scala FarrowLvParser.scala farrow-lv.scala \
	FarrowUbdTokens.scala FarrowUbdTokens.scala FarrowUbdScanner.scala FarrowUbdParser.scala farrow-ubd.scala \
	farrow-ubd-fiber.scala \
	NestedUbdTokens.scala NestedUbdScanner.scala NestedUbdParser.scala nested-ubd.scala \
	nested-ubd-fiber.scala \
	*.output *.lex *.y *.scala~

.PHONY: all clean phony_explicit

all : ${SCALAGEN}
all : grammar_implicit.class simple_implicit.class classic_binding_implicit.class Classic.class
all : GrammarScanner.class GrammarTokens.class SimpleScanner.class SimpleTokens.class
all : GrammarParserBase.class GrammarParser.class SimpleParserBase.class SimpleParser.class
all : test_coll_implicit.class TestCollDriver.class
all : test_cycle_implicit.class TestCycleDriver.class
all : test_use_coll_implicit.class TestUseCollDriver.class
all : grammar_implicit.class GrammarDriver.class
all : FarrowLv_implicit.class FarrowLvDriver.class
all : FarrowUbd_implicit.class FarrowUbdDriver.class
all : FarrowUbdFiber_implicit.class FarrowUbdFiberDriver.class
all : NestedUbd_implicit.class NestedUbdDriver.class
all : NestedUbdFiber_implicit.class NestedUbdFiberDriver.class
all : nested_cycles_implicit.class NestedCyclesDriver.class
all : broad_fiber_cycle_implicit.class BroadFiberCycleDriver.class
all : below_fiber_cycle_implicit.class BelowFiberCycleDriver.class
all : below_single_fiber_cycle_implicit.class BelowSingleFiberCycleDriver.class
all : local_fiber_cycle_implicit.class LocalFiberCycleDriver.class

.PHONY: run

run: Classic.run TestCollDriver.run TestCycleDriver.run NestedCyclesDriver.run

phony_explicit:

%.static.scala : ../%.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} -S -C $*
	mv $*.scala $@

%.synth.scala : ../%.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} -F $*
	mv $*.scala $@

%.scala : ../%.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} $*

%_implicit.class : %.scala
	${SCALAC} ${SCALACFLAGS} $*.scala
	@touch $@

FarrowUbd.lex : ../farrow-ubd.lex FarrowUbd.lex.scala
	cat FarrowUbd.lex.scala ../farrow-ubd.lex > $@

FarrowUbdScanner.scala : FarrowUbd.lex
	java -jar ${SCALAFLEX} --scala $<

FarrowUbd.y : ../farrow-ubd.y FarrowUbd.y.scala
	cat $< FarrowUbd.y.scala > $@

FarrowUbdParser.scala FarrowUbdTokens.scala : FarrowUbd.y ${SCALABISON}
	bison -v FarrowUbd.y
	rm FarrowUbd.tab.c
	${SCALA} -classpath ${SCALABISON} edu.uwm.cs.scalabison.RunGenerator FarrowUbd.y

FarrowUbdScanner.class : FarrowUbdScanner.scala FarrowUbdTokens.class
	${SCALAC} ${SCALACFLAGS} $<

FarrowUbdParser.class : FarrowUbdParser.scala FarrowUbdScanner.class FarrowUbdParserBase.class
	${SCALAC} ${SCALACFLAGS} $<

FarrowUbdParserBase.class : FarrowUbdParserBase.scala FarrowUbd_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

FarrowUbdTokens.class : FarrowUbdTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

FarrowUbd_implicit.class : farrow-ubd-tree.scala farrow-ubd${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

FarrowUbdFiber_implicit.class : farrow-ubd-tree.scala farrow-ubd-fiber${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

FarrowUbdDriver.class : FarrowUbd_implicit.class FarrowUbdParser.class
	${SCALAC} ${SCALACFLAGS} farrow-ubd-driver.scala

FarrowUbdFiberDriver.class : FarrowUbdFiber_implicit.class FarrowUbdParser.class
	${SCALAC} ${SCALACFLAGS} farrow-ubd-fiber-driver.scala

NestedUbd.lex : ../nested-ubd.lex NestedUbd.lex.scala
	cat NestedUbd.lex.scala ../nested-ubd.lex > $@

NestedUbdScanner.scala : NestedUbd.lex
	java -jar ${SCALAFLEX} --scala $<

NestedUbd.y : ../nested-ubd.y NestedUbd.y.scala
	cat $< NestedUbd.y.scala > $@

NestedUbdParser.scala NestedUbdTokens.scala : NestedUbd.y ${SCALABISON}
	bison -v NestedUbd.y
	rm NestedUbd.tab.c
	${SCALA} -classpath ${SCALABISON} edu.uwm.cs.scalabison.RunGenerator NestedUbd.y

NestedUbdScanner.class : NestedUbdScanner.scala NestedUbdTokens.class
	${SCALAC} ${SCALACFLAGS} $<

NestedUbdParser.class : NestedUbdParser.scala NestedUbdScanner.class NestedUbdParserBase.class
	${SCALAC} ${SCALACFLAGS} $<

NestedUbdParserBase.class : NestedUbdParserBase.scala NestedUbd_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

NestedUbdTokens.class : NestedUbdTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

NestedUbd_implicit.class : nested-ubd-tree.scala nested-ubd${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

NestedUbdDriver.class : NestedUbd_implicit.class NestedUbdParser.class
	${SCALAC} ${SCALACFLAGS} nested-ubd-driver.scala

NestedUbdFiber_implicit.class : nested-ubd-tree.scala nested-ubd-fiber.static.scala
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

NestedUbdFiberDriver.class : NestedUbdFiber_implicit.class NestedUbdParser.class
	${SCALAC} ${SCALACFLAGS} nested-ubd-fiber-driver.scala

FarrowLv.lex : ../farrow-lv.lex FarrowLv.lex.scala
	cat FarrowLv.lex.scala ../farrow-lv.lex > $@

FarrowLvScanner.scala : FarrowLv.lex
	java -jar ${SCALAFLEX} --scala $<

FarrowLv.y : ../farrow-lv.y FarrowLv.y.scala
	cat $< FarrowLv.y.scala > $@

FarrowLvParser.scala FarrowLvTokens.scala : FarrowLv.y ${SCALABISON}
	bison -v FarrowLv.y
	rm FarrowLv.tab.c
	${SCALA} -classpath ${SCALABISON} edu.uwm.cs.scalabison.RunGenerator FarrowLv.y

FarrowLvScanner.class : FarrowLvScanner.scala FarrowLvTokens.class
	${SCALAC} ${SCALACFLAGS} $<

FarrowLvParser.class : FarrowLvParser.scala FarrowLvScanner.class FarrowLvParserBase.class
	${SCALAC} ${SCALACFLAGS} $<

FarrowLvParserBase.class : FarrowLvParserBase.scala FarrowLv_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

FarrowLvTokens.class : FarrowLvTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

FarrowLv_implicit.class : farrow-lv-tree.scala farrow-lv${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

FarrowLvDriver.class : FarrowLv_implicit.class FarrowLvParser.class
	${SCALAC} ${SCALACFLAGS} farrow-lv-driver.scala

Grammar.lex : ../grammar.lex Grammar.lex.scala
	cat Grammar.lex.scala ../grammar.lex > $@

GrammarScanner.scala : Grammar.lex
	java -jar ${SCALAFLEX} --scala $<

Grammar.y : ../grammar.y Grammar.y.scala
	cat $< Grammar.y.scala > $@

GrammarParser.scala GrammarTokens.scala : Grammar.y ${SCALABISON}
	bison -v Grammar.y
	rm Grammar.tab.c
	${SCALA} -classpath ${SCALABISON} edu.uwm.cs.scalabison.RunGenerator Grammar.y

GrammarScanner.class : GrammarScanner.scala GrammarTokens.class
	${SCALAC} ${SCALACFLAGS} $<

GrammarParser.class : GrammarParser.scala GrammarScanner.class GrammarParserBase.class
	${SCALAC} ${SCALACFLAGS} $<

GrammarParserBase.class : GrammarParserBase.scala grammar_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

GrammarTokens.class : GrammarTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

Simple.lex : ../simple.lex Simple.lex.scala
	cat Simple.lex.scala ../simple.lex > $@

SimpleScanner.scala : Simple.lex
	java -jar ${SCALAFLEX} --scala $<

Simple.y : ../simple.y Simple.y.scala
	cat $< Simple.y.scala > $@

SimpleParser.scala SimpleTokens.scala : Simple.y ${SCALABISON}
	bison -v Simple.y
	rm Simple.tab.c
	${SCALA} -classpath ${SCALABISON} edu.uwm.cs.scalabison.RunGenerator Simple.y

SimpleScanner.class : SimpleScanner.scala SimpleTokens.class
	${SCALAC} ${SCALACFLAGS} $<

SimpleParser.class : SimpleParser.scala SimpleParserBase.class SimpleScanner.class
	${SCALAC} ${SCALACFLAGS} $<

SimpleParserBase.class : SimpleParserBase.scala simple_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

SimpleTokens.class : SimpleTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

nested-cycles.class: nested-cycles${IMPL_SUFFIX} simple_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

.PHONY: nested_cycles_implicit.class
nested_cycles_implicit.class : nested-cycles.class simple_implicit.class ;

NestedCyclesDriver.class: nested_cycles_implicit.class SimpleParser.class
	${SCALAC} ${SCALACFLAGS} nested-cycles-driver.scala

classic_binding_implicit.class : classic-binding${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

Classic.class : classic-driver.scala SimpleParser.class
	${SCALAC} ${SCALACFLAGS} $<

Classic.class   classic_binding_implicit.class : simple_implicit.class
Classic.class : classic_binding_implicit.class

test_coll_implicit.class : test-coll${IMPL_SUFFIX} tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

test_use_coll_implicit.class : test-use-coll${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

TestCollDriver.class : test_coll_implicit.class TinyParser.class test-coll-driver.scala
	${SCALAC} ${SCALACFLAGS}  test-coll-driver.scala

TestUseCollDriver.class : test_use_coll_implicit.class TinyParser.class test-use-coll-driver.scala tiny_implicit.class
	${SCALAC} ${SCALACFLAGS}  test-use-coll-driver.scala

test_cycle_implicit.class : test-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

TestCycleDriver.class : test_cycle_implicit.class TinyParser.class test-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS}  test-cycle-driver.scala

grammar_implicit.class : grammar.scala first${IMPL_SUFFIX} follow${IMPL_SUFFIX} nullable${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

GrammarDriver.class : grammar_implicit.class GrammarParser.class
	${SCALAC} ${SCALACFLAGS} grammar-driver.scala

TinyScanner.class : TinyScanner.scala
	${SCALAC} ${SCALACFLAGS} $<

TinyParser.class : TinyParser.scala TinyScanner.class tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

use_global_implicit.class : use-global${IMPL_SUFFIX} tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

UseGlobal.class : use_global_implicit.class TinyParser.class
UseGlobal.class : use-global-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

broad_fiber_cycle_implicit.class : tiny_implicit.class
broad_fiber_cycle_implicit.class : broad-fiber-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

BroadFiberCycleDriver.class : broad_fiber_cycle_implicit.class tiny_implicit.class TinyParser.class
BroadFiberCycleDriver.class : broad-fiber-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

below_fiber_cycle_implicit.class : tiny_implicit.class
below_fiber_cycle_implicit.class : below-fiber-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

BelowFiberCycleDriver.class : below_fiber_cycle_implicit.class tiny_implicit.class TinyParser.class
BelowFiberCycleDriver.class : below-fiber-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

below_single_fiber_cycle_implicit.class : tiny_implicit.class
below_single_fiber_cycle_implicit.class : below-single-fiber-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

BelowSingleFiberCycleDriver.class : below_single_fiber_cycle_implicit.class tiny_implicit.class TinyParser.class
BelowSingleFiberCycleDriver.class : below-single-fiber-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

local_fiber_cycle_implicit.class : tiny_implicit.class
local_fiber_cycle_implicit.class : local-fiber-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

LocalFiberCycleDriver.class : local_fiber_cycle_implicit.class tiny_implicit.class TinyParser.class
LocalFiberCycleDriver.class : local-fiber-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

.PHONY: %.run

%.run : %.class
	scala ${JVMFLAGS} ${SCALAFLAGS} $* ${ARGS}

clean:
	rm -f *.class ${SCALAGEN} ${MISCGEN}

