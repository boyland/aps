SCHED_MODULES ?= $(filter-out $(basename $(notdir $(AST))),$(basename $(notdir $(wildcard *.aps))))
SCALA_MAKE = $(MAKE) -f ../scala/Makefile AST='$(AST)' LEXER='$(LEXER)' PARSER='$(PARSER)'

export $(filter TEST_DRIVERS TEST_PROGRAMS,$(.VARIABLES))

.PHONY: all sched scala test

%.sched: %.aps
	$(MAKE) -C .. $@

%.scala: %.aps
	+$(SCALA_MAKE) $@

%.class %.run:
	+$(SCALA_MAKE) $@

ifneq ($(wildcard scala/scala.mk),)
test:
	+$(SCALA_MAKE) test

scala:
	+$(SCALA_MAKE) all
endif

all: scala

sched: $(addsuffix .sched,$(SCHED_MODULES))
