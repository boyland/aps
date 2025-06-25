APSSCHED=../bin/apssched
APS2SCALA=../bin/aps2scala
BASE=.:../base

default:
	@echo "Try 'make simple-oag.sched' to schedule AG in simple-oag"

%.sched : %.aps
	${APSSCHED} -DCOT -p ${BASE} $*

%.scc.sched : %.aps
	${APSSCHED} -C -DCOTo -p ${BASE} $*

%.scc.sched : %.aps
	${APSSCHED} -C -DCOTo -p ${BASE} $*

%.scc.scala : %.aps
	${APS2SCALA} -p ${BASE} $*

%.synth.scala : %.aps
	${APS2SCALA} -F -p ${BASE} $*

%.scala : %.aps
	${APS2SCALA} -p ${BASE} $*

%.debug: %.aps
	gdb --args "${APSSCHED}" "-DeFEfpa" "-p" "${BASE}" "$*"

.PHONY: default %.sched
