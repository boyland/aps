EXAMPLE := $(notdir $(CURDIR))
AST_MODULES ?=
SCHED_MODULES ?= $(filter-out $(AST_MODULES),$(basename $(notdir $(wildcard *.aps))))
SCALA_DRIVERS ?=
TEST_DRIVERS ?=
TEST_PROGRAMS ?= ../$(EXAMPLE)/programs/*
TEST_EVALUATORS ?= DYNAMIC,STATIC

.PHONY: all sched scala test

test:
ifeq ($(strip $(TEST_DRIVERS)),)
	@echo "No evaluator tests configured for $(EXAMPLE)" >&2; exit 2
else
	+bash ../scala/compare-evaluators.sh --batch \
		$(foreach driver,$(TEST_DRIVERS),$(driver) '$(TEST_PROGRAMS)' '$(or $(TEST_EVALUATORS_$(driver)),$(TEST_EVALUATORS))')
endif

all: scala

sched:
ifeq ($(strip $(SCHED_MODULES)),)
	@echo "No scheduler modules configured for $(EXAMPLE)" >&2; exit 2
else
	$(MAKE) -C .. $(addsuffix .sched,$(SCHED_MODULES))
endif

scala:
ifeq ($(strip $(SCALA_DRIVERS)),)
	@echo "No Scala drivers configured for $(EXAMPLE)" >&2; exit 2
else
	$(MAKE) -C ../scala $(addsuffix .class,$(SCALA_DRIVERS))
endif
