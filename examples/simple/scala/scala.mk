PARSER_BASENAME = Simple

DRIVERS = Classic NestedCyclesDriver SimpleBindingDriver SimpleBinding1Driver \
	SimpleBinding2Driver SimpleBinding3Driver SimpleSncDriver SimpleSncFiberDriver

DRIVER.Classic = classic-binding.aps DYNAMIC,STATIC classic-driver.scala

DRIVER.NestedCyclesDriver = nested-cycles.aps DYNAMIC,STATIC nested-cycles-driver.scala

DRIVER.SimpleSncDriver = simple-snc.aps DYNAMIC,FARROW simple-snc-driver.scala

DRIVER.SimpleSncFiberDriver = simple-snc-fiber.aps DYNAMIC simple-snc-fiber-driver.scala

DRIVER.SimpleBindingDriver = simple-binding.aps DYNAMIC,STATIC simple-binding-driver.scala

DRIVER.SimpleBinding1Driver = simple-binding1.aps DYNAMIC,STATIC simple-binding-driver.scala SimpleBindingDriver

DRIVER.SimpleBinding2Driver = simple-binding2.aps DYNAMIC,STATIC simple-binding-driver.scala SimpleBindingDriver

DRIVER.SimpleBinding3Driver = simple-binding3.aps DYNAMIC,STATIC simple-binding-driver.scala SimpleBindingDriver
