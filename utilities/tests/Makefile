CC=gcc 
CFLAGS=-g -I ..

all: build test clean
build: prime.o hashcons.o hashcons_test.o driver.o
	$(CC) $(CFLAGS) $^ -o test.out

prime.o: ../prime.h ../prime.c
	$(CC) -c $(CFLAGS) ../prime.c -o prime.o

hashcons.o: prime.o ../hashcons.h ../hashcons.c
	$(CC) -c $(CFLAGS) prime.o ../hashcons.c -o hashcons.o

hashcons_test.o: hashcons.o hashcons_test.h hashcons_test.c
	$(CC) -c $(CFLAGS) hashcons_test.c

driver.o: hashcons_test.o driver.c
	$(CC) -c $(CFLAGS) driver.c -o driver.o

test: build
	@./test.out

clean:
	@rm -rf *.o *.out