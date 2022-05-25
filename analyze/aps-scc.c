#include <jbb.h>
#include <stdio.h>
#include "aps-ag.h"
#include "jbb-alloc.h"

struct state {
  Graph* graph;
  AUG_GRAPH* aug_graph;
};

typedef struct state State;

/*** determining strongly connected sets of attributes ***/

static void make_augmented_cycles_for_node(AUG_GRAPH* aug_graph,
                                           Graph* graph,
                                           Declaration node) {
  int start = Declaration_info(node)->instance_index;
  int n = aug_graph->instances.length;
  int max = n;
  int phy_n;
  int i, j;
  STATE* s = aug_graph->global_state;
  PHY_GRAPH* phy_graph = Declaration_info(node)->node_phy_graph;

  if (phy_graph == NULL) {
    /* a semantic child of a constructor */
    /* printf("No summary graph for %s\n",decl_name(node)); */
    return;
  }

  phy_n = phy_graph->instances.length;

  /* discover when the instances for this node end.
   */
  for (i = start; i < max; ++i) {
    if (aug_graph->instances.array[i].node != node) {
      /* DEBUG
       *print_instance(&aug_graph->instances.array[i],stdout);
       *printf(" does not refer to %s\n",
       *     symbol_name(def_name(declaration_def(node))));
       */
      max = i;
      break;
    }
  }

  // Merge cycles involving phylum and augmented graph together.
  for (i = start; i < max; ++i) {
    int phy_i = i - start;
    if (phy_graph->mingraph[phy_i * phy_n + phy_i] != no_dependency) {
      // merge_sets(constructor_index + i, phylum_index + phy_i);
    }
  }
}

static void* make_augmented_cycles_func_calls(void* state_untyped, void* node) {
  State* state = (State*)state_untyped;

  AUG_GRAPH* aug_graph = state->aug_graph;
  Graph* graph = state->graph;

  switch (ABSTRACT_APS_tnode_phylum(node)) {
    default:
      break;
    case KEYExpression: {
      Expression e = (Expression)node;
      Declaration fdecl = 0;
      if ((fdecl = local_call_p(e)) != NULL &&
          Declaration_KEY(fdecl) == KEYfunction_decl) {
        Declaration proxy = Expression_info(e)->funcall_proxy;
        /* need to figure out constructor index */
        int i;
        for (i = 0; i < aug_graph->global_state->match_rules.length; ++i)
          if (aug_graph == &aug_graph->global_state->aug_graphs[i])
            break;
        if (proxy == NULL)
          fatal_error("missing funcall proxy");
        make_augmented_cycles_for_node(aug_graph, graph, proxy);
      }
    } break;
    case KEYDeclaration: {
      Declaration decl = (Declaration)node;
      switch (Declaration_KEY(decl)) {
        default:
          break;
        case KEYsome_function_decl:
        case KEYtop_level_match:
          /* don't look inside (unless its what we're doing the analysis for) */
          if (aug_graph->match_rule != node)
            return NULL;
          break;
        case KEYassign: {
          Declaration pdecl = proc_call_p(assign_rhs(decl));
          if (pdecl != NULL) {
            /* need to figure out constructor index */
            int i;
            for (i = 0; i < aug_graph->global_state->match_rules.length; ++i)
              if (aug_graph == &aug_graph->global_state->aug_graphs[i])
                break;
            /* if we never take the break, we must have the global
             * dependencies augmented dependency graph.
             */
            make_augmented_cycles_for_node(aug_graph, graph, decl);
          }
          break;
        }
      }
      break;
    }
  }
  return state;
}

static void make_augmented_cycles(AUG_GRAPH* aug_graph, Graph* graph) {
  Declaration rhs_decl;
  switch (Declaration_KEY(aug_graph->match_rule)) {
    default:
      fatal_error("unexpected match rule");
      break;
    case KEYmodule_decl:
      make_augmented_cycles_for_node(aug_graph, graph, aug_graph->match_rule);
      break;
    case KEYsome_function_decl:
      make_augmented_cycles_for_node(aug_graph, graph, aug_graph->match_rule);
      break;
    case KEYtop_level_match:
      make_augmented_cycles_for_node(aug_graph, graph, aug_graph->lhs_decl);
      for (rhs_decl = aug_graph->first_rhs_decl; rhs_decl != NULL;
           rhs_decl = Declaration_info(rhs_decl)->next_decl) {
        make_augmented_cycles_for_node(aug_graph, graph, rhs_decl);
      }
      break;
  }
  /* find procedure calls */
  State* state = (State*)alloca(sizeof(State));
  state->aug_graph = aug_graph;
  state->graph = graph;

  traverse_Declaration(make_augmented_cycles_func_calls, state,
                       aug_graph->match_rule);
}

static void analyze_state(STATE* s) {
  int i, j, k;
  /* summary cycles */
  for (i = 0; i < s->phyla.length; i++) {
    PHY_GRAPH* phy = &s->phy_graphs[i];
    int n = phy->instances.length;
    Graph* graph = graph_create(n);

    for (j = 0; j < n; j++) {
      for (k = 0; k < n; k++) {
        if (phy->mingraph[j * n + k] != no_dependency) {
          graph_add_edge(graph, j, k);
        }
      }
    }
  }
  /* augmented dependency graph */
  for (i = 0; i <= s->match_rules.length; i++) {
    AUG_GRAPH* aug_graph = (i == s->match_rules.length)
                               ? &s->global_dependencies
                               : &s->aug_graphs[i];
    int n = aug_graph->instances.length;
    Graph* graph = graph_create(n);
    for (j = 0; j < n; j++) {
      for (k = 0; k < n; k++) {
        if (edgeset_kind(aug_graph->graph[j * n + k]) != no_dependency) {
          graph_add_edge(graph, j, k);
        }
      }
    }
    make_augmented_cycles(aug_graph, graph);
    aug_graph->components = graph_scc(graph);

    printf("Components of %s\n", decl_name(aug_graph->syntax_decl));
    for (j = 0; j < aug_graph->components.length; j++) {
      COMPONENT comp = aug_graph->components.array[j];
      printf("Component #%d\n", j);

      for (k = 0; k < comp.length; k++) {
        print_instance(&aug_graph->instances.array[comp.array[k]], stdout);
        printf("\n");
      }

      printf("\n");
    }
  }
}

void state_scc(STATE* s) {
  analyze_state(s);
}
