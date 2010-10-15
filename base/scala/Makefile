.PHONY: all install clean

SCALA= aps-impl.scala basic.handcode.scala table.handcode.scala

all : aps-library.jar

install : aps-library.jar
	cp aps-library.jar ../../lib

aps-library.jar : ${SCALA}
	@rm -f *.class
	scalac -deprecation ${SCALA}
	jar cvf aps-library.jar *.class

clean :
	rm -f *.class aps-library.jar

%.scala : RCS/%.scala,v
	co $*.scala

Makefile : RCS/Makefile,v
	co Makefile

