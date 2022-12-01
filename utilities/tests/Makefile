CC=gcc 
LINTER_FLAGS=-Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
EXTERNAL_DEPENDENCIES=prime.o hashcons.o hashtable.o
CFLAGS=-g ${LINTER_FLAGS} -I. -I..
TESTS=hashcons_test.o hashtable_test.o 

all: build test clean
build: ${TESTS} $(addprefix ../, ${EXTERNAL_DEPENDENCIES}) driver.o
	$(CC) $(CFLAGS) $^ -o test.out

%_test.o: %_test.h %_test.c
	$(CC) -c $(CFLAGS) $*_test.c

driver.o: ${TESTS} driver.c
	$(CC) -c $(CFLAGS) driver.c -o driver.o

test: build
	@./test.out

clean:
	@rm -rf *.o *.out
