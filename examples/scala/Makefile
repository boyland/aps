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
APSPATH = $(subst $() $(),:,$(abspath ${AST_DIRS} ${APSTOP}/base))
APS2SCALAFLAGS = -p ${APSPATH} -G
EVALUATOR ?= DYNAMIC

OUTPUT_DYNAMIC = dynamic
OUTPUT_STATIC = static
OUTPUT_FARROW = farrow
OUTPUT_SCC = scc
ifeq (${OUTPUT_${EVALUATOR}},)
$(error Unknown EVALUATOR '${EVALUATOR}'; expected DYNAMIC, STATIC, FARROW or SCC)
endif

OUT = ${OUTPUT_${EVALUATOR}}
FLAGS_STATIC = -S -C
FLAGS_FARROW = -F
FLAGS_SCC = -C
EVALUATOR_FLAGS = ${FLAGS_${EVALUATOR}}
TREE_MODULES = simple bigger tiny grammar farrow-lv-tree farrow-ubd-tree nested-ubd-tree
DRIVER_CLASSES =
MAIN_CLASS = $(basename $@)

vpath %.aps ${AST_DIRS}
vpath %.scala ${SCALA_SOURCE_DIRS}
vpath %.lex ${AST_DIRS}
vpath %.y ${AST_DIRS}

include $(wildcard ../*/scala.mk)
include rules.mk

.PHONY: all clean run test force-generation
.DELETE_ON_ERROR:
.SECONDARY:

all: ${DRIVER_CLASSES}
run: Classic.run TestCollDriver.run TestCycleDriver.run NestedCyclesDriver.run
test:
	+bash ./compare-evaluators.sh

%.static.scala: %.aps force-generation
	$(MAKE) EVALUATOR=STATIC static/$*/.generated

%.scc.scala: %.aps force-generation
	$(MAKE) EVALUATOR=SCC scc/$*/.generated

%.farrow.scala: %.aps force-generation
	$(MAKE) EVALUATOR=FARROW farrow/$*/.generated

%.scala: %.aps force-generation
	$(MAKE) ${OUT}/$*/.generated

force-generation:

clean:
	rm -rf dynamic static farrow scc
