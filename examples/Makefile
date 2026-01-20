DEBUG_FLAGS = -DCOTo
APSSCHED=../bin/apssched
APS2SCALA=../bin/aps2scala
BASE=.:../base

default:
	@echo "Try 'make simple-oag.sched' to schedule AG in simple-oag"

%.sched : %.aps
	${APSSCHED} ${DEBUG_FLAGS} -p ${BASE} $*

%.scc.sched : %.aps
	${APSSCHED} -C ${DEBUG_FLAGS} -p ${BASE} $*

%.scc.scala : %.aps
	${APS2SCALA} -C ${DEBUG_FLAGS} -p ${BASE} $*

%.synth.scala : %.aps
	${APS2SCALA} -F ${DEBUG_FLAGS} -p ${BASE} $*

%.scala : %.aps
	${APS2SCALA} ${DEBUG_FLAGS} -p ${BASE} $*

%.debug: %.aps
	gdb --args "${APSSCHED}" "${DEBUG_FLAGS}" "-p" "${BASE}" "$*"

.PHONY: default %.sched
