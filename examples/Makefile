DEBUG_FLAGS = -DCOTo
APSSCHED=../bin/apssched
AST_DIRS=tiny simple grammar farrow-lv farrow-ubd nested-ubd balanced bigger fj type conditional standalone
BASE=$(subst $() $(),:,$(AST_DIRS)):../base

vpath %.aps $(AST_DIRS)

default: test

sched:
	@set -e; for dir in $(AST_DIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

scala:
	$(MAKE) -C scala all

test:
	+@status=0; for dir in $(AST_DIRS); do \
	  $(MAKE) -C $$dir test || status=1; \
	done; \
	exit $$status

%.sched: %.aps
	${APSSCHED} ${DEBUG_FLAGS} -p ${BASE} $*

%.scc.sched: %.aps
	${APSSCHED} -C ${DEBUG_FLAGS} -p ${BASE} $*

%.scc.scala: %.aps force-scala
	$(MAKE) -C scala $@

%.farrow.scala: %.aps force-scala
	$(MAKE) -C scala $@

%.scala: %.aps force-scala
	$(MAKE) -C scala $@

%.debug: %.aps
	gdb --args "${APSSCHED}" "${DEBUG_FLAGS}" "-p" "${BASE}" "$*"

force-scala:

.PHONY: default sched scala test %.sched force-scala
