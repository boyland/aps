object NTupleLatticeSpec extends Spec {

  def testBasic() = {
    // Arrange
    val tree = new M_TINY("tiny");
    tree.v_root(
      tree.v_branch(
        tree.v_branch(
          tree.v_leaf(1),
          tree.v_leaf(1),
        ),
        tree.v_branch(
          tree.v_leaf(2),
          tree.v_leaf(2),
        ),
      )
    );

    // Act
    val module = new M_TINY_TUPLE_LATTICE("ntuple-lattice", tree);
    module.finish()

    val attr = module.v_root_syn(module.t_Root.nodes(0))

    // Assert
    assertEquals(List(Set(1, 2)), attr, "flattened tree")
  }
}
