CC = gcc
LIBS = -lm
CFLAGS = -g -Wall

.PHONY: default all clean

all : prime.o  hashcons.o stack.o scc.o topological-sort.o
prime.o : prime.h
hashcons.o : prime.o hashcons.h
stack.o : stack.h
scc.o : stack.o scc.h
topological-sort.o : topological-sort.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
