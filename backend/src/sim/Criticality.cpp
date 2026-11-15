#include "gridflow/sim/Criticality.h"

#include <algorithm>

#include "gridflow/sim/Allocation.h"

namespace gridflow {
namespace {
double totalUnmet(const Graph& graph) {
    double result = 0.0;
    for (int i = 0; i < graph.nodeCount(); ++i) result += graph.node(i).shortage;
    return result;
}
double totalDemand(const Graph& graph) {
    double result = 0.0;
    for (int i = 0; i < graph.nodeCount(); ++i) result += graph.node(i).effectiveDemand;
    return result;
}
}

std::vector<CriticalityEntry> rankCriticality(const Graph& current) {
    const double baselineUnmet = totalUnmet(current);
    const double denominator = totalDemand(current);
    std::vector<CriticalityEntry> result;
    for (int i = 0; i < current.nodeCount(); ++i) {
        const Facility& node = current.node(i);
        if (node.type == NodeType::DemandCentre || node.status == NodeStatus::Failed) continue;
        Graph copy = current;
        copy.node(i).status = NodeStatus::Failed;
        runAllocation(copy);
        const double unmet = totalUnmet(copy);
        result.push_back({node.id, node.name, unmet,
            denominator == 0.0 ? 0.0 : (unmet - baselineUnmet) / denominator * 100.0});
    }
    std::sort(result.begin(), result.end(), [&current](const CriticalityEntry& a, const CriticalityEntry& b) {
        if (a.unmetDemandIfRemoved != b.unmetDemandIfRemoved)
            return a.unmetDemandIfRemoved > b.unmetDemandIfRemoved;
        return current.findNode(a.nodeId) < current.findNode(b.nodeId);
    });
    return result;
}

}  // namespace gridflow
