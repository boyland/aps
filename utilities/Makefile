CC= gcc
AR= ar
CFLAGS= -shared

.PHONY: default all clean

all : utilities.o
prime.o : prime.h
hashcons.o : prime.o hashcons.h
stack.o : stack.h
scc.o : stack.o scc.h
topological.o : stack.o topological.h
hashtable.o : prime.o hashtable.h

install: utilities.o
utilities.o: prime.o hashcons.o stack.o scc.o topological.o hashtable.o
	$(AR) -cr $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean install

realclean: clean
clean:
	-rm -f *.o
