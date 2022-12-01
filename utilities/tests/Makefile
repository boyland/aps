# include ../external.variable

CC=gcc 
LINTER_FLAGS=-Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
UTILITIES=../utilities.o
CFLAGS=-g ${LINTER_FLAGS} -I. -I.. -I../../lib
TESTS=hashcons_test.o hashtable_test.o 
EXTERNAL_DEPENDENCIES=../../lib/aps-ag.a ../../lib/aps-lib.o ../utilities.o

all: build test clean
build: ${TESTS} ${UTILITIES} ${EXTERNAL_DEPENDENCIES} driver.o 
	$(CC) $(CFLAGS) $^ -o test.out

%_test.o: %_test.h %_test.c ${UTILITIES} 
	$(CC) -c $(CFLAGS) $*_test.c

driver.o: ${TESTS} driver.c
	$(CC) -c $(CFLAGS) driver.c -o driver.o

test: build
	@./test.out

clean:
	@rm -rf *.o *.out