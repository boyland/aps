DEBUG_FLAGS = -DCOTo
APSSCHED=../bin/apssched
APS2SCALA=../bin/aps2scala
AST_DIRS=tiny simple grammar farrow-lv farrow-ubd nested-ubd balanced bigger fj type conditional standalone
BASE=$(subst $() $(),:,$(AST_DIRS)):../base

vpath %.aps $(AST_DIRS)

default: test

TEST_DIRS = tiny simple bigger grammar farrow-ubd nested-ubd

sched:
	@set -e; for dir in $(AST_DIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

scala:
	$(MAKE) -C scala all

test:
	+@status=0; for dir in $(TEST_DIRS); do \
	  $(MAKE) -C $$dir test || status=1; \
	done; \
	exit $$status

%.sched: %.aps
	${APSSCHED} ${DEBUG_FLAGS} -p ${BASE} $*

%.scc.sched: %.aps
	${APSSCHED} -C ${DEBUG_FLAGS} -p ${BASE} $*

%.scc.scala: %.aps
	${APS2SCALA} -C ${DEBUG_FLAGS} -p ${BASE} $*

%.farrow.scala : %.aps
	${APS2SCALA} -F ${DEBUG_FLAGS} -p ${BASE} $*

%.scala : %.aps
	${APS2SCALA} ${DEBUG_FLAGS} -p ${BASE} $*

%.debug: %.aps
	gdb --args "${APSSCHED}" "${DEBUG_FLAGS}" "-p" "${BASE}" "$*"

.PHONY: default sched scala test %.sched
