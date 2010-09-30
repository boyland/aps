.PHONY: all install clean

all : aps-library.jar

install : aps-library.jar
	cp aps-library.jar ../../lib

aps-library.jar : aps-impl.scala basic.handcode.scala
	@rm -f *.class
	scalac -deprecation aps-impl.scala basic.handcode.scala
	jar cvf aps-library.jar *.class

clean :
	rm -f *.class aps-library.jar
