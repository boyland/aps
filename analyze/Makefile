CC= gcc
AR= ar
RANLIB= ar -qs
PROJECT-DIR=/afs/cs.uwm.edu/project/aps
APSAGOBJS= aps-bind.o aps-read.o aps-info.o aps-ag-util.o \
	  aps-fiber.o aps-cond.o aps-dnc.o aps-cycle.o aps-oag.o \
	  aps-analyze.o aps-type.o
APSCOBJS= apsc.o ${APSAGOBJS} aps-fiber-callsite.o
APSCLIB= ${PROJECT-DIR}/aps/aps-lib.o
APSCUSE= table.o alist.o

all : apsc aps-ag.a

CFLAGS= -I${PROJECT-DIR}/aps -g
apsc : ${APSCOBJS} ${APSCLIB} ${APSCUSE}
	${CC} ${CFLAGS} -o apsc ${APSCOBJS} ${APSCLIB} ${APSCUSE}

aps-ag.a : ${APSAGOBJS}
	${AR} -cr aps-ag.a ${APSAGOBJS}

aps-info.o aps-bind.o aps-ag-util.o aps-fiber.o aps-fiber-callsite.o\
aps-dnc.o aps-cycle.o aps-oag.o : aps-info.h
aps-dnc.o aps-cycle.o aps-oag.o\
aps-fiber.o aps-fiber-callsite.o: aps-fiber.h aps-dnc.h aps-fiber-callsite.h
aps-oag.o : aps-oag.h
aps-cycle.o aps-dnc.o : aps-cycle.h

bin.install: apsc
	cp apsc /users/classes/cs754/bin/.`uname -m`-`uname -s`/apssched
