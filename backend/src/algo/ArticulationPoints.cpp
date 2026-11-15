#include "ArticulationPoints.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

namespace gridflow {

std::vector<int> articulationPoints(const Graph& graph) {
    const int nodeCount = graph.nodeCount();
    std::vector<int> discovery(static_cast<std::size_t>(nodeCount), -1);
    std::vector<int> low(static_cast<std::size_t>(nodeCount), -1);
    std::vector<unsigned char> isArticulation(static_cast<std::size_t>(nodeCount), 0);
    int time = 0;

    const std::function<void(int, int)> visit = [&](int current, int parentEdge) {
        const std::size_t currentIndex = static_cast<std::size_t>(current);
        discovery[currentIndex] = low[currentIndex] = time++;
        int childCount = 0;

        const auto inspectEdge = [&](int edgeIndex, int neighbour) {
            if (edgeIndex == parentEdge || graph.edge(edgeIndex).status == EdgeStatus::Failed ||
                graph.node(neighbour).status == NodeStatus::Failed) {
                return;
            }

            const std::size_t neighbourIndex = static_cast<std::size_t>(neighbour);
            if (discovery[neighbourIndex] == -1) {
                ++childCount;
                visit(neighbour, edgeIndex);
                low[currentIndex] = std::min(low[currentIndex], low[neighbourIndex]);
                if (parentEdge != -1 && low[neighbourIndex] >= discovery[currentIndex]) {
                    isArticulation[currentIndex] = 1;
                }
            } else {
                low[currentIndex] = std::min(low[currentIndex], discovery[neighbourIndex]);
            }
        };

        for (int edgeIndex : graph.outEdges(current)) {
            inspectEdge(edgeIndex, graph.edge(edgeIndex).to);
        }
        for (int edgeIndex : graph.inEdges(current)) {
            inspectEdge(edgeIndex, graph.edge(edgeIndex).from);
        }

        if (parentEdge == -1 && childCount >= 2) isArticulation[currentIndex] = 1;
    };

    for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        const std::size_t index = static_cast<std::size_t>(nodeIndex);
        if (graph.node(nodeIndex).status != NodeStatus::Failed && discovery[index] == -1) {
            visit(nodeIndex, -1);
        }
    }

    std::vector<int> result;
    for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        if (isArticulation[static_cast<std::size_t>(nodeIndex)] != 0) {
            result.push_back(nodeIndex);
        }
    }
    return result;
}

}  // namespace gridflow
