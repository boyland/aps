.PHONY: all

all : aps-library.jar

aps-library.jar : aps-impl.scala basic.handcode.scala
	-rm *.class
	scalac aps-impl.scala basic.handcode.scala
	jar cvf aps-library.jar *.class
