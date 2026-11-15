#include "ImpactAnalyzer.h"

#include <algorithm>

#include <cstddef>
#include <vector>

namespace gridflow {
namespace {

double unmetDemand(const Graph& graph) {
    double total = 0.0;
    for (int index = 0; index < graph.nodeCount(); ++index) {
        const Facility& node = graph.node(index);
        if (node.type == NodeType::DemandCentre) total += node.shortage;
    }
    return total;
}

}  // namespace

ImpactReport analyzeImpact(const Graph& before, const Graph& after,
                           const std::vector<Disruption>& disruptions) {
    ImpactReport report;
    const ComponentInfo componentsBefore = connectedComponents(before);
    report.componentCountBefore = componentsBefore.count;
    report.components = connectedComponents(after);
    report.componentCountAfter = report.components.count;
    report.unmetDemandBefore = unmetDemand(before);
    report.unmetDemandAfter = unmetDemand(after);
    std::vector<unsigned char> seenNodes(static_cast<std::size_t>(before.nodeCount()), 0);
    std::vector<unsigned char> seenEdges(static_cast<std::size_t>(before.edgeCount()), 0);

    for (const Disruption& disruption : disruptions) {
        int start = -1;
        if (disruption.type == DisruptionType::EdgeDisruption) {
            const int edgeIndex = before.findEdge(disruption.targetId);
            if (edgeIndex < 0) continue;
            const Route& edge = before.edge(edgeIndex);
            const std::size_t index = static_cast<std::size_t>(edgeIndex);
            if (seenEdges[index] == 0) {
                seenEdges[index] = 1;
                report.exposedEdges.push_back(edge.id);
            }
            start = edge.to;
        } else {
            start = before.findNode(disruption.targetId);
        }

        if (start < 0) continue;
        const TraversalResult exposure = bfsDownstream(before, start);
        for (int nodeIndex : exposure.exposedNodeIndices) {
            const std::size_t index = static_cast<std::size_t>(nodeIndex);
            if (seenNodes[index] == 0) {
                seenNodes[index] = 1;
                report.exposedNodes.push_back(before.node(nodeIndex).id);
            }
        }
        for (int edgeIndex : exposure.exposedEdgeIndices) {
            const std::size_t index = static_cast<std::size_t>(edgeIndex);
            if (seenEdges[index] == 0) {
                seenEdges[index] = 1;
                report.exposedEdges.push_back(before.edge(edgeIndex).id);
            }
        }
    }

    for (int index = 0; index < after.nodeCount(); ++index) {
        const Facility& node = after.node(index);
        if (node.type != NodeType::DemandCentre || node.shortage <= 0.0) continue;
        report.shortages.push_back(
            Shortage{node.id, node.name, node.priority, node.effectiveDemand, node.served,
                     node.shortage});
    }

    std::sort(report.shortages.begin(), report.shortages.end(),
              [](const Shortage& left, const Shortage& right) {
                  if (left.priority != right.priority) return left.priority < right.priority;
                  if (left.shortage != right.shortage) return left.shortage > right.shortage;
                  return left.nodeId < right.nodeId;
              });
    return report;
}

}  // namespace gridflow
