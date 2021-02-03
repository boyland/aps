CC=gcc 
CFLAGS=-I ..

all: output test clean
output: driver.o
	$(CC) $(CFLAGS) hashcons_test.o prime.o hashcons.o driver.o -o output

prime.o: ../prime.h ../prime.c
	$(CC) -c $(CFLAGS) ../prime.c -o prime.o

hashcons.o: prime.o ../hashcons.h ../hashcons.c
	$(CC) -c $(CFLAGS) ../hashcons.h -o hashcons.o

hashcons_test.o: hashcons.o hashcons_test.c
	$(CC) -c $(CFLAGS) hashcons_test.c

driver.o: hashcons_test.o
	$(CC) -c $(CFLAGS) driver.c -o driver.o

test: output
	@./output

clean:
	@rm -rf *.o output