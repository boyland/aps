EXAMPLE := $(notdir $(CURDIR))
AST_MODULES ?=
SCHED_MODULES ?= $(filter-out $(AST_MODULES),$(basename $(notdir $(wildcard *.aps))))
SCALA_DRIVERS ?=
TEST_DRIVERS ?=
TEST_PROGRAMS ?= ../$(EXAMPLE)/programs/*
DEFAULT_EVALUATORS ?= DYNAMIC,STATIC
TEST_EVALUATORS ?= $(DEFAULT_EVALUATORS)

.PHONY: all sched scala test

%.sched: %.aps
	$(MAKE) -C .. $@

%.scala: %.aps
	$(MAKE) -C .. $@

test:
	+@status=0; \
	$(foreach driver,$(TEST_DRIVERS), \
	  bash ../scala/compare-evaluators.sh $(driver) '$(TEST_PROGRAMS)' \
	    '$(or $(TEST_EVALUATORS_$(driver)),$(TEST_EVALUATORS))' || status=1;) \
	exit $$status

all: scala

sched: $(addsuffix .sched,$(SCHED_MODULES))

scala:
	+@for driver in $(SCALA_DRIVERS); do \
	  $(MAKE) -C ../scala $$driver.class || exit $$?; \
	done
