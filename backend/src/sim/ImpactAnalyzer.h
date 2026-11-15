#pragma once

#include <string>
#include <vector>

#include "gridflow/core/Graph.h"
#include "gridflow/model/Disruption.h"
#include "../algo/Traversal.h"

namespace gridflow {

struct Shortage {
    std::string nodeId;
    std::string name;
    int priority = 0;
    double demand = 0.0;
    double served = 0.0;
    double shortage = 0.0;
};

struct ImpactReport {
    std::vector<std::string> exposedNodes;
    std::vector<std::string> exposedEdges;
    ComponentInfo components;
    int componentCountBefore = 0;
    int componentCountAfter = 0;
    std::vector<Shortage> shortages;
    double unmetDemandBefore = 0.0;
    double unmetDemandAfter = 0.0;
};

// O(D(V + E) + S log S): D disruptions, S shortage entries; O(V + E + S) space.
ImpactReport analyzeImpact(const Graph& before, const Graph& after,
                           const std::vector<Disruption>& disruptions);

}  // namespace gridflow
