APSTOP= ../..
SCALAC= scalac
APSLIB = ${APSTOP}/lib/aps-library.jar
SCALAFLAGS= -cp .:${APSLIB}
SCALACFLAGS= ${SCALAFLAGS}
APS2SCALA = ${APSTOP}/bin/aps2scala
APS2SCALAFLAGS = -p ..:${APSTOP}/base -G

SCALAGEN = simple.scala classic-binding.scala

.PHONY: all clean

all : ${SCALAGEN}
all : simple_implicit.class classic_binding_implicit.class Classic.class

%.scala : ../%.aps 
	${APS2SCALA} ${APS2SCALAFLAGS} $*

%_implicit.class : %.scala
	${SCALAC} ${SCALACFLAGS} $*.scala

classic_binding_implicit.class : classic-binding.scala
	${SCALAC} ${SCALACFLAGS} $<

Classic.class : classic-driver.scala
	${SCALAC} ${SCALACFLAGS} $<

Classic.class   classic_binding_implicit.class : simple_implicit.class
Classic.class : classic_binding_implicit.class

test_coll_implicit.class : test-coll.scala
	${SCALAC} ${SCALACFLAGS} $<

TestCollDriver.class : test_coll_implicit.class test-coll-driver.scala
	${SCALAC} ${SCALACFLAGS}  test-coll-driver.scala

.PHONY: %.run

%.run : %.class
	scala ${SCALAFLAGS} $*

clean:
	rm -f *.class ${SCALAGEN}

