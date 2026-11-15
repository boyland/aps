SOURCES.grammar = grammar.aps first.aps follow.aps nullable.aps \
	first-driver.scala follow-driver.scala nullable-driver.scala
PARSER_BASENAME.grammar = Grammar
LEXER.grammar = grammar.lex
PARSER.grammar = grammar.y

SOURCES.first = grammar.aps first.aps follow.aps nullable.aps
PARSER_BASENAME.first = Grammar
LEXER.first = grammar.lex
PARSER.first = grammar.y
DRIVER.FirstDriver = grammar.aps first-driver.scala

SOURCES.follow = grammar.aps first.aps follow.aps nullable.aps
PARSER_BASENAME.follow = Grammar
LEXER.follow = grammar.lex
PARSER.follow = grammar.y
DRIVER.FollowDriver = grammar.aps follow-driver.scala

SOURCES.nullable = grammar.aps first.aps follow.aps nullable.aps
PARSER_BASENAME.nullable = Grammar
LEXER.nullable = grammar.lex
PARSER.nullable = grammar.y
DRIVER.NullableDriver = grammar.aps nullable-driver.scala

DRIVERS += FirstDriver FollowDriver NullableDriver
