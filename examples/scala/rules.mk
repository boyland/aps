DRIVERS = $(patsubst DRIVER.%,%,$(filter DRIVER.%,${.VARIABLES}))
DRIVER_CLASSES = $(addsuffix .class,${DRIVERS})
DRIVER_RUNS = $(addsuffix .run,${DRIVERS})

driver-input = $(or $(filter %.aps,$(firstword ${DRIVER.$1})),$(error DRIVER.$1 must start with an .aps input))
driver-module = $(basename $(call driver-input,$1))

DRIVER_NAME = $(basename $@)
MODULE = $(call driver-module,${DRIVER_NAME})
MAIN_CLASS = $(or $(word 3,${DRIVER.${DRIVER_NAME}}),${DRIVER_NAME})

# Output paths are <evaluator>/<module>/<file>.
MODULE_NAME = $(firstword $(subst /, ,$*))
MODULE_INPUT = $(wildcard $(addsuffix /${MODULE_NAME}.aps,${AST_DIRS}))
MODULE_FOLDER = $(patsubst %/${MODULE_NAME}.aps,%,${MODULE_INPUT})
MODULE_GROUP = $(notdir ${MODULE_FOLDER})
MODULE_DRIVERS = $(foreach driver,${DRIVERS},\
	$(if $(filter ${MODULE_NAME},$(call driver-module,${driver})),${driver}))
MODULE_AST = $(or ${AST.${MODULE_GROUP}},${MODULE_GROUP}.aps)
MODULE_SOURCES = $(sort ${MODULE_NAME}.aps ${MODULE_AST} ${SOURCES.${MODULE_GROUP}} \
	$(foreach driver,${MODULE_DRIVERS},$(word 2,${DRIVER.${driver}})))
MODULE_APS = $(filter %.aps,${MODULE_SOURCES})
MODULE_HANDCODE = $(filter %.scala,${MODULE_SOURCES})
MODULE_SCALA = $(addprefix ${OUT}/${MODULE_NAME}/,$(MODULE_APS:.aps=.scala))

PARSER = ${PARSER.${MODULE_GROUP}}
PARSER_FILES = $(addprefix ${PARSER},Scanner.scala Parser.scala Tokens.scala)
PARSER_SCALA = $(if ${PARSER},$(addprefix ${OUT}/${MODULE_NAME}/,${PARSER_FILES}))
PARSER_BASE = $(addsuffix ParserBase.scala,${PARSER})

.PHONY: ${DRIVER_CLASSES} ${DRIVER_RUNS}
# Resolve module-specific prerequisites after Make knows the target and matched path.
.SECONDEXPANSION:

${DRIVER_CLASSES} ${DRIVER_RUNS}: ${OUT}/$$(MODULE)/.compiled

${DRIVER_CLASSES}:
	@test -f "${OUT}/${MODULE}/${MAIN_CLASS}.class" || \
	  { echo "Missing main class ${MAIN_CLASS} in ${OUT}/${MODULE}" >&2; exit 1; }

${DRIVER_RUNS}:
	${SCALA} ${JVMFLAGS} -cp ${OUT}/${MODULE}:$(abspath ${APSLIB}) ${MAIN_CLASS} ${ARGS}

${OUT}/%/.generated: $$(MODULE_SCALA)
	@touch $@

${OUT}/%/.compiled: $$(MODULE_SCALA) $$(MODULE_HANDCODE) \
	$$(PARSER_SCALA) $$(PARSER_BASE) ${MAKEFILE_LIST}
	mkdir -p $(@D)
	find "$(@D)" -maxdepth 1 -type f -name '*.class' -delete
	${SCALAC} -cp $(abspath ${APSLIB}) -d $(@D) $(filter %.scala,$^)
	@touch $@

${OUT}/%.scala: $$(notdir $$*).aps $$(MODULE_APS) ${APS2SCALA} ${MAKEFILE_LIST}
	mkdir -p $(@D)
	cd $(@D) && $(abspath ${APS2SCALA}) ${APS2SCALAFLAGS} \
	  $(if $(filter $(notdir $*),${TREE_MODULES}),,${EVALUATOR_FLAGS}) $(notdir $*)

${OUT}/%.lex: $$(MODULE_GROUP).lex $$(notdir $$*).lex.scala
	mkdir -p $(@D)
	cat $(filter %.lex.scala,$^) $(filter %.lex,$^) > $@

${OUT}/%.y: $$(MODULE_GROUP).y $$(notdir $$*).y.scala
	mkdir -p $(@D)
	cat $(filter %.y,$^) $(filter %.y.scala,$^) > $@

${OUT}/%Scanner.scala: ${OUT}/%.lex ${SCALAFLEX}
	cd $(@D) && java -jar $(abspath ${SCALAFLEX}) --scala $(notdir $<)

${OUT}/%Parser.scala ${OUT}/%Tokens.scala &: ${OUT}/%.y ${SCALABISON}
	cd $(@D) && bison -v $(notdir $<)
	rm $(basename $<).tab.c
	cd $(@D) && ${SCALA} -classpath $(abspath ${SCALABISON}) \
	  edu.uwm.cs.scalabison.RunGenerator $(notdir $<)
