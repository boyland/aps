DRIVER_CLASSES = $(addsuffix .class,${DRIVERS})
DRIVER_RUNS = $(addsuffix .run,${DRIVERS})

driver-module = $(basename $(or $(filter %.aps,$(firstword ${DRIVER.$1})),$(error DRIVER.$1 must start with an .aps input)))

DRIVER_NAME = $(basename $@)
DRIVER_MODULE = $(call driver-module,${DRIVER_NAME})
MAIN_CLASS = $(or $(word 2,${DRIVER.${DRIVER_NAME}}),${DRIVER_NAME})

# Output paths are <evaluator>/<module>/<file>.
TARGET_MODULE = $(firstword $(subst /, ,$*))
MODULE_CONFIG ?= $(foreach dir,${AST_DIRS},$(if $(wildcard ${dir}/${TARGET_MODULE}.aps),$(wildcard ${dir}/scala.mk ${dir}/ast.mk)))
MODULE_SOURCES = $(sort ${SHARED_SOURCES} $(or ${SOURCES.${TARGET_MODULE}},$(error Declare SOURCES.${TARGET_MODULE} in scala.mk)))
MODULE_APS = $(filter %.aps,${MODULE_SOURCES})
MODULE_HANDCODE = $(filter %.scala,${MODULE_SOURCES})
MODULE_SCALA = $(addprefix ${OUT}/${TARGET_MODULE}/,$(MODULE_APS:.aps=.scala))

PARSER_FILES = $(addprefix ${PARSER_BASENAME},Scanner.scala Parser.scala Tokens.scala)
PARSER_SCALA = $(if ${PARSER_BASENAME},$(addprefix ${OUT}/${TARGET_MODULE}/,${PARSER_FILES}))
PARSER_BASE = $(addsuffix ParserBase.scala,${PARSER_BASENAME})

.PHONY: ${DRIVER_CLASSES} ${DRIVER_RUNS}
# Resolve module-specific prerequisites after Make knows the target and matched path.
.SECONDEXPANSION:

${DRIVER_CLASSES} ${DRIVER_RUNS}: ${OUT}/$$(DRIVER_MODULE)/.compiled

${DRIVER_CLASSES}:
	@test -f "${OUT}/${DRIVER_MODULE}/${MAIN_CLASS}.class" || \
	  { echo "Missing main class ${MAIN_CLASS} in ${OUT}/${DRIVER_MODULE}" >&2; exit 1; }

${DRIVER_RUNS}:
	${SCALA} ${JVMFLAGS} -cp ${OUT}/${DRIVER_MODULE}:$(abspath ${APSLIB}) ${MAIN_CLASS} ${ARGS}

${OUT}/%/.generated: $$(MODULE_SCALA)
	@touch $@

${OUT}/%/.compiled: $$(MODULE_SCALA) $$(MODULE_HANDCODE) \
	$$(PARSER_SCALA) $$(PARSER_BASE) ${APSLIB} Makefile rules.mk $$(MODULE_CONFIG)
	mkdir -p $(@D)
	find "$(@D)" -maxdepth 1 -type f -name '*.class' -delete
	${SCALAC} -cp $(abspath ${APSLIB}) -d $(@D) $(filter %.scala,$^)
	@touch $@

${OUT}/%.scala: $$(notdir $$*).aps $$(MODULE_APS) ${APS2SCALA} Makefile rules.mk $$(MODULE_CONFIG)
	mkdir -p $(@D)
	cd $(@D) && $(abspath ${APS2SCALA}) ${APS2SCALAFLAGS} \
	  $(if $(filter $(notdir $*),${TREE_MODULES}),,${EVALUATOR_FLAGS}) $(notdir $*)

${OUT}/%.lex: $$(LEXER) $$(notdir $$*).lex.scala Makefile rules.mk $$(MODULE_CONFIG)
	mkdir -p $(@D)
	cat $(filter %.lex.scala,$^) $(filter %.lex,$^) > $@

${OUT}/%.y: $$(PARSER) $$(notdir $$*).y.scala Makefile rules.mk $$(MODULE_CONFIG)
	mkdir -p $(@D)
	cat $(filter %.y,$^) $(filter %.y.scala,$^) > $@

${OUT}/%Scanner.scala: ${OUT}/%.lex ${SCALAFLEX}
	cd $(@D) && java -jar $(abspath ${SCALAFLEX}) --scala $(notdir $<)

${OUT}/%Parser.scala ${OUT}/%Tokens.scala &: ${OUT}/%.y ${SCALABISON}
	cd $(@D) && bison -v $(notdir $<)
	rm $(basename $<).tab.c
	cd $(@D) && ${SCALA} -classpath $(abspath ${SCALABISON}) \
	  edu.uwm.cs.scalabison.RunGenerator $(notdir $<)
