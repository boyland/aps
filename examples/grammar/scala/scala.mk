SHARED_SOURCES = first.aps follow.aps nullable.aps
PARSER_BASENAME = Grammar

DRIVERS = FirstDriver FollowDriver NullableDriver

DRIVER.FirstDriver = grammar.aps DYNAMIC,STATIC first-driver.scala

DRIVER.FollowDriver = grammar.aps DYNAMIC,STATIC follow-driver.scala

DRIVER.NullableDriver = grammar.aps DYNAMIC,STATIC nullable-driver.scala
