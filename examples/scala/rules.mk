PARSER_CLASSES = $(addprefix ${OUT}/,$(addsuffix Scanner.class,${PARSERS}) \
	$(addsuffix Parser.class,${PARSERS}) $(addsuffix ParserBase.class,${PARSERS}) \
	$(addsuffix Tokens.class,${PARSERS}))
MISCGEN += $(addsuffix Parser.scala,${PARSERS}) $(addsuffix Tokens.scala,${PARSERS}) \
	$(addsuffix Scanner.scala,${PARSERS})

${ALL_CLASSES} ${PARSER_CLASSES}: | ${OUT}
	${SCALAC} ${SCALACFLAGS} $(filter %.scala,$^)

${EVALUATOR_CLASSES}: | ${OUT}
	${SCALAC} ${SCALACFLAGS} $(filter %.scala,$^)
	@touch $@

$(addsuffix .lex,${PARSERS}):
	cat $(word 2,$^) $(word 1,$^) > $@

$(addsuffix .y,${PARSERS}):
	cat $^ > $@

%Scanner.scala: %.lex
	java -jar ${SCALAFLEX} --scala $<

%Parser.scala %Tokens.scala &: %.y ${SCALABISON}
	bison -v $*.y
	rm $*.tab.c
	${SCALA} -classpath ${SCALABISON} edu.uwm.cs.scalabison.RunGenerator $*.y

$(addprefix ${OUT}/,$(addsuffix Scanner.class,${PARSERS})): ${OUT}/%Scanner.class: %Scanner.scala ${OUT}/%Tokens.class
$(addprefix ${OUT}/,$(addsuffix Parser.class,${PARSERS})): ${OUT}/%Parser.class: %Parser.scala ${OUT}/%Scanner.class ${OUT}/%ParserBase.class
$(addprefix ${OUT}/,$(addsuffix ParserBase.class,${PARSERS})): ${OUT}/%ParserBase.class: %ParserBase.scala
$(addprefix ${OUT}/,$(addsuffix Tokens.class,${PARSERS})): ${OUT}/%Tokens.class: %Tokens.scala
