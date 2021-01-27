APSTOP= ../..
CPP = g++
CPPFLAGS = -ftemplate-depth=63 -I ${APSTOP}/base/cpp
APSCPP = ${APSTOP}/bin/apscpp
APSCPPFLAGS = -p ..:${APSTOP}/base -G
BASECPP = ${APSTOP}/lib/basecpp.a

all : test-driver tiny-driver test-coll-driver \
	classic-driver dynamic-driver simple-driver

.PHONY: all clean

%.h %.cpp : ../%.aps 
	${APSCPP} ${APSCPPFLAGS} $*

%.o : %.cpp
	${CPP} ${CPPFLAGS} -c $*.cpp

%-driver : %-driver.cpp %.o
	${CPP} ${CPPFLAGS} $*-driver.cpp $*.o ${BASECPP} -o $*-driver

classic-binding.o : simple.h 
classic-driver.o : simple.h classic-binding.h
classic-driver : classic-driver.o classic-binding.o simple.o
	${CPP} ${CPPFLAGS} classic-driver.o classic-binding.o simple.o ${BASECPP} -o $@

dynamic-binding.o : simple.h
dynamic-driver.o : simple.h dynamic-binding.h
dynamic-driver : dynamic-driver.o dynamic-binding.o simple.o
	${CPP} ${CPPFLAGS} dynamic-driver.o dynamic-binding.o simple.o ${BASECPP} -o $@

simple-binding3.h simple-binding3.cpp : ../simple-binding3.aps
	${APSCPP} ${APSCPPFLAGS} -S simple-binding3
simple-driver.o : simple.h simple-binding3.h
simple-driver : simple-driver.o simple-binding3.o simple.o
	${CPP} ${CPPFLAGS} simple-driver.o simple-binding3.o simple.o ${BASECPP} -o $@

test-coll-driver.o : tiny.h test-coll.h
test-coll-driver: test-coll-driver.o tiny.o test-coll.o
	${CPP} ${CPPFLAGS} test-coll-driver.o tiny.o test-coll.o ${BASECPP} -o $@

APSMODS = test.aps tiny.aps test-coll.aps test-cicular.aps \
	  simple.aps \
	  simple-binding3.aps dynamic-binding.aps classic-binding.aps
APSCLEAN = ${APSMODS:.aps=.h} ${APSMODS:.aps=.cpp}

clean:
	rm -f core *.o *driver
	rm -f test test-tiny classic-driver dynamic-driver
	rm -f ${APSCLEAN}
