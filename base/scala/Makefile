.PHONY: all install clean

SCALA= aps-impl.scala basic.handcode.scala table.handcode.scala
SVERSION=2.10

all : aps-library-2.9.jar aps-library-2.10.jar aps-library-2.11.jar

install : aps-library-2.9.jar aps-library-2.10.jar aps-library-2.11.jar
	cp aps-library*.jar ../../lib

aps-library-2.9.jar : ${SCALA}
	@rm -f *.class
	/afs/cs.uwm.edu/package/scala/scala-2.9.1.final/common/bin/scalac -deprecation ${SCALA}
	jar cvf $@ *.class

aps-library-2.10.jar : ${SCALA}
	@rm -f *.class
	sed 's/register/register.asInstanceOf[T_Result]/' < basic.handcode.scala > basic.handcode-2.10.scala
	/afs/cs.uwm.edu/package/scala/scala-2.10.5/common/bin/scalac -deprecation `echo ${SCALA} | sed 's/basic.handcode/basic.handcode-2.10/'`
	jar cvf $@ *.class
	rm -f  basic.handcode-2.10.scala

aps-library-2.11.jar : ${SCALA}
	@rm -f *.class
	/afs/cs.uwm.edu/package/scala/scala-2.11.7/common/bin/scalac -deprecation ${SCALA}
	jar cvf $@ *.class

.PHONY: test-multiset.run
test-multiset.run : aps-library-${SVERSION}.jar
	scalac -cp aps-library-${SVERSION}.jar test-multiset.scala
	scala -cp aps-library-${SVERSION}.jar:. TestMultiSet

clean :
	rm -f *.class *.jar *-2.*.scala

%.scala : RCS/%.scala,v
	co $*.scala

Makefile : RCS/Makefile,v
	co Makefile

