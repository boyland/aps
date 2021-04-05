SPECS=First Follow
EXAMPLES_PATH=../..
ROOT_PATH=../${EXAMPLES_PATH}
SCALAV=2.12
APSLIB=${ROOT_PATH}/lib/aps-library-${SCALAV}.jar
SCALA_FLAGS=.:${APSLIB}
APS2SCALA=${ROOT_PATH}/bin/aps2scala

.PHONY:
all: $(addsuffix Spec.run, $(SPECS))

%.class: %.scala
	scalac -cp ${SCALA_FLAGS} $<

%.generate:
	${APS2SCALA} -DCOT -p ${EXAMPLES_PATH}:${ROOT_PATH}/base $*

%.aps: %.generate %.class
	echo "Building aps"

%.run: %.class
	@scala -cp ${SCALA_FLAGS} $<

FirstSpec.compile: Spec.class GrammarUtil.compile first.aps FirstSpec.class 
	echo "Building FirstSpec.scala"

FollowSpec.compile: Spec.class GrammarUtil.compile follow.aps FollowSpec.class
	echo "Building FollowSpec.scala"

GrammarUtil.compile: grammar.aps grammar.class GrammarUtil.class
	echo "Building GrammarUtil.scala"

.PHONY:
clean:
	rm -f *.class grammar.scala first.scala follow.scala
