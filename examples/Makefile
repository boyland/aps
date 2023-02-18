APSSCHED=../bin/apssched
BASE=.:../base

default:
	@echo "Try 'make simple-oag.sched' to schedule AG in simple-oag"

%.sched : %.aps
	${APSSCHED} -DCOT -p ${BASE} $*

%.scc.sched : %.aps
	${APSSCHED} -S -DCOTo -p ${BASE} $*

%.debug: %.aps
	gdb --args "${APSSCHED}" "-DeFEfpa" "-p" "${BASE}" "$*"

.PHONY: default %.sched
