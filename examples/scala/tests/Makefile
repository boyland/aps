SPECS=first.spec.scala follow.spec.scala
TREES=grammar.scala
UTILS=grammar.util.scala
EXAMPLES_PATH=../..
ROOT_PATH=../${EXAMPLES_PATH}
SCALAV=2.12
APSLIB=${ROOT_PATH}/lib/aps-library-${SCALAV}.jar
APS2SCALA=${ROOT_PATH}/bin/aps2scala

%.build:
	@scalac -cp ${APSLIB} Spec.scala ${UTILS} ${TREES} $* $(basename $(basename $*)).scala

%.generate:
	@${APS2SCALA} -DCOT -p ${EXAMPLES_PATH}:${ROOT_PATH}/base $(basename $(basename $*))

all: $(addsuffix .generate, $(TREES)) $(addsuffix .run, $(SPECS)) 

first.spec.scala.run: first.spec.scala.run2 
	# FirstSpec is the name of class
	@scala -cp .:${APSLIB} FirstSpec

follow.spec.scala.run: follow.spec.scala.run2 
	# FollowSpec is the name of class
	@scala -cp .:${APSLIB} FollowSpec

%.run2: %.generate %.build
	@echo "Run2"

clean:
	rm -f *.class grammar.scala first.scala follow.scala