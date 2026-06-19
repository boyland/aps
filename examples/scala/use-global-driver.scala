object UseGlobal
{
	def main(args : Array[String]) : Unit = {
		if (args.length == 0) {
			println("Usage: UseGlobal <file.program>");
			System.exit(1);
		} else {
			val debug = args.contains("--debug");
			for (arg <- args if arg != "--debug") {
				doWithFile(arg, debug)
			};
		}
	};

	def doWithFile(filename : String, debug : Boolean) : Unit = {
		val t = new M_TINY("Tiny");
		val p = new TinyParser(t);
		val r = p.parseFile(filename);
		t.finish();
		val m = new M_USE_GLOBAL("UseGlobal",t);
		val r2 = r.asInstanceOf[m.t_Result.T_Root];
		if (debug) Debug.activate();
		m.finish();
		println("Results:");
		println("done is " + m.v_done(r2));
	};
}
