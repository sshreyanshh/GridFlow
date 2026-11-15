#include "Traversal.h"

#include <vector>

#include "../ds/Queue.h"

namespace gridflow {

TraversalResult bfsDownstream(const Graph& graph, int start) {
    const Facility& startingNode = graph.node(start);
    TraversalResult result;
    if (startingNode.status == NodeStatus::Failed) return result;

    std::vector<unsigned char> visited(static_cast<std::size_t>(graph.nodeCount()), 0);
    Queue<int> frontier;
    visited[static_cast<std::size_t>(start)] = 1;
    frontier.push(start);
    result.exposedNodes.push_back(startingNode.id);
    result.exposedNodeIndices.push_back(start);

    while (!frontier.empty()) {
        const int current = frontier.front();
        frontier.pop();

        for (int edgeIndex : graph.outEdges(current)) {
            const Route& route = graph.edge(edgeIndex);
            if (route.status == EdgeStatus::Failed) continue;

            const Facility& next = graph.node(route.to);
            if (next.status == NodeStatus::Failed) continue;
            result.exposedEdges.push_back(route.id);
            result.exposedEdgeIndices.push_back(edgeIndex);

            const std::size_t nextIndex = static_cast<std::size_t>(route.to);
            if (visited[nextIndex] == 0) {
                visited[nextIndex] = 1;
                result.exposedNodes.push_back(next.id);
                result.exposedNodeIndices.push_back(route.to);
                frontier.push(route.to);
            }
        }
    }

    return result;
}

ComponentInfo connectedComponents(const Graph& graph) {
    const int nodeCount = graph.nodeCount();
    std::vector<int> componentOf(static_cast<std::size_t>(nodeCount), -1);
    std::vector<unsigned char> componentHasSupply;
    std::vector<int> stack;
    ComponentInfo result;

    for (int start = 0; start < nodeCount; ++start) {
        const std::size_t startIndex = static_cast<std::size_t>(start);
        if (graph.node(start).status == NodeStatus::Failed || componentOf[startIndex] != -1) {
            continue;
        }

        const int componentId = result.count++;
        componentHasSupply.push_back(0);
        componentOf[startIndex] = componentId;
        stack.push_back(start);

        while (!stack.empty()) {
            const int current = stack.back();
            stack.pop_back();
            const Facility& currentNode = graph.node(current);
            if (currentNode.type == NodeType::Source ||
                currentNode.type == NodeType::ImportTerminal) {
                componentHasSupply[static_cast<std::size_t>(componentId)] = 1;
            }

            const auto visitNeighbour = [&](int edgeIndex, int neighbour) {
                if (graph.edge(edgeIndex).status == EdgeStatus::Failed) return;
                const std::size_t neighbourIndex = static_cast<std::size_t>(neighbour);
                if (graph.node(neighbour).status == NodeStatus::Failed ||
                    componentOf[neighbourIndex] != -1) {
                    return;
                }
                componentOf[neighbourIndex] = componentId;
                stack.push_back(neighbour);
            };

            for (int edgeIndex : graph.outEdges(current)) {
                visitNeighbour(edgeIndex, graph.edge(edgeIndex).to);
            }
            for (int edgeIndex : graph.inEdges(current)) {
                visitNeighbour(edgeIndex, graph.edge(edgeIndex).from);
            }
        }
    }

    for (int index = 0; index < nodeCount; ++index) {
        const int componentId = componentOf[static_cast<std::size_t>(index)];
        if (componentId >= 0 &&
            graph.node(index).type == NodeType::DemandCentre &&
            componentHasSupply[static_cast<std::size_t>(componentId)] == 0) {
            result.isolatedDemandCentres.push_back(graph.node(index).id);
        }
    }

    return result;
}

}  // namespace gridflow
