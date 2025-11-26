object TestSynthSpec extends Spec {

  def testBasic() = {
    // Arrange
    val tree = new M_TINY("tiny");
    tree.v_root(
      tree.v_branch(
        tree.v_branch(
          tree.v_branch(
            tree.v_leaf(3),
            tree.v_leaf(5),
          ),
          tree.v_branch(
            tree.v_leaf(3),
            tree.v_leaf(5),
          ),
        ),
        tree.v_branch(
          tree.v_branch(
            tree.v_leaf(5),
            tree.v_leaf(5),
          ),
          tree.v_branch(
            tree.v_leaf(3),
            tree.v_leaf(3),
          ),
        ),
      )
    );

    // Act
    val module = new M_TEST_SYNTH("synth", tree);
    module.finish();

    val attr = module.v_syn(module.t_Root.nodes(0));

    // Assert
    assertEquals(3, attr, "final attribute should be 3");
  }
}
