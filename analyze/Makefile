APSCOBJS= apsc.o aps-bind.o aps-read.o aps-info.o aps-ag-util.o \
	  aps-fiber.o aps-cond.o aps-dnc.o aps-cycle.o aps-oag.o aps-analyze.o
APSCLIB= ${HOME}/work/aps/aps-lib.o
APSCUSE= table.o alist.o

CFLAGS= -I${HOME}/src -I${HOME}/work/aps -g
apsc : ${APSCOBJS} ${APSCLIB} ${APSCUSE}
	cc ${CFLAGS} -o apsc ${APSCOBJS} ${APSCLIB} ${APSCUSE}

aps-info.o aps-bind.o aps-ag-util.o aps-fiber.o \
aps-dnc.o aps-cycle.o aps-oag.o : aps-info.h
aps-dnc.o aps-cycle.o aps-oag.o aps-fiber.o : aps-fiber.h aps-dnc.h
aps-oag.o : aps-oag.h
aps-cycle.o aps-dnc.o : aps-cycle.h
