CC= gcc
AR= ar
RANLIB= ar -qs
PROJECT-DIR=..
APSAGOBJS= aps-bind.o aps-read.o aps-info.o aps-ag-util.o \
	  aps-fiber.o aps-cond.o aps-dnc.o aps-cycle.o aps-oag.o \
	  aps-analyze.o aps-debug.o aps-type.o table.o alist.o \
	  canonical-signature.o canonical-type.o aps-scc.o
APSCOBJS= apsc.o ${APSAGOBJS} 
APSCLIB= ${PROJECT-DIR}/lib/aps-lib.o
UTILITIESLIB= ${PROJECT-DIR}/utilities/utilities.o

all : apsc aps-ag.a

CFLAGS= -I${PROJECT-DIR}/parse -I${PROJECT-DIR}/utilities -g 
apsc : ${APSCOBJS} ${APSCLIB}
	${CC} ${CFLAGS} -o apsc ${APSCOBJS} ${APSCLIB} ${UTILITIESLIB}

aps-ag.a : ${APSAGOBJS}
	${AR} -cr aps-ag.a ${APSAGOBJS}

aps-info.o aps-bind.o aps-ag-util.o aps-fiber.o aps-fiber-callsite.o\
aps-dnc.o aps-cycle.o aps-oag.o : aps-info.h aps-bind.h
aps-dnc.o aps-cycle.o aps-oag.o\
aps-fiber.o aps-fiber-callsite.o: aps-fiber.h aps-dnc.h aps-fiber-callsite.h
aps-oag.o : aps-oag.h
aps-cycle.o aps-dnc.o : aps-cycle.h
aps-debug.o : aps-debug.h
canonical-type.o : canonical-type.h
canonical-signature.o : canonical-type.o canonical-signature.h
aps-scc.o : aps-dnc.o aps-scc.h

install: apsc aps-ag.a
	mv apsc ../bin/apssched
	mv aps-ag.a ../lib/.

clean :
	rm -f *.o apsc core *.a

realclean : clean

.PHONY: install clean realclean

% : RCS/%,v
	co $*
