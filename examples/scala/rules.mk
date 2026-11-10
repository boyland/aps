PARSER_CLASSES = $(addprefix ${OUT}/,$(addsuffix Scanner.class,${PARSER_PREFIXES}) \
	$(addsuffix Parser.class,${PARSER_PREFIXES}) $(addsuffix ParserBase.class,${PARSER_PREFIXES}) \
	$(addsuffix Tokens.class,${PARSER_PREFIXES}))
MISCGEN += $(addsuffix Parser.scala,${PARSER_PREFIXES}) $(addsuffix Tokens.scala,${PARSER_PREFIXES}) \
	$(addsuffix Scanner.scala,${PARSER_PREFIXES})

${ALL_CLASSES} ${PARSER_CLASSES}: | ${OUT}
	${SCALAC} ${SCALACFLAGS} $(filter %.scala,$^)

${EVALUATOR_CLASSES}: | ${OUT}
	${SCALAC} ${SCALACFLAGS} $(filter %.scala,$^)
	@touch $@

$(addsuffix .lex,${PARSER_PREFIXES}):
	cat $(word 2,$^) $(word 1,$^) > $@

$(addsuffix .y,${PARSER_PREFIXES}):
	cat $^ > $@

%Scanner.scala: %.lex
	java -jar ${SCALAFLEX} --scala $<

%Parser.scala %Tokens.scala &: %.y ${SCALABISON}
	bison -v $*.y
	rm $*.tab.c
	${SCALA} -classpath ${SCALABISON} edu.uwm.cs.scalabison.RunGenerator $*.y

$(addprefix ${OUT}/,$(addsuffix Scanner.class,${PARSER_PREFIXES})): ${OUT}/%Scanner.class: %Scanner.scala ${OUT}/%Tokens.class
$(addprefix ${OUT}/,$(addsuffix Parser.class,${PARSER_PREFIXES})): ${OUT}/%Parser.class: %Parser.scala ${OUT}/%Scanner.class ${OUT}/%ParserBase.class
$(addprefix ${OUT}/,$(addsuffix ParserBase.class,${PARSER_PREFIXES})): ${OUT}/%ParserBase.class: %ParserBase.scala
$(addprefix ${OUT}/,$(addsuffix Tokens.class,${PARSER_PREFIXES})): ${OUT}/%Tokens.class: %Tokens.scala
