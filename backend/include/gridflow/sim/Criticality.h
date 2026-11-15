#pragma once

#include <string>
#include <vector>

#include "gridflow/core/Graph.h"

namespace gridflow {

struct CriticalityEntry {
    std::string nodeId;
    std::string name;
    double unmetDemandIfRemoved = 0.0;
    double increasePercent = 0.0;
};

std::vector<CriticalityEntry> rankCriticality(const Graph& current);

}  // namespace gridflow
