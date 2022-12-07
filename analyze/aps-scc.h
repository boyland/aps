#ifndef APS_SCC_H
#define APS_SCC_H

#include "aps-dnc.h"

/**
 * @brief sets phylum graph SCC components
 * @param phy_graph phylum graph
 */
void set_phylum_graph_components(PHY_GRAPH* phy_graph);

/**
 * @brief sets augmented dependency graph SCC components
 * @param aug_graph augmented dependency graph
 */
void set_aug_graph_components(AUG_GRAPH* aug_graph);

#endif
