#pragma once

#include <vector>

#include "gridflow/core/Graph.h"

namespace gridflow {

enum class Metric { Cost, Time };

struct SearchPath {
    bool found = false;
    std::vector<int> nodes;
    std::vector<int> edges;
    double totalCost = 0.0;
    double totalTime = 0.0;
    double bottleneck = 0.0;
    int supplier = -1;
};

SearchPath shortestPath(const Graph& graph, int from, int to, Metric metric);
SearchPath reverseResidualPath(const Graph& graph, int demand,
                               const std::vector<double>& edgeResidual,
                               const std::vector<double>& nodeResidual,
                               const std::vector<double>& supplyRemaining);

}  // namespace gridflow
