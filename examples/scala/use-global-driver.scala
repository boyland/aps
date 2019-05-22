object UseGlobal
{
	def main(args : Array[String]) : Unit = {
		if (args.length == 0) {
			doWith("3,(1,4)")
		} else {
			for (arg <- args) {
				doWith(arg)
			};
		}
		
	};

	def doWith(s : String) : Unit = {
	    	val t = new M_TINY("Tiny");
		val p = new TinyParser(t);
		val m = new M_USE_GLOBAL("UseGlobal",t);
		val r = p.asRoot(s).asInstanceOf[m.t_Result.T_Root];
		t.finish();
		Debug.activate();
		m.finish();
		val t_Tiny = m.t_Result;
		println("done is " + m.v_done(r));
	};
}
