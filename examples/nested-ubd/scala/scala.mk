PARSER_BASENAME = NestedUbd

DRIVERS = NestedUbdDriver NestedUbdFiberDriver

DRIVER.NestedUbdDriver = nested-ubd.aps DYNAMIC,STATIC,FARROW nested-ubd-driver.scala

DRIVER.NestedUbdFiberDriver = nested-ubd-fiber.aps DYNAMIC,STATIC nested-ubd-fiber-driver.scala
