CC= gcc
AR= ar
RANLIB= ar -qs
PROJECT-DIR=..
APSAGOBJS= aps-bind.o aps-read.o aps-info.o aps-ag-util.o \
	  aps-fiber.o aps-cond.o aps-dnc.o aps-cycle.o aps-oag.o \
	  aps-analyze.o aps-debug.o aps-type.o table.o alist.o \
	  prime.o hashcons.o canonical-signature.o canonical-type.o \
	  stack.o scc.o aps-scc.o topological-sort.o aps-schedule.o
APSCOBJS= apsc.o ${APSAGOBJS} 
APSCLIB= ${PROJECT-DIR}/lib/aps-lib.o

all : apsc aps-ag.a

CFLAGS= -I${PROJECT-DIR}/parse -g
apsc : ${APSCOBJS} ${APSCLIB}
	${CC} ${CFLAGS} -o apsc ${APSCOBJS} ${APSCLIB}

aps-ag.a : ${APSAGOBJS}
	${AR} -cr aps-ag.a ${APSAGOBJS}

aps-info.o aps-bind.o aps-ag-util.o aps-fiber.o aps-fiber-callsite.o\
aps-dnc.o aps-cycle.o aps-oag.o : aps-info.h aps-bind.h
aps-dnc.o aps-cycle.o aps-oag.o\
aps-fiber.o aps-fiber-callsite.o: aps-fiber.h aps-dnc.h aps-fiber-callsite.h
aps-oag.o : aps-oag.h
aps-cycle.o aps-dnc.o : aps-cycle.h
aps-debug.o : aps-debug.h
prime.o : prime.h
hashcons.o : prime.o hashcons.h
canonical-type.o : hashcons.o canonical-type.h
canonical-signature.o : hashcons.o canonical-type.o canonical-signature.h
canonical-signature.o : hashcons.o canonical-type.o canonical-signature.h
stack.o : stack.h
scc.o : stack.o scc.h
topological-sort.o : topological-sort.h
aps-scc.o : aps-dnc.o scc.o aps-scc.h
aps-schedule.o : aps-dnc.o aps-cycle.o aps-scc.o topological-sort.o aps-schedule.h

install: apsc aps-ag.a
	mv apsc ../bin/apssched
	mv aps-ag.a ../lib/.

clean :
	rm -f *.o apsc core *.a

realclean : clean

.PHONY: install clean realclean

% : RCS/%,v
	co $*
