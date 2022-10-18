CC=gcc 
LINTER_FLAGS=-Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
CFLAGS=-g -I .. ${LINTER_FLAGS}

all: build test clean
build: prime.o hashcons.o hashcons_test.o hashtable.o hashtable_test.o driver.o
	$(CC) $(CFLAGS) $^ -o test.out

prime.o: ../prime.h ../prime.c
	$(CC) -c $(CFLAGS) ../prime.c -o prime.o

hashcons.o: prime.o ../hashcons.h ../hashcons.c
	$(CC) -c $(CFLAGS) prime.o ../hashcons.c -o hashcons.o

hashcons_test.o: hashcons.o hashcons_test.h hashcons_test.c
	$(CC) -c $(CFLAGS) hashcons_test.c

hashtable.o: prime.o ../hashtable.h ../hashtable.c
	$(CC) -c $(CFLAGS) prime.o ../hashtable.c -o hashtable.o

hashtable_test.o: hashtable.o hashtable_test.h hashtable_test.c
	$(CC) -c $(CFLAGS) hashtable_test.c

driver.o: hashcons_test.o hashtable_test.o driver.c
	$(CC) -c $(CFLAGS) driver.c -o driver.o

test: build
	@./test.out

clean:
	@rm -rf *.o *.out