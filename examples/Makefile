default:
	@echo "Try 'make simple-oag.sched' to schedule AG in simple-oag"

%.scheds : %.aps
	../bin/apssched -p .:../base $*

%.sched : %.aps
	../bin/apssched -DCuUOT -p .:../base $*

%.debug: %.aps
	gdb --args  "../bin/apssched" "-DeFEfpa" "-p" ".:../base" "../examples/$*"

build:
	cd ../ && make clean && make && cd examples

all: build first-impl.sched

.PHONY: default %.sched
