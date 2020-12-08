../../bin/aps2scala -p ..:../../base -G tiny
../../bin/aps2scala -S -p ..:../../base -G tiny-circular-simple
scalac -cp .:../../lib/aps-library-2.12.jar tiny.scala tiny-circular-simple.scala tiny-circular-driver.scala
scala -cp .:../../lib/aps-library-2.12.jar TinyCircularDriver
