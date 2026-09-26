SPECS=FirstSpec FollowSpec NullableSpec NTupleLatticeSpec TestSynthSpec
EXAMPLES_PATH=../..
ROOT_PATH=../${EXAMPLES_PATH}
AST_PATHS=${EXAMPLES_PATH}/tiny:${EXAMPLES_PATH}/simple:${EXAMPLES_PATH}/grammar:${EXAMPLES_PATH}/farrow-lv:${EXAMPLES_PATH}/farrow-ubd:${EXAMPLES_PATH}/nested-ubd:${EXAMPLES_PATH}/balanced:${EXAMPLES_PATH}/bigger:${EXAMPLES_PATH}/type:${EXAMPLES_PATH}/conditional:${EXAMPLES_PATH}/standalone
TEST_SOURCE_DIRS=${EXAMPLES_PATH}/tiny/scala/tests ${EXAMPLES_PATH}/grammar/scala/tests
SCALAV=2.13
APSLIB=${ROOT_PATH}/lib/aps-library-${SCALAV}.jar
SCALA_FLAGS=.:${APSLIB}
APS2SCALA=${ROOT_PATH}/bin/aps2scala

vpath %.scala ${TEST_SOURCE_DIRS}

.PHONY: all
all: $(addsuffix .run, $(SPECS))

.PHONY: clean
clean:
	rm -f *.class grammar.scala first.scala follow.scala tiny.scala ntuple-lattice.scala test-synth.scala nullable.scala

%.class: %.scala
	scalac -cp ${SCALA_FLAGS} $<

first.scala: ${EXAMPLES_PATH}/grammar/first.aps
follow.scala: ${EXAMPLES_PATH}/grammar/follow.aps
nullable.scala: ${EXAMPLES_PATH}/grammar/nullable.aps
ntuple-lattice.scala: ${EXAMPLES_PATH}/tiny/ntuple-lattice.aps
first.scala follow.scala nullable.scala ntuple-lattice.scala:
	${APS2SCALA} -DCOT -C -p ${AST_PATHS}:${ROOT_PATH}/base -S $(basename $(@F))

tiny.scala: ${EXAMPLES_PATH}/tiny/tiny.aps
grammar.scala: ${EXAMPLES_PATH}/grammar/grammar.aps
test-synth.scala: ${EXAMPLES_PATH}/tiny/test-synth.aps
tiny.scala grammar.scala test-synth.scala:
	${APS2SCALA} -DCOT -p ${AST_PATHS}:${ROOT_PATH}/base $(basename $(@F))

%.run: %.class
	scala -cp ${SCALA_FLAGS} $(basename $<)

GrammarUtil.class: GrammarUtil.scala grammar.class
	scalac -cp ${SCALA_FLAGS} $<

FirstSpec.class: FirstSpec.scala Spec.class GrammarUtil.class first.class
	scalac -cp ${SCALA_FLAGS} $<

FollowSpec.class: FollowSpec.scala Spec.class GrammarUtil.class follow.class
	scalac -cp ${SCALA_FLAGS} $<

NullableSpec.class: NullableSpec.scala Spec.class GrammarUtil.class nullable.class
	scalac -cp ${SCALA_FLAGS} $<

NTupleLatticeSpec.class: NTupleLatticeSpec.scala Spec.class tiny.class ntuple-lattice.class
	scalac -cp ${SCALA_FLAGS} $<

TestSynthSpec.class: TestSynthSpec.scala Spec.class tiny.class test-synth.class
	scalac -cp ${SCALA_FLAGS} $<
