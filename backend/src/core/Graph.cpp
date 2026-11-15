#include "gridflow/core/Graph.h"

#include <cstddef>
#include <stdexcept>

namespace gridflow {

int Graph::addFacility(const Facility& facility) {
    // Validate everything BEFORE modifying anything, so a failure leaves the graph unchanged.
    if (facility.id.empty()) {
        throw std::invalid_argument("Facility id must not be empty");
    }
    if (nodeIndex_.contains(facility.id)) {
        throw std::invalid_argument("Duplicate facility id: " + facility.id);
    }

    const int index = nodeCount();
    nodes_.push_back(facility);
    out_.emplace_back();  // empty adjacency list for the new node
    in_.emplace_back();
    nodeIndex_.put(facility.id, index);
    return index;
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
    if (edgeIndex_.contains(route.id)) {
        throw std::invalid_argument("Duplicate route id: " + route.id);
    }

    const int index = edgeCount();
    edges_.push_back(route);
    out_[static_cast<std::size_t>(route.from)].push_back(index);
    in_[static_cast<std::size_t>(route.to)].push_back(index);
    edgeIndex_.put(route.id, index);
    return index;
}

int Graph::findNode(const std::string& id) const {
    const int* index = nodeIndex_.get(id);
    return index != nullptr ? *index : -1;
}

int Graph::findEdge(const std::string& id) const {
    const int* index = edgeIndex_.get(id);
    return index != nullptr ? *index : -1;
}

const std::vector<int>& Graph::outEdges(int node) const {
    return out_.at(static_cast<std::size_t>(node));
}

const std::vector<int>& Graph::inEdges(int node) const {
    return in_.at(static_cast<std::size_t>(node));
}

}  // namespace gridflow