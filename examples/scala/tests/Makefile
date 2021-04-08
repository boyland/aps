SPECS=First Follow
EXAMPLES_PATH=../..
ROOT_PATH=../${EXAMPLES_PATH}
SCALAV=2.12
APSLIB=${ROOT_PATH}/lib/aps-library-${SCALAV}.jar
SCALA_FLAGS=.:${APSLIB}
APS2SCALA=${ROOT_PATH}/bin/aps2scala

.PHONY: all
all: $(addsuffix Spec.run, $(SPECS))

.PHONY: clean
clean:
	rm -f *.class grammar.scala first.scala follow.scala

%.class: %.scala
	scalac -cp ${SCALA_FLAGS} $<

%.scala:
	${APS2SCALA} -DCOT -p ${EXAMPLES_PATH}:${ROOT_PATH}/base $*

%.aps: %.class
	echo "Needed so Makefile does not break"

%.run: %.class
	scala -cp ${SCALA_FLAGS} $(basename $<)

GrammarUtil.class: GrammarUtil.scala grammar.aps
	scalac -cp ${SCALA_FLAGS} $<

FirstSpec.class: FirstSpec.scala Spec.class GrammarUtil.class first.aps
	scalac -cp ${SCALA_FLAGS} $<

FollowSpec.class: FollowSpec.scala Spec.class GrammarUtil.class follow.aps
 	scalac -cp ${SCALA_FLAGS} $<
