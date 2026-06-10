CPP=g++ 
CPPFLAGS=-Wall -g -Wno-unused-variable -DUSING_CXX -DAPS2SCALA -I../parse -I../analyze -I../codegen -I../utilities

APS2SCALAOBJS = aps2scala.o dump-scala.o implement.o dyn-impl.o static-impl.o static-scc-impl.o synth-impl.o
APS2SCALALIBS = ../lib/aps-lib.o ../lib/aps-ag.a ../utilities/utilities.o
aps2scala : ${APS2SCALAOBJS} ${APS2SCALALIBS}
	${CPP} ${CPPFLAGS} ${APS2SCALAOBJS} ${APS2SCALALIBS} -o aps2scala

${APS2SCALAOBJS} : dump-scala.h

install: aps2scala
	mv aps2scala ../bin/.

synth-impl.o : ../codegen/synth-impl.cc 
	${CPP} -c ${CPPFLAGS} $< -o $@

static-impl.o : ../codegen/static-impl.cc 
	${CPP} -c ${CPPFLAGS} $< -o $@

static-scc-impl.o : ../codegen/static-scc-impl.cc 
	${CPP} -c ${CPPFLAGS} $< -o $@

implement.o : ../codegen/implement.cc
	${CPP} -c ${CPPFLAGS} $< -o $@

clean:
	rm -f aps2scala *.o core

realclean: clean

.PHONY: clean realclean
