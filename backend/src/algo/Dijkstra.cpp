#include "gridflow/algo/Dijkstra.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "gridflow/model/Constants.h"
#include "gridflow/ds/MinHeap.h"

namespace gridflow {
namespace {

struct Entry { double dist; int node; };
struct EntryBefore {
    bool operator()(const Entry& left, const Entry& right) const {
        return left.dist < right.dist || (left.dist == right.dist && left.node < right.node);
    }
};

SearchPath search(const Graph& graph, int start, int target, Metric metric, bool reverse,
                  const std::vector<double>* edgeResidual,
                  const std::vector<double>* nodeResidual,
                  const std::vector<double>* supplyRemaining) {
    if (start < 0 || start >= graph.nodeCount() ||
        (!reverse && (target < 0 || target >= graph.nodeCount()))) {
        throw std::out_of_range("Dijkstra node index is out of range");
    }
    const int count = graph.nodeCount();
    std::vector<double> best(static_cast<std::size_t>(count), std::numeric_limits<double>::infinity());
    std::vector<int> parentEdge(static_cast<std::size_t>(count), -1);
    MinHeap<Entry, EntryBefore> heap;
    best[static_cast<std::size_t>(start)] = 0.0;
    heap.push({0.0, start});
    int settledTarget = -1;
    while (!heap.empty()) {
        Entry entry = heap.top();
        heap.pop();
        if (entry.dist > best[static_cast<std::size_t>(entry.node)]) continue;
        if (graph.node(entry.node).status == NodeStatus::Failed) continue;
        if (reverse && supplyRemaining->at(static_cast<std::size_t>(entry.node)) > kEpsilon) {
            settledTarget = entry.node;
            break;
        }
        if (!reverse && entry.node == target) { settledTarget = target; break; }
        const std::vector<int>& adjacent = reverse ? graph.inEdges(entry.node) : graph.outEdges(entry.node);
        for (int edgeIndex : adjacent) {
            const Route& route = graph.edge(edgeIndex);
            const int next = reverse ? route.from : route.to;
            const double residual = edgeResidual ? edgeResidual->at(static_cast<std::size_t>(edgeIndex))
                                                  : route.effectiveCapacity;
            const double nodeCap = nodeResidual ? nodeResidual->at(static_cast<std::size_t>(next))
                                                 : graph.node(next).capacity;
            if (route.status == EdgeStatus::Failed || residual <= kEpsilon ||
                graph.node(next).status == NodeStatus::Failed || nodeCap <= kEpsilon) continue;
            if (reverse && nodeResidual->at(static_cast<std::size_t>(entry.node)) <= kEpsilon) continue;
            const double weight = metric == Metric::Cost ? route.cost : route.transitTime;
            const double candidate = entry.dist + weight;
            const std::size_t nextIndex = static_cast<std::size_t>(next);
            if (candidate < best[nextIndex]) {
                best[nextIndex] = candidate;
                parentEdge[nextIndex] = edgeIndex;
                heap.push({candidate, next});
            }
        }
    }
    SearchPath result;
    if (settledTarget < 0) return result;
    result.found = true;
    result.supplier = reverse ? settledTarget : -1;
    int cursor = reverse ? settledTarget : target;
    if (reverse) {
        result.nodes.push_back(cursor);
        while (cursor != start) {
            int edgeIndex = parentEdge.at(static_cast<std::size_t>(cursor));
            if (edgeIndex < 0) throw std::logic_error("Dijkstra parent chain is incomplete");
            result.edges.push_back(edgeIndex);
            cursor = graph.edge(edgeIndex).to;
            result.nodes.push_back(cursor);
        }
    } else {
        result.nodes.push_back(cursor);
        while (cursor != start) {
            int edgeIndex = parentEdge.at(static_cast<std::size_t>(cursor));
            if (edgeIndex < 0) throw std::logic_error("Dijkstra parent chain is incomplete");
            result.edges.push_back(edgeIndex);
            cursor = graph.edge(edgeIndex).from;
            result.nodes.push_back(cursor);
        }
        std::reverse(result.nodes.begin(), result.nodes.end());
        std::reverse(result.edges.begin(), result.edges.end());
    }
    result.bottleneck = std::numeric_limits<double>::infinity();
    for (int edgeIndex : result.edges) {
        const Route& route = graph.edge(edgeIndex);
        result.totalCost += route.cost;
        result.totalTime += route.transitTime;
        const double capacity = edgeResidual ? edgeResidual->at(static_cast<std::size_t>(edgeIndex))
                                             : route.effectiveCapacity;
        result.bottleneck = std::min(result.bottleneck, capacity);
    }
    if (result.edges.empty()) result.bottleneck = 0.0;
    return result;
}

}  // namespace

SearchPath shortestPath(const Graph& graph, int from, int to, Metric metric) {
    return search(graph, from, to, metric, false, nullptr, nullptr, nullptr);
}

SearchPath reverseResidualPath(const Graph& graph, int demand,
                               const std::vector<double>& edgeResidual,
                               const std::vector<double>& nodeResidual,
                               const std::vector<double>& supplyRemaining) {
    if (edgeResidual.size() != static_cast<std::size_t>(graph.edgeCount()) ||
        nodeResidual.size() != static_cast<std::size_t>(graph.nodeCount()) ||
        supplyRemaining.size() != static_cast<std::size_t>(graph.nodeCount())) {
        throw std::invalid_argument("Dijkstra residual vector size does not match graph");
    }
    return search(graph, demand, -1, Metric::Cost, true, &edgeResidual,
                  &nodeResidual, &supplyRemaining);
}

}  // namespace gridflow
