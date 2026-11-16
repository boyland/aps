PARSER_BASENAME = FarrowUbd

DRIVERS = FarrowUbdDriver FarrowUbdFiberDriver

DRIVER.FarrowUbdDriver = farrow-ubd.aps DYNAMIC,STATIC,FARROW farrow-ubd-driver.scala

DRIVER.FarrowUbdFiberDriver = farrow-ubd-fiber.aps DYNAMIC,STATIC farrow-ubd-fiber-driver.scala
