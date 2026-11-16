DRIVER_CLASSES = $(addsuffix .class,${DRIVERS})
DRIVER_RUNS = $(addsuffix .run,${DRIVERS})
MODULES = $(sort $(basename $(filter %.aps,${AST} ${SHARED_SOURCES})) \
	$(foreach driver,${DRIVERS},$(call driver-module,$(driver))))
GENERATE_TARGETS = $(addprefix ${OUT}/,$(addsuffix /generate,${MODULES}))

DRIVER_NAME = $(basename $@)
DRIVER_MODULE = $(call driver-module,${DRIVER_NAME})
MAIN_CLASS = $(call driver-main,${DRIVER_NAME})

# Output paths are <evaluator>/<module>/<file>.
TARGET_MODULE = $(firstword $(subst /, ,$*))
MODULE_CONFIG ?= Makefile ast.mk scala/scala.mk
BUILD_CONFIG = ../scala/Makefile ../scala/rules.mk ../common.mk ${MODULE_CONFIG}
MODULE_INPUT = $(if $(filter ${TARGET_MODULE},${MODULES}),${TARGET_MODULE}.aps,$(error No Scala inputs declared for module ${TARGET_MODULE}))
MODULE_DRIVERS = $(foreach driver,${DRIVERS},$(if $(filter ${TARGET_MODULE},$(call driver-module,$(driver))),$(driver)))
MODULE_SOURCES = $(sort ${AST} ${SHARED_SOURCES} ${MODULE_INPUT} \
	$(foreach driver,${MODULE_DRIVERS},$(call driver-sources,$(driver))))
MODULE_APS = $(filter %.aps,${MODULE_SOURCES})
MODULE_HANDCODE = $(filter %.scala,${MODULE_SOURCES})
MODULE_SCALA = $(addprefix ${OUT}/${TARGET_MODULE}/,$(MODULE_APS:.aps=.scala))

PARSER_FILES = $(addprefix ${PARSER_BASENAME},Scanner.scala Parser.scala Tokens.scala)
PARSER_SCALA = $(if ${PARSER_BASENAME},$(addprefix ${OUT}/${TARGET_MODULE}/,${PARSER_FILES}))
PARSER_BASE = $(addsuffix ParserBase.scala,${PARSER_BASENAME})

.PHONY: ${DRIVER_CLASSES} ${DRIVER_RUNS} ${GENERATE_TARGETS}
# Resolve module-specific prerequisites after Make knows the target and matched path.
.SECONDEXPANSION:

${DRIVER_CLASSES} ${DRIVER_RUNS}: ${OUT}/$$(DRIVER_MODULE)/.compiled

${DRIVER_CLASSES}:
	@test -f "${OUT}/${DRIVER_MODULE}/${MAIN_CLASS}.class" || \
	  { echo "Missing main class ${MAIN_CLASS} in ${OUT}/${DRIVER_MODULE}" >&2; exit 1; }

${DRIVER_RUNS}:
	${SCALA} ${JVMFLAGS} -cp ${OUT}/${DRIVER_MODULE}:$(abspath ${APSLIB}) ${MAIN_CLASS} ${ARGS}

${GENERATE_TARGETS}: ${OUT}/%/generate: $$(MODULE_SCALA)

${OUT}/%/.compiled: $$(MODULE_SCALA) $$(MODULE_HANDCODE) \
	$$(PARSER_SCALA) $$(PARSER_BASE) ${APSLIB} $$(BUILD_CONFIG)
	mkdir -p $(@D)
	find "$(@D)" -maxdepth 1 -type f -name '*.class' -delete
	${SCALAC} -cp $(abspath ${APSLIB}) -d $(@D) $(filter %.scala,$^)
	@touch $@

${OUT}/%.scala: $$(notdir $$*).aps $$(MODULE_APS) ${APS2SCALA} $$(BUILD_CONFIG)
	mkdir -p $(@D)
	cd $(@D) && $(abspath ${APS2SCALA}) ${APS2SCALAFLAGS} \
	  $(if $(filter $(notdir $*),${TREE_MODULES}),,${EVALUATOR_FLAGS}) $(notdir $*)

${OUT}/%.lex: $$(LEXER) $$(notdir $$*).lex.scala $$(BUILD_CONFIG)
	mkdir -p $(@D)
	cat $(filter %.lex.scala,$^) $(filter %.lex,$^) > $@

${OUT}/%.y: $$(PARSER) $$(notdir $$*).y.scala $$(BUILD_CONFIG)
	mkdir -p $(@D)
	cat $(filter %.y,$^) $(filter %.y.scala,$^) > $@

${OUT}/%Scanner.scala: ${OUT}/%.lex ${SCALAFLEX}
	cd $(@D) && java -jar $(abspath ${SCALAFLEX}) --scala $(notdir $<)

${OUT}/%Parser.scala ${OUT}/%Tokens.scala &: ${OUT}/%.y ${SCALABISON}
	cd $(@D) && bison -v $(notdir $<)
	rm $(basename $<).tab.c
	cd $(@D) && ${SCALA} -classpath $(abspath ${SCALABISON}) \
	  edu.uwm.cs.scalabison.RunGenerator $(notdir $<)
