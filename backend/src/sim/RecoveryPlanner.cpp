#include "gridflow/sim/RecoveryPlanner.h"

#include <algorithm>

#include "gridflow/model/Constants.h"

namespace gridflow {

RecoveryPlanData buildRecoveryPlan(const Graph& current, const AllocationResult& allocation,
                                  const AllocationResult& baselineAllocation,
                                  const std::vector<Disruption>& disruptions) {
    RecoveryPlanData plan;
    for (const AllocationEntry& entry : allocation.allocations) {
        PlannedAllocation item;
        item.demandCentreId = current.node(entry.demandNode).id;
        item.flow = entry.flow; item.totalCost = entry.totalCost; item.totalTime = entry.totalTime;
        for (int node : entry.pathNodes) item.path.push_back(current.node(node).id);
        for (int edge : entry.pathEdges) item.edgeIds.push_back(current.edge(edge).id);
        plan.allocations.push_back(item);
        double bottleneck = entry.pathEdges.empty() ? 0.0 : current.edge(entry.pathEdges.front()).effectiveCapacity;
        for (int edge : entry.pathEdges) bottleneck = std::min(bottleneck, current.edge(edge).effectiveCapacity);
        for (int edge : entry.pathEdges) {
            const Route& route = current.edge(edge);
            if (route.effectiveCapacity > 0.0 && route.flow >= route.effectiveCapacity - kEpsilon &&
                route.effectiveCapacity <= bottleneck + kEpsilon) {
                plan.recommendations.push_back({"ADD_CAPACITY", route.id, 2,
                    "Increase " + route.id + " capacity beyond " + std::to_string(route.effectiveCapacity) + " EU/day."});
            }
        }
    }
    for (const Unresolved& item : allocation.unresolved) {
        const Facility& node = current.node(item.node);
        const char* reason = item.reason == Reason::NoPath ? "NO_PATH" : "CAPACITY_EXHAUSTED";
        plan.unresolved.push_back({node.id, item.shortage, reason});
        if (node.priority == 3) plan.recommendations.push_back({"LOAD_SHED", node.id, 3,
            "Shed " + std::to_string(item.shortage) + " EU/day at " + node.id + " until supply recovers."});
    }
    for (int i = 0; i < current.nodeCount(); ++i) {
        const Facility& node = current.node(i);
        if (node.type != NodeType::DemandCentre) continue;
        std::vector<std::vector<int>> baselinePaths, currentPaths;
        for (const AllocationEntry& item : baselineAllocation.allocations)
            if (item.demandNode == i) baselinePaths.push_back(item.pathEdges);
        for (const AllocationEntry& item : allocation.allocations)
            if (item.demandNode == i) currentPaths.push_back(item.pathEdges);
        if (baselinePaths != currentPaths) plan.recommendations.push_back({"REROUTE", node.id, 1,
            "Review the route plan for " + node.id + "; current service is " +
            std::to_string(current.node(i).served) + " EU/day."});
    }
    for (const Disruption& disruption : disruptions) {
        plan.recommendations.push_back({"RESTORE", disruption.targetId, 1,
            "Restore " + disruption.targetId + " after the disruption is cleared."});
    }
    std::sort(plan.recommendations.begin(), plan.recommendations.end(),
        [](const PlannedRecommendation& a, const PlannedRecommendation& b) {
            if (a.priority != b.priority) return a.priority < b.priority;
            if (a.type != b.type) return a.type < b.type;
            return a.targetId < b.targetId;
        });
    return plan;
}

}  // namespace gridflow
