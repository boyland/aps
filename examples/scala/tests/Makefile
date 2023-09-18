SPECS=FirstSpec FollowSpec NullableSpec NTupleLatticeSpec
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
	rm -f *.class grammar.scala first.scala follow.scala tiny.scala ntuple-lattice.scala

%.class: %.scala
	scalac -cp ${SCALA_FLAGS} $<

first.scala follow.scala nullable.scala ntuple-lattice.scala:
	${APS2SCALA} -DCOT -C -p ${EXAMPLES_PATH}:${ROOT_PATH}/base -S $(basename $(@F))

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

NullableSpec.class: Spec.class GrammarUtil.class nullable.class
	scalac -cp ${SCALA_FLAGS} NullableSpec.scala

NTupleLatticeSpec.class: Spec.class tiny.class ntuple-lattice.class
	scalac -cp ${SCALA_FLAGS} NTupleLatticeSpec.scala
