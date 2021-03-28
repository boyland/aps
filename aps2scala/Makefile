CPP=g++ 
CPPFLAGS=-Wall -g -DUSING_CXX -I../parse -I../analyze

APS2SCALAOBJS = aps2scala.o dump-scala.o implement.o dyn-impl.o static-impl.o
APS2SCALALIBS = ../lib/aps-lib.o ../lib/aps-ag.a
aps2scala : ${APS2SCALAOBJS} ${APS2SCALALIBS}
	${CPP} ${CPPFLAGS} ${APS2SCALAOBJS} ${APS2SCALALIBS} -o aps2scala

${APS2SCALAOBJS} : implement.h dyn-impl.h dump-scala.h

%.cc : RCS/%.cc,v
	co $<

%.h : RCS/%.h,v
	co $<

install: aps2scala
	mv aps2scala ../bin/.

clean:
	rm -f aps2scala *.o core

realclean: clean

.PHONY: clean realclean
