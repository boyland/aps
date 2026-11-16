.DEFAULT_GOAL := all
APSTOP = ../..
AST ?= $(error Invoke the Scala build through the example Makefile)
SCALAC = scalac -J-Xmx4g
SCALA = scala
JVMFLAGS = -J-Xss64m -J-Xmx16g
APSLIB = ${APSTOP}/lib/aps-library.jar
SCALABISON = ${HOME}/git/scala-bison/scala-bison-2.13.jar
SCALAFLEX = ${HOME}/Documents/Courses/sp26-cs654/etc/JFlex.jar
APS2SCALA = ${APSTOP}/bin/aps2scala
AST_DIRS := $(sort $(patsubst %/,%,$(dir $(wildcard ../*/*.aps))))
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

OUT = ../scala/${OUTPUT_${EVALUATOR}}
FLAGS_STATIC = -S -C
FLAGS_FARROW = -F
FLAGS_SCC = -C
EVALUATOR_FLAGS = ${FLAGS_${EVALUATOR}}
TREE_MODULES := $(basename ${AST})

vpath %.aps ${AST_DIRS}
vpath %.scala scala

include scala/scala.mk

COMMA := ,
driver-module = $(basename $(firstword $(DRIVER.$1)))
driver-evaluators = $(word 2,$(DRIVER.$1))
driver-options = $(wordlist 3,$(words $(DRIVER.$1)),$(DRIVER.$1))
driver-sources = $(filter %.aps %.scala,$(DRIVER.$1))
driver-main = $(or $(filter-out %.scala,$(call driver-options,$1)),$1)

define validate-driver
$(if $(filter %.aps,$(firstword $(DRIVER.$1))),,$(error DRIVER.$1 must start with an .aps input))
$(if $(call driver-evaluators,$1),,$(error DRIVER.$1 must specify evaluators after the .aps input))
$(if $(filter-out DYNAMIC STATIC FARROW SCC,$(subst $(COMMA), ,$(call driver-evaluators,$1))),$(error Unknown evaluator in DRIVER.$1: $(call driver-evaluators,$1)))
$(if $(or $(filter $(COMMA)% %$(COMMA),$(call driver-evaluators,$1)),$(findstring $(COMMA)$(COMMA),$(call driver-evaluators,$1))),$(error DRIVER.$1 must use a nonempty comma-separated evaluator list))
$(if $(filter %.aps,$(call driver-options,$1)),$(error Put additional APS inputs for DRIVER.$1 in SHARED_SOURCES))
$(if $(word 2,$(filter-out %.scala,$(call driver-options,$1))),$(error DRIVER.$1 allows only one optional main object))
$(if $(and $(filter-out %.scala,$(call driver-options,$1)),$(filter %.scala,$(lastword $(DRIVER.$1)))),$(error DRIVER.$1 main object must follow all .scala inputs))
endef

$(foreach driver,${DRIVERS},$(call validate-driver,$(driver)))
TEST_DRIVERS ?= ${DRIVERS}
TEST_PROGRAMS ?= programs/*
TEST_INPUTS = $(foreach pattern,$(TEST_PROGRAMS),$(or $(wildcard $(pattern)),$(error No program files found for: $(pattern))))
include ../scala/rules.mk

.PHONY: all clean run test
.DELETE_ON_ERROR:
.SECONDARY:

all: ${DRIVER_CLASSES}

test:
	+@status=0; \
	$(foreach driver,$(TEST_DRIVERS), \
	  for program in $(foreach input,$(TEST_INPUTS),'$(input)'); do \
	    bash ../scala/compare-evaluators.sh $(driver) "$$program" \
	      '$(call driver-evaluators,$(driver))' || status=1; \
	  done;) \
	exit $$status

run: ${DRIVER_RUNS}

%.scala: %.aps
	$(MAKE) -f ../scala/Makefile ${OUT}/$*/generate

clean:
	rm -rf $(foreach output,dynamic static farrow scc,$(addprefix ../scala/$(output)/,${MODULES}))
