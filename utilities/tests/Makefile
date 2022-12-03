CC=gcc 
LINTER_FLAGS=-Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
EXTERNAL_DEPENDENCIES=utilities.o
CFLAGS=-g ${LINTER_FLAGS} -I. -I..
LINKFLAGS=-lm
TESTS=hashcons_test.o hashtable_test.o stack_test.o scc_test.o 

all: build test clean
build: ${TESTS} $(addprefix ../, ${EXTERNAL_DEPENDENCIES}) common.o driver.o
	$(CC) $(CFLAGS) $^ -o test.out ${LINKFLAGS}

%_test.o: %_test.h %_test.c
	$(CC) -c $(CFLAGS) $*_test.c ${LINKFLAGS}

%.o: ${TESTS} %.c
	$(CC) -c $(CFLAGS) $*.c

test: build
	@./test.out

realclean: clean
clean:
	@rm -rf *.o *.out
