default:
	@echo "Try 'make simple-oag.sched' to schedule AG in simple-oag"

%.sched : %.aps
	../bin/apssched -DCOT -p .:../base $*

.PHONY: default %.sched
