APSTOP= ../..
CPP = g++
APSCPP = ${APSTOP}/bin/apscpp
APSCPPFLAGS = -p ${APSTOP}/base

all : basic.h table.h

.PHONY: all install clean realclean

install : all basecpp.a
	mv basecpp.a ${APSTOP}/lib/.

BASEOBJS = basic.o table.o aps-impl.o
basecpp.a : ${BASEOBJS}
	ar -cr basecpp.a ${BASEOBJS}

basic.h basic.cpp : 
	${APSCPP} ${APSCPPFLAGS} \
	       --omit _basic_1 --omit _basic_22 \
	       --omit NULL_TYPE --omit NULL_PHYLUM \
	       --omit _basic_7 --omit _basic_9 --omit _basic_10 \
	       --omit parent --omit ancestor \
	       --omit REAL --omit IEEE --omit IEEEdouble --omit IEEEsingle \
	       --omit Real --omit IEEEwiden --omit IEEEnarrow \
	       --omit STRING --omit Boolean --omit String \
	       --omit '..' --omit _op_vv --omit _basic_24 \
	       --impl BOOLEAN bool --impl INTEGER int --impl IEEE double \
	       --impl CHARACTER char \
	       basic
	echo '#include "basic.handcode.h"' >> basic.h
	echo '#include "basic.handcode.cpp"' >> basic.cpp

table.cpp table.h : 
	${APSCPP} ${APSCPPFLAGS} --omit TABLE table
	echo '#include "table.handcode.h"' >> table.h

test : test-list test-set test-table
	-test-list
	-test-set
	-test-table

% : %.cpp basecpp.a
	${CPP} ${CPPFLAGS} $*.cpp basecpp.a -o $*

clean :
	rm -f *.o *.a core test-list test-set test-table
	rm -f basic.cpp table.cpp

realclean : clean
	rm -f basic.h table.h
