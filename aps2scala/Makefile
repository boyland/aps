CPP=g++ 
CPPFLAGS=-Wall -g -DUSING_CXX -I../parse -I../analyze

APSCPPOBJS = apscpp.o dump-cpp.o implement.o dyn-impl.o static-impl.o
APSCPPLIBS = ../lib/aps-lib.o ../lib/aps-ag.a
apscpp : ${APSCPPOBJS} ${APSCPPLIBS}
	${CPP} ${CPPFLAGS} ${APSCPPOBJS} ${APSCPPLIBS} -o apscpp

${APSCPPOBJS} : implement.h dump-cpp.h

install: apscpp
	mv apscpp ../bin/.

clean:
	rm -f apscpp *.o core

realclean: clean

.PHONY: clean realclean
