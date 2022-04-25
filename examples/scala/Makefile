APSTOP= ../..
SCALAC= scalac
SCALA= scala
SCALAV= 2.12
APSLIB = ${APSTOP}/lib/aps-library-${SCALAV}.jar
SCALAFLAGS= -cp .:${APSLIB}
SCALACFLAGS= ${SCALAFLAGS}
APS2SCALA = ${APSTOP}/bin/aps2scala
APS2SCALAFLAGS = -p ..:${APSTOP}/base -G

SCALAGEN = simple.scala classic-binding.scala tiny.scala \
	test-coll.scala test-use-coll.scala test-cycle.scala use-global.scala \
	grammar.scala first.scala follow.scala

MISCGEN = nested-cycles.scala GrammarParser.scala SimpleParser.scala \
	GrammarTokens.scala SimpleTokens.scala *.scala~ GrammarScanner.scala \
	*.output *.lex *.y

.PHONY: all clean

all : ${SCALAGEN}
all : grammar_implicit.class simple_implicit.class classic_binding_implicit.class Classic.class
all : GrammarScanner.class GrammarTokens.class SimpleScanner.class SimpleTokens.class
all : GrammarParserBase.class GrammarParser.class SimpleParserBase.class SimpleParser.class
all : test_coll_implicit.class TestCollDriver.class
all : test_cycle_implicit.class TestCycleDriver.class
all : test_use_coll_implicit.class TestUseCollDriver.class
all : grammar_implicit.class GrammarDriver.class
all : nested_cycles_implicit.class NestedCyclesDriver.class

.PHONY: run

run: Classic.run TestCollDriver.run TestCycleDriver.run NestedCyclesDriver.run

%.static.scala : ../%.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} -S $*

%.scala : ../%.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} $*

%_implicit.class : %.scala
	${SCALAC} ${SCALACFLAGS} $*.scala

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

nested-cycles.class: nested-cycles.static.scala simple_implicit.class
	${SCALAC} ${SCALACFLAGS} nested-cycles.scala

.PHONY: nested_cycles_implicit.class
nested_cycles_implicit.class : nested-cycles.class simple_implicit.class ;

NestedCyclesDriver.class: nested_cycles_implicit.class SimpleParser.class
	${SCALAC} ${SCALACFLAGS} nested-cycles-driver.scala

classic_binding_implicit.class : classic-binding.scala
	${SCALAC} ${SCALACFLAGS} $<

Classic.class : classic-driver.scala SimpleParser.class
	${SCALAC} ${SCALACFLAGS} $<

Classic.class   classic_binding_implicit.class : simple_implicit.class
Classic.class : classic_binding_implicit.class

test_coll_implicit.class : test-coll.scala tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} $<

test_use_coll_implicit.class : test-use-coll.scala
	${SCALAC} ${SCALACFLAGS} $<

TestCollDriver.class : test_coll_implicit.class test-coll-driver.scala
	${SCALAC} ${SCALACFLAGS}  test-coll-driver.scala

TestUseCollDriver.class : test_use_coll_implicit.class test-use-coll-driver.scala tiny_implicit.class
	${SCALAC} ${SCALACFLAGS}  test-use-coll-driver.scala

test_cycle_implicit.class : test-cycle.scala
	${SCALAC} ${SCALACFLAGS} $<

TestCycleDriver.class : test_cycle_implicit.class test-cycle-driver.scala
	${SCALAC} ${SCALACFLAGS}  test-cycle-driver.scala

grammar_implicit.class : grammar.scala first.scala follow.scala nullable.scala
	${SCALAC} ${SCALACFLAGS} $^

GrammarDriver.class : grammar_implicit.class GrammarParser.class
	${SCALAC} ${SCALACFLAGS} grammar-driver.scala

TinyParser.class : tiny-parser.handcode.scala tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} tiny-parser.handcode.scala

use_global_implicit.class : use-global.scala tiny_implicit.class
	${SCALAC} ${SCALACFLAGS} use-global.scala

UseGlobal.class : use_global_implicit.class TinyParser.class
UseGlobal.class : use-global-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

.PHONY: %.run

%.run : %.class
	scala ${SCALAFLAGS} $* ${ARGS}

clean:
	rm -f *.class ${SCALAGEN} ${MISCGEN}

