SPECS=FirstSpec FollowSpec
EXAMPLES_PATH=../..
ROOT_PATH=../${EXAMPLES_PATH}
SCALAV=2.12
APSLIB=${ROOT_PATH}/lib/aps-library-${SCALAV}.jar
SCALA_FLAGS=.:${APSLIB}
APS2SCALA=${ROOT_PATH}/bin/aps2scala

.PHONY: all
all: $(addsuffix .run, $(SPECS))

.PHONY: clean
clean:
	rm -f *.class grammar.scala first.scala follow.scala

%.class: %.scala
	scalac -cp ${SCALA_FLAGS} $<

%.scala:
	${APS2SCALA} -DCOT -p ${EXAMPLES_PATH}:${ROOT_PATH}/base $*

%.run: %.class
	scala -cp ${SCALA_FLAGS} $(basename $<)

GrammarUtil.class: grammar.class
	scalac -cp ${SCALA_FLAGS} GrammarUtil.scala

FirstSpec.class: Spec.class GrammarUtil.class first.class
	scalac -cp ${SCALA_FLAGS} FirstSpec.scala

FollowSpec.class: Spec.class GrammarUtil.class follow.class
	scalac -cp ${SCALA_FLAGS} FollowSpec.scala
