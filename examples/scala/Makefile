APSTOP= ../..
SCALAC= scalac
SCALA= scala
SCALAV= 2.13
JVMFLAGS= -J-Xss64m -J-Xmx16g 
APSLIB = ${APSTOP}/lib/aps-library.jar
SCALAFLEX= ${APSTOP}/lib/JFlex+scala.jar
SCALABISON= ${APSTOP}/lib/scala-bison.jar
SCALAFLAGS= -cp .:${OUT}:${APSLIB}
SCALACFLAGS= ${SCALAFLAGS} -d ${OUT}
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

OUT_BASE = obj
OUT = ${OUT_BASE}/${EVALUATOR}
$(shell mkdir -p ${OUT})

SCALAGEN = simple.scala classic-binding.scala tiny.scala broad-fiber-cycle.scala \
	below-fiber-cycle.scala below-single-fiber-cycle.scala local-fiber-cycle.scala \
	test-coll.scala test-use-coll.scala test-cycle.scala use-global.scala \
	grammar.scala first.scala follow.scala nullable.scala \
	farrow-lv-tree.scala farrow-ubd-tree.scala

MISCGEN = nested-cycles.scala SimpleParser.scala SimpleScanner.scala \
	GrammarTokens.scala SimpleTokens.scala GrammarScanner.scala GrammarParser.scala \
	FarrowLvTokens.scala FarrowLvTokens.scala FarrowLvScanner.scala FarrowLvParser.scala farrow-lv.scala \
	FarrowUbdTokens.scala FarrowUbdTokens.scala FarrowUbdScanner.scala FarrowUbdParser.scala farrow-ubd.scala \
	farrow-ubd-fiber.scala \
	NestedUbdTokens.scala NestedUbdScanner.scala NestedUbdParser.scala nested-ubd.scala \
	nested-ubd-fiber.scala nested-ubd-tree.scala \
	*.output *.lex *.y *.scala~

.PHONY: all clean phony_explicit

# keep generated .scala
.SECONDARY:

all : ${SCALAGEN}
all : ${OUT}/grammar_implicit.class ${OUT}/simple_implicit.class ${OUT}/classic_binding_implicit.class ${OUT}/Classic.class
all : ${OUT}/GrammarScanner.class ${OUT}/GrammarTokens.class ${OUT}/SimpleScanner.class ${OUT}/SimpleTokens.class
all : ${OUT}/GrammarParserBase.class ${OUT}/GrammarParser.class ${OUT}/SimpleParserBase.class ${OUT}/SimpleParser.class
all : ${OUT}/test_coll_implicit.class ${OUT}/TestCollDriver.class
all : ${OUT}/test_cycle_implicit.class ${OUT}/TestCycleDriver.class
all : ${OUT}/test_use_coll_implicit.class ${OUT}/TestUseCollDriver.class
all : ${OUT}/grammar_implicit.class ${OUT}/GrammarDriver.class
all : ${OUT}/FarrowLv_implicit.class ${OUT}/FarrowLvDriver.class
all : ${OUT}/FarrowUbd_implicit.class ${OUT}/FarrowUbdDriver.class
all : ${OUT}/FarrowUbdFiber_implicit.class ${OUT}/FarrowUbdFiberDriver.class
all : ${OUT}/NestedUbd_implicit.class ${OUT}/NestedUbdDriver.class
all : ${OUT}/NestedUbdFiber_implicit.class ${OUT}/NestedUbdFiberDriver.class
all : ${OUT}/nested_cycles_implicit.class ${OUT}/NestedCyclesDriver.class
all : ${OUT}/broad_fiber_cycle_implicit.class ${OUT}/BroadFiberCycleDriver.class
all : ${OUT}/below_fiber_cycle_implicit.class ${OUT}/BelowFiberCycleDriver.class
all : ${OUT}/below_single_fiber_cycle_implicit.class ${OUT}/BelowSingleFiberCycleDriver.class
all : ${OUT}/local_fiber_cycle_implicit.class ${OUT}/LocalFiberCycleDriver.class

.PHONY: run

run: Classic.run TestCollDriver.run TestCycleDriver.run NestedCyclesDriver.run

phony_explicit:

%.static.scala : ../%.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} -S -C $*
	mv $*.scala $@

%.scala : ../%.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} $*

${OUT}/%_implicit.class : %.scala
	${SCALAC} ${SCALACFLAGS} $*.scala
	@touch $@

%.class : ${OUT}/%.class ;

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

${OUT}/FarrowUbdScanner.class : FarrowUbdScanner.scala ${OUT}/FarrowUbdTokens.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/FarrowUbdParser.class : FarrowUbdParser.scala ${OUT}/FarrowUbdScanner.class ${OUT}/FarrowUbdParserBase.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/FarrowUbdParserBase.class : FarrowUbdParserBase.scala ${OUT}/FarrowUbd_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/FarrowUbdTokens.class : FarrowUbdTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/FarrowUbd_implicit.class : farrow-ubd-tree.scala farrow-ubd${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

${OUT}/FarrowUbdFiber_implicit.class : farrow-ubd-tree.scala farrow-ubd-fiber${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

${OUT}/FarrowUbdDriver.class : ${OUT}/FarrowUbd_implicit.class ${OUT}/FarrowUbdParser.class
	${SCALAC} ${SCALACFLAGS} farrow-ubd-driver.scala

${OUT}/FarrowUbdFiberDriver.class : ${OUT}/FarrowUbdFiber_implicit.class ${OUT}/FarrowUbdParser.class
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

${OUT}/NestedUbdScanner.class : NestedUbdScanner.scala ${OUT}/NestedUbdTokens.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/NestedUbdParser.class : NestedUbdParser.scala ${OUT}/NestedUbdScanner.class ${OUT}/NestedUbdParserBase.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/NestedUbdParserBase.class : NestedUbdParserBase.scala ${OUT}/NestedUbd_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/NestedUbdTokens.class : NestedUbdTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/NestedUbd_implicit.class : nested-ubd-tree.scala nested-ubd${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

${OUT}/NestedUbdDriver.class : ${OUT}/NestedUbd_implicit.class ${OUT}/NestedUbdParser.class
	${SCALAC} ${SCALACFLAGS} nested-ubd-driver.scala

${OUT}/NestedUbdFiber_implicit.class : nested-ubd-tree.scala nested-ubd-fiber${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

${OUT}/NestedUbdFiberDriver.class : ${OUT}/NestedUbdFiber_implicit.class ${OUT}/NestedUbdParser.class
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

${OUT}/FarrowLvScanner.class : FarrowLvScanner.scala ${OUT}/FarrowLvTokens.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/FarrowLvParser.class : FarrowLvParser.scala ${OUT}/FarrowLvScanner.class ${OUT}/FarrowLvParserBase.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/FarrowLvParserBase.class : FarrowLvParserBase.scala ${OUT}/FarrowLv_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/FarrowLvTokens.class : FarrowLvTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/FarrowLv_implicit.class : farrow-lv-tree.scala farrow-lv${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

${OUT}/FarrowLvDriver.class : ${OUT}/FarrowLv_implicit.class ${OUT}/FarrowLvParser.class
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

${OUT}/GrammarScanner.class : GrammarScanner.scala ${OUT}/GrammarTokens.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/GrammarParser.class : GrammarParser.scala ${OUT}/GrammarScanner.class ${OUT}/GrammarParserBase.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/GrammarParserBase.class : GrammarParserBase.scala ${OUT}/grammar_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/GrammarTokens.class : GrammarTokens.scala
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

${OUT}/SimpleScanner.class : SimpleScanner.scala ${OUT}/SimpleTokens.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/SimpleParser.class : SimpleParser.scala ${OUT}/SimpleParserBase.class ${OUT}/SimpleScanner.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/SimpleParserBase.class : SimpleParserBase.scala ${OUT}/simple_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/SimpleTokens.class : SimpleTokens.scala
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/nested-cycles.class: nested-cycles${IMPL_SUFFIX} ${OUT}/simple_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

.PHONY: ${OUT}/nested_cycles_implicit.class
${OUT}/nested_cycles_implicit.class : ${OUT}/nested-cycles.class ${OUT}/simple_implicit.class ;

${OUT}/NestedCyclesDriver.class: ${OUT}/nested_cycles_implicit.class ${OUT}/SimpleParser.class
	${SCALAC} ${SCALACFLAGS} nested-cycles-driver.scala

${OUT}/classic_binding_implicit.class : classic-binding${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/Classic.class : classic-driver.scala ${OUT}/SimpleParser.class
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/Classic.class   ${OUT}/classic_binding_implicit.class : ${OUT}/simple_implicit.class
${OUT}/Classic.class : ${OUT}/classic_binding_implicit.class

${OUT}/test_coll_implicit.class : test-coll${IMPL_SUFFIX} ${OUT}/tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/test_use_coll_implicit.class : test-use-coll${IMPL_SUFFIX} ${OUT}/tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/TestCollDriver.class : ${OUT}/test_coll_implicit.class ${OUT}/TinyParser.class test-coll-driver.scala
	${SCALAC} ${SCALACFLAGS}  test-coll-driver.scala

${OUT}/TestUseCollDriver.class : ${OUT}/test_use_coll_implicit.class ${OUT}/TinyParser.class test-use-coll-driver.scala ${OUT}/tiny_implicit.class
	${SCALAC} ${SCALACFLAGS}  test-use-coll-driver.scala

${OUT}/test_cycle_implicit.class : test-cycle${IMPL_SUFFIX} ${OUT}/tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/TestCycleDriver.class : ${OUT}/test_cycle_implicit.class ${OUT}/TinyParser.class test-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS}  test-cycle-driver.scala

${OUT}/grammar_implicit.class : grammar.scala first${IMPL_SUFFIX} follow${IMPL_SUFFIX} nullable${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $^
	@touch $@

${OUT}/GrammarDriver.class : ${OUT}/grammar_implicit.class ${OUT}/GrammarParser.class
	${SCALAC} ${SCALACFLAGS} grammar-driver.scala

${OUT}/TinyParser.class : tiny-parser.handcode.scala ${OUT}/tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} tiny-parser.handcode.scala

${OUT}/use_global_implicit.class : use-global${IMPL_SUFFIX} ${OUT}/tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/UseGlobal.class : ${OUT}/use_global_implicit.class ${OUT}/TinyParser.class
${OUT}/UseGlobal.class : use-global-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/broad_fiber_cycle_implicit.class : ${OUT}/tiny_implicit.class
${OUT}/broad_fiber_cycle_implicit.class : broad-fiber-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/BroadFiberCycleDriver.class : ${OUT}/broad_fiber_cycle_implicit.class ${OUT}/tiny_implicit.class ${OUT}/TinyParser.class
${OUT}/BroadFiberCycleDriver.class : broad-fiber-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/below_fiber_cycle_implicit.class : ${OUT}/tiny_implicit.class
${OUT}/below_fiber_cycle_implicit.class : below-fiber-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/BelowFiberCycleDriver.class : ${OUT}/below_fiber_cycle_implicit.class ${OUT}/tiny_implicit.class ${OUT}/TinyParser.class
${OUT}/BelowFiberCycleDriver.class : below-fiber-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/below_single_fiber_cycle_implicit.class : ${OUT}/tiny_implicit.class
${OUT}/below_single_fiber_cycle_implicit.class : below-single-fiber-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/BelowSingleFiberCycleDriver.class : ${OUT}/below_single_fiber_cycle_implicit.class ${OUT}/tiny_implicit.class ${OUT}/TinyParser.class
${OUT}/BelowSingleFiberCycleDriver.class : below-single-fiber-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

${OUT}/local_fiber_cycle_implicit.class : ${OUT}/tiny_implicit.class
${OUT}/local_fiber_cycle_implicit.class : local-fiber-cycle${IMPL_SUFFIX}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

${OUT}/LocalFiberCycleDriver.class : ${OUT}/local_fiber_cycle_implicit.class ${OUT}/tiny_implicit.class ${OUT}/TinyParser.class
${OUT}/LocalFiberCycleDriver.class : local-fiber-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

.PHONY: %.run

%.run : ${OUT}/%.class
	scala ${JVMFLAGS} ${SCALAFLAGS} $* ${ARGS}

clean:
	rm -rf ${OUT_BASE}
	rm -f *.class ${SCALAGEN} ${MISCGEN} *.static.scala

