#include "gridflow/core/Graph.h"

#include <cstddef>
#include <stdexcept>

namespace gridflow {

int Graph::addFacility(const Facility& facility) {
    // Validate everything BEFORE modifying anything, so a failure leaves the graph unchanged.
    if (facility.id.empty()) {
        throw std::invalid_argument("Facility id must not be empty");
    }
    if (findNode(facility.id) != -1) {
        throw std::invalid_argument("Duplicate facility id: " + facility.id);
    }

    nodes_.push_back(facility);
    out_.emplace_back();  // empty adjacency list for the new node
    in_.emplace_back();
    return nodeCount() - 1;
}

int Graph::addRoute(const Route& route) {
    if (route.id.empty()) {
        throw std::invalid_argument("Route id must not be empty");
    }
    if (!isValidNode(route.from) || !isValidNode(route.to)) {
        throw std::invalid_argument("Route " + route.id + " refers to a node that does not exist");
    }
    if (route.from == route.to) {
        throw std::invalid_argument("Route " + route.id + " connects a node to itself");
    }
    if (findEdge(route.id) != -1) {
        throw std::invalid_argument("Duplicate route id: " + route.id);
    }

    const int index = edgeCount();
    edges_.push_back(route);
    out_[static_cast<std::size_t>(route.from)].push_back(index);
    in_[static_cast<std::size_t>(route.to)].push_back(index);
    return index;
}

// Linear scans for now: O(n). To be replaced by HashTable lookups (average O(1)).
int Graph::findNode(const std::string& id) const {
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

int Graph::findEdge(const std::string& id) const {
    for (std::size_t i = 0; i < edges_.size(); ++i) {
        if (edges_[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

const std::vector<int>& Graph::outEdges(int node) const {
    return out_.at(static_cast<std::size_t>(node));
}

const std::vector<int>& Graph::inEdges(int node) const {
    return in_.at(static_cast<std::size_t>(node));
}

}  // namespace gridflow