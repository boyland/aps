CC= gcc
AR= ar
CFLAGS= -Wall -shared

.PHONY: default all clean

all : utils.o
prime.o : prime.h
hashcons.o : prime.o hashcons.h
stack.o : stack.h
scc.o : stack.o scc.h
topological.o : topological.h

install: utils.o
utils.o: prime.o  hashcons.o stack.o scc.o topological.o
	$(AR) -cr $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean install

realclean: clean
clean:
	-rm -f *.o
