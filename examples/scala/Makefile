.DEFAULT_GOAL := all
APSTOP = ../..
SCALAC = scalac -J-Xmx4g
SCALA = scala
SCALAV = 2.13
JVMFLAGS = -J-Xss64m -J-Xmx16g
APSLIB = ${APSTOP}/lib/aps-library.jar
SCALABISON = ${HOME}/git/scala-bison/scala-bison-2.13.jar
SCALAFLEX = ${HOME}/Documents/Courses/sp26-cs654/etc/JFlex.jar
APS2SCALA = ${APSTOP}/bin/aps2scala
AST_DIRS = ../tiny ../simple ../grammar ../farrow-lv ../farrow-ubd ../nested-ubd \
	../balanced ../bigger ../fj ../type ../conditional ../standalone
SCALA_SOURCE_DIRS = $(addsuffix /scala,${AST_DIRS})
APSPATH = $(subst $() $(),:,${AST_DIRS}):${APSTOP}/base
SCALAFLAGS = -cp .:${OUT}:${APSLIB}
SCALACFLAGS = ${SCALAFLAGS} -d ${OUT}
APS2SCALAFLAGS = -p ${APSPATH} -G
EVALUATOR ?= DYNAMIC

ifeq (${EVALUATOR},STATIC)
IMPL_SUFFIX = .static.scala
else ifeq (${EVALUATOR},FARROW)
IMPL_SUFFIX = .farrow.scala
else ifeq (${EVALUATOR},DYNAMIC)
IMPL_SUFFIX = .scala
else
$(error Unknown EVALUATOR '${EVALUATOR}'; expected DYNAMIC, STATIC or FARROW)
endif

OUT_BASE = obj
OUT = ${OUT_BASE}/${EVALUATOR}
AST_TREE_SCALAGEN = simple.scala bigger.scala tiny.scala grammar.scala \
	farrow-lv-tree.scala farrow-ubd-tree.scala nested-ubd-tree.scala
EVALUATOR_EXAMPLES =
EVALUATOR_CLASSES =
DRIVER_CLASSES =
ADDITIONAL_CLASSES =
ALL_CLASSES = ${DRIVER_CLASSES} ${ADDITIONAL_CLASSES}
PARSERS =
MISCGEN = *.output *.lex *.y *.scala~

vpath %.aps ${AST_DIRS}
vpath %.scala ${SCALA_SOURCE_DIRS}
vpath %.lex ${AST_DIRS}
vpath %.y ${AST_DIRS}
vpath %.program $(addsuffix /programs,${AST_DIRS})
vpath %.cfg $(addsuffix /programs,${AST_DIRS})

include $(wildcard ../*/scala.mk)
include rules.mk

SCALAGEN = ${AST_TREE_SCALAGEN} \
	$(foreach example,${EVALUATOR_EXAMPLES},$(example).scala $(example).static.scala $(example).farrow.scala)

.PHONY: all clean run test phony_explicit
.SECONDARY:

all: ${ALL_CLASSES} ${BINDING_VARIANT_CLASSES}
run: Classic.run TestCollDriver.run TestCycleDriver.run NestedCyclesDriver.run
test:
	+bash ./compare-evaluators.sh
phony_explicit:

${OUT}:
	mkdir -p $@

%.static.scala: %.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} -S -C $*
	cp $*.scala $@

%.farrow.scala: %.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} -F $*
	cp $*.scala $@

%.scala: %.aps ${APS2SCALA}
	${APS2SCALA} ${APS2SCALAFLAGS} $*

${OUT}/%_implicit.class: %.scala | ${OUT}
	${SCALAC} ${SCALACFLAGS} $<
	@touch $@

%.class: ${OUT}/%.class ;

%.run: ${OUT}/%.class
	${SCALA} ${JVMFLAGS} ${SCALAFLAGS} $* ${ARGS}

clean:
	rm -rf ${OUT_BASE}
	rm -f *.class ${SCALAGEN} ${MISCGEN}
