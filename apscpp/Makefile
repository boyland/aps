CPP=g++ 
CPPFLAGS=-Wall -g -DUSING_CXX -I../parse -I../analyze -I../codegen -I../utils

APSCPPOBJS = apscpp.o dump-cpp.o implement.o dyn-impl.o static-impl.o
APSCPPLIBS = ../lib/aps-lib.o ../lib/aps-ag.a ../utils/utils.o
apscpp : ${APSCPPOBJS} ${APSCPPLIBS}
	${CPP} ${CPPFLAGS} ${APSCPPOBJS} ${APSCPPLIBS} -o apscpp

${APSCPPOBJS} : dump-cpp.h

static-impl.o : ../codegen/static-impl.cc
	${CPP} -c ${CPPFLAGS} $< -o $@

implement.o : ../codegen/implement.cc
	${CPP} -c ${CPPFLAGS} $< -o $@

install: apscpp
	mv apscpp ../bin/.

clean:
	rm -f apscpp *.o core

realclean: clean

.PHONY: clean realclean
