CC=gcc
LINTER_FLAGS= -Wno-pointer-to-int-cast -Wno-discarded-qualifiers -Wno-unused-function -Wno-int-to-pointer-cast
CFLAGS=-Wall -g -DYYDEBUG -DYY_SKIP_YYWRAP ${LINTER_FLAGS}

APSLIBOBJS= alloc.o string.o symbol.o \
	    aps-tree.o aps-traverse.o aps-util.o \
	    aps-lex.o aps.tab.o aps-tree-dump.o
APSCOBJS= apsc.o aps-tree-dump.o aps-lib.o

aps-lib.o : ${APSLIBOBJS}
	ld -r ${APSLIBOBJS} -o aps-lib.o

aps-lex.c : aps.lex
	flex -t aps.lex > aps-lex.c

aps-tree.o : tree.i

aps2lisp : ${APSCOBJS}
	${CC} ${CFLAGS} ${APSCOBJS} -o aps2lisp

apsc.o ${APSLIBOBJS} : aps.tab.h jbb.h jbb-symbol.h jbb-string.h jbb-alloc.h

aps.tab.h aps.tab.c : aps.y
	bison -d -p aps_yy aps.y

install : aps-lib.o
	mv aps-lib.o ../lib/.

clean:
	rm -f *.o aps.tab.* aps2lisp core aps-lex.c

realclean: clean

.PHONY: all install clean realclean
