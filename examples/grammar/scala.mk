DRIVER_CLASSES += FirstDriver.class FollowDriver.class NullableDriver.class
PARSER_LEXER_Grammar = grammar.lex Grammar.lex.scala
PARSER_GRAMMAR_Grammar = grammar.y Grammar.y.scala

SOURCES_grammar = grammar.aps first.aps follow.aps nullable.aps \
	first-driver.scala follow-driver.scala nullable-driver.scala
PARSER_grammar = Grammar
FirstDriver.class FirstDriver.run FollowDriver.class FollowDriver.run \
	NullableDriver.class NullableDriver.run: MODULE = grammar
