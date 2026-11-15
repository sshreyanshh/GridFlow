#pragma once

#include <string>
#include <vector>

#include "gridflow/core/Graph.h"

namespace gridflow {

struct TraversalResult {
    std::vector<std::string> exposedNodes;
    std::vector<std::string> exposedEdges;
    std::vector<int> exposedNodeIndices;
    std::vector<int> exposedEdgeIndices;
};

struct ComponentInfo {
    int count = 0;
    std::vector<std::string> isolatedDemandCentres;
};

// O(V + E): visits each reachable non-failed node and its outgoing edges once.
TraversalResult bfsDownstream(const Graph& graph, int start);

// O(V + E): finds components of the undirected, non-failed portion of the graph.
ComponentInfo connectedComponents(const Graph& graph);

}  // namespace gridflow
