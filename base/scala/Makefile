.PHONY: all install clean

SCALA= aps-impl.scala basic.handcode.scala table.handcode.scala symbol.scala symbol.handcode.scala flat.scala
SVERSION=2.12
LIB=../../lib

# We can't make 2.9 anymore because it doesn't understand Java 8

all : aps-library-2.10.jar aps-library-2.11.jar aps-library-2.12.jar

test: test-multiset.run test-set.run test-flat.run

flat.scala: ../flat.aps ../../bin/aps2scala
	../../bin/aps2scala -p .. flat

install : 
	mkdir -p ${LIB} && cp aps-library*.jar ${LIB}

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
	/afs/cs.uwm.edu/package/scala/scala-2.11.12/common/bin/scalac -deprecation ${SCALA}
	jar cvf $@ *.class

aps-library-2.12.jar : ${SCALA}
	@rm -f *.class
	/afs/cs.uwm.edu/package/scala/scala-2.12.8/common/bin/scalac -deprecation ${SCALA}
	jar cvf $@ *.class

.PHONY: test-%.run
test-%.run : test-%.scala aps-library-${SVERSION}.jar
	scala -cp aps-library-${SVERSION}.jar $<

aps-library-2.13.jar : ${SCALA}
	@rm -f *.class
	/afs/cs.uwm.edu/package/scala/scala-2.13.7/common/bin/scalac -deprecation ${SCALA}
	jar cvf $@ *.class

aps-library.jar : ${SCALA}
	@rm -f *.class
	scalac -deprecation ${SCALA}
	jar cvf $@ *.class

clean :
	rm -f *.class *.jar *-2.*.scala

