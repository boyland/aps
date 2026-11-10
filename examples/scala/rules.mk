MODULE_NAME = $(firstword $(subst /, ,$*))
MODULE_SOURCES = $(or ${SOURCES_${MODULE_NAME}},${MODULE_NAME}.aps)
MODULE_PARSER = ${PARSER_${MODULE_NAME}}
MODULE_APS = $(filter %.aps,${MODULE_SOURCES})
MODULE_HANDCODE = $(filter %.scala,${MODULE_SOURCES})
MODULE_SCALA = $(addprefix ${OUT}/${MODULE_NAME}/,$(notdir $(patsubst %.aps,%.scala,$(filter %.aps,${MODULE_SOURCES}))))
PARSER_SCALA = $(addprefix ${OUT}/${MODULE_NAME}/,$(addsuffix Scanner.scala,${MODULE_PARSER}) \
	$(addsuffix Parser.scala,${MODULE_PARSER}) $(addsuffix Tokens.scala,${MODULE_PARSER}))
DRIVER_RUNS = $(patsubst %.class,%.run,${DRIVER_CLASSES})

.PHONY: ${DRIVER_CLASSES} ${DRIVER_RUNS}
.SECONDEXPANSION:

${DRIVER_CLASSES} ${DRIVER_RUNS}: ${OUT}/$$(or $$(MODULE),$$(error No module configured for $$@))/.compiled

${DRIVER_CLASSES}:
	@test -f "${OUT}/${MODULE}/${MAIN_CLASS}.class" || \
	  { echo "Missing main class ${MAIN_CLASS} in ${OUT}/${MODULE}" >&2; exit 1; }

${DRIVER_RUNS}:
	${SCALA} ${JVMFLAGS} -cp ${OUT}/${MODULE}:$(abspath ${APSLIB}) ${MAIN_CLASS} ${ARGS}

${OUT}/%/.generated: $$(MODULE_SCALA)
	@touch $@

${OUT}/%/.compiled: $$(MODULE_SCALA) $$(MODULE_HANDCODE) \
	$$(PARSER_SCALA) $$(addsuffix ParserBase.scala,$$(MODULE_PARSER)) ${MAKEFILE_LIST}
	mkdir -p $(@D)
	find "$(@D)" -maxdepth 1 -type f -name '*.class' -delete
	${SCALAC} -cp $(abspath ${APSLIB}) -d $(@D) $(filter %.scala,$^)
	@touch $@

${OUT}/%.scala: $$(notdir $$*).aps $$(MODULE_APS) ${APS2SCALA} ${MAKEFILE_LIST}
	mkdir -p $(@D)
	cd $(@D) && $(abspath ${APS2SCALA}) ${APS2SCALAFLAGS} \
	  $(if $(filter $(notdir $*),${TREE_MODULES}),,${EVALUATOR_FLAGS}) $(notdir $*)

${OUT}/%.lex: $$(or $$(PARSER_LEXER_$$(notdir $$*)),$$(error No lexer inputs configured for $$(notdir $$*)))
	mkdir -p $(@D)
	cat $(filter %.lex.scala,$^) $(filter %.lex,$^) > $@

${OUT}/%.y: $$(or $$(PARSER_GRAMMAR_$$(notdir $$*)),$$(error No grammar inputs configured for $$(notdir $$*)))
	mkdir -p $(@D)
	cat $(filter %.y,$^) $(filter %.y.scala,$^) > $@

${OUT}/%Scanner.scala: ${OUT}/%.lex ${SCALAFLEX}
	cd $(@D) && java -jar $(abspath ${SCALAFLEX}) --scala $(notdir $<)

${OUT}/%Parser.scala ${OUT}/%Tokens.scala &: ${OUT}/%.y ${SCALABISON}
	cd $(@D) && bison -v $(notdir $<)
	rm $(basename $<).tab.c
	cd $(@D) && ${SCALA} -classpath $(abspath ${SCALABISON}) \
	  edu.uwm.cs.scalabison.RunGenerator $(notdir $<)
