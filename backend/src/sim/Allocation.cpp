#include "gridflow/sim/Allocation.h"

#include <algorithm>
#include <stdexcept>

#include "gridflow/algo/Dijkstra.h"
#include "gridflow/model/Constants.h"
#include "gridflow/ds/MinHeap.h"

namespace gridflow {
namespace {

struct DemandEntry { int node; int priority; double shortage; };
struct DemandBefore {
    bool operator()(const DemandEntry& a, const DemandEntry& b) const {
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.shortage != b.shortage) return a.shortage > b.shortage;
        return a.node < b.node;
    }
};

}  // namespace

AllocationResult runAllocation(Graph& graph) {
    const std::size_t nodes = static_cast<std::size_t>(graph.nodeCount());
    const std::size_t edges = static_cast<std::size_t>(graph.edgeCount());
    std::vector<double> edgeResidual(edges, 0.0);
    std::vector<double> nodeResidual(nodes, 0.0);
    std::vector<double> supplyRemaining(nodes, 0.0);
    std::vector<double> effectiveDemand(nodes, 0.0);
    std::vector<double> served(nodes, 0.0);
    std::vector<double> throughput(nodes, 0.0);
    std::vector<unsigned char> madeProgress(nodes, 0);
    for (int i = 0; i < graph.nodeCount(); ++i) {
        Facility& node = graph.node(i);
        node.effectiveSupply = (node.type == NodeType::Source || node.type == NodeType::ImportTerminal)
            ? (node.status == NodeStatus::Failed ? 0.0 : node.supply * (1.0 - node.severity)) : 0.0;
        node.effectiveDemand = node.type == NodeType::DemandCentre
            ? node.demand * (1.0 + node.surge) : 0.0;
        node.served = 0.0;
        node.shortage = node.effectiveDemand;
        node.throughput = 0.0;
        nodeResidual[static_cast<std::size_t>(i)] = node.status == NodeStatus::Failed ? 0.0 : node.capacity;
        supplyRemaining[static_cast<std::size_t>(i)] = node.effectiveSupply;
        effectiveDemand[static_cast<std::size_t>(i)] = node.effectiveDemand;
    }
    for (int i = 0; i < graph.edgeCount(); ++i) {
        Route& edge = graph.edge(i);
        const bool blocked = edge.status == EdgeStatus::Failed ||
            graph.node(edge.from).status == NodeStatus::Failed || graph.node(edge.to).status == NodeStatus::Failed;
        edge.effectiveCapacity = blocked ? 0.0 : edge.capacity * (1.0 - edge.severity);
        edge.flow = 0.0;
        edgeResidual[static_cast<std::size_t>(i)] = edge.effectiveCapacity;
    }
    MinHeap<DemandEntry, DemandBefore> demands;
    for (int i = 0; i < graph.nodeCount(); ++i) {
        const Facility& node = graph.node(i);
        if (node.type == NodeType::DemandCentre && effectiveDemand[static_cast<std::size_t>(i)] > kEpsilon)
            demands.push({i, node.priority, effectiveDemand[static_cast<std::size_t>(i)]});
    }
    AllocationResult result;
    const std::size_t cap = static_cast<std::size_t>(graph.edgeCount() + graph.nodeCount() * 3 + 1) *
                            static_cast<std::size_t>(graph.nodeCount() + 1);
    std::size_t iterations = 0;
    while (!demands.empty()) {
        if (++iterations > cap) throw std::logic_error("allocation exceeded defensive iteration cap");
        DemandEntry item = demands.top();
        demands.pop();
        const std::size_t demandIndex = static_cast<std::size_t>(item.node);
        const double unmet = effectiveDemand[demandIndex] - served[demandIndex];
        if (unmet <= kEpsilon) continue;
        SearchPath path = reverseResidualPath(graph, item.node, edgeResidual, nodeResidual, supplyRemaining);
        if (!path.found) {
            result.unresolved.push_back({item.node, unmet, madeProgress[demandIndex] != 0
                ? Reason::CapacityExhausted : Reason::NoPath});
            continue;
        }
        double delta = std::min(unmet, supplyRemaining.at(static_cast<std::size_t>(path.supplier)));
        for (int edge : path.edges) delta = std::min(delta, edgeResidual.at(static_cast<std::size_t>(edge)));
        for (int node : path.nodes) delta = std::min(delta, nodeResidual.at(static_cast<std::size_t>(node)));
        if (delta <= kEpsilon) {
            result.unresolved.push_back({item.node, unmet, Reason::CapacityExhausted});
            continue;
        }
        for (int edge : path.edges) edgeResidual[static_cast<std::size_t>(edge)] -= delta;
        for (int node : path.nodes) {
            nodeResidual[static_cast<std::size_t>(node)] -= delta;
            throughput[static_cast<std::size_t>(node)] += delta;
        }
        supplyRemaining[static_cast<std::size_t>(path.supplier)] -= delta;
        served[demandIndex] += delta;
        madeProgress[demandIndex] = 1;
        result.allocations.push_back({item.node, path.nodes, path.edges, delta, path.totalCost, path.totalTime});
        const double left = effectiveDemand[demandIndex] - served[demandIndex];
        if (left > kEpsilon) demands.push({item.node, item.priority, left});
    }
    for (int i = 0; i < graph.nodeCount(); ++i) {
        Facility& node = graph.node(i);
        node.served = served[static_cast<std::size_t>(i)];
        node.shortage = node.effectiveDemand - node.served;
        if (node.shortage < kEpsilon) node.shortage = 0.0;
        node.throughput = throughput[static_cast<std::size_t>(i)];
    }
    for (int i = 0; i < graph.edgeCount(); ++i) {
        graph.edge(i).flow = graph.edge(i).effectiveCapacity - edgeResidual[static_cast<std::size_t>(i)];
    }
    return result;
}

}  // namespace gridflow
