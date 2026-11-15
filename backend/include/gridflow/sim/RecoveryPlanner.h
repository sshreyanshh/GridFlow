#pragma once

#include <string>
#include <vector>

#include "gridflow/core/Graph.h"
#include "gridflow/model/Disruption.h"
#include "gridflow/sim/Allocation.h"

namespace gridflow {

struct PlannedAllocation {
    std::string demandCentreId;
    std::vector<std::string> path;
    std::vector<std::string> edgeIds;
    double flow = 0.0;
    double totalCost = 0.0;
    double totalTime = 0.0;
};
struct PlannedUnresolved { std::string nodeId; double shortage = 0.0; std::string reason; };
struct PlannedRecommendation {
    std::string type;
    std::string targetId;
    int priority = 1;
    std::string message;
};
struct RecoveryPlanData {
    std::vector<PlannedAllocation> allocations;
    std::vector<PlannedUnresolved> unresolved;
    std::vector<PlannedRecommendation> recommendations;
};

RecoveryPlanData buildRecoveryPlan(const Graph& current, const AllocationResult& allocation,
                                  const AllocationResult& baselineAllocation,
                                  const std::vector<Disruption>& disruptions);

}  // namespace gridflow
