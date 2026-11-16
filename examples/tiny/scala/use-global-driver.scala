object UseGlobal
{
	def main(args : Array[String]) : Unit = {
		val debug = args.contains("--debug");
		val files = args.filter(_ != "--debug")
		if (files.isEmpty) {
			doWith("3,(1,4)", debug)
		} else {
			for (arg <- files) {
				doWithFile(arg, debug)
			};
		}
	};

	def doWith(s : String, debug : Boolean) : Unit = {
		val t = new M_TINY("Tiny");
		val p = new TinyParser(t);
		val m = new M_USE_GLOBAL("UseGlobal",t);
		val r = p.asRoot(s).asInstanceOf[m.t_Result.T_Root];
		t.finish();
		if (debug) Debug.activate();
		m.finish();
		println("Results:");
		println("done is " + m.v_done(r));
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
