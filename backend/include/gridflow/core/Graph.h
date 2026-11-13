#pragma once
// The energy network as a directed graph, stored with adjacency lists.
//
// Storage layout (everything is addressed by integer INDEX, never by pointer):
//
//   nodes_[i]   -> the i-th Facility
//   edges_[e]   -> the e-th Route (it knows its own `from` and `to` indices)
//   out_[i]     -> indices of the routes that LEAVE node i
//   in_[i]      -> indices of the routes that ENTER node i
//
// Invariants the class guarantees (a Graph can never be built in a broken state):
//   1. Every facility id is non-empty and unique.
//   2. Every route id is non-empty and unique.
//   3. Every route's `from` and `to` refer to existing nodes, and from != to.
//   4. out_ and in_ always match edges_ exactly.
//
// Copying: all members are std::vector of plain values, so the compiler-generated
// copy constructor and assignment already produce a fully independent deep copy.
// (This is the "rule of zero": no hand-written destructor or copy code needed.)
// The criticality ranking and Simulation::reset() rely on this.
//
// Complexity note: findNode/findEdge are currently a linear scan, O(n). They will
// be replaced by our own HashTable (average O(1)); the public interface will not change.

#include <string>
#include <vector>

#include "gridflow/model/Facility.h"
#include "gridflow/model/Route.h"

namespace gridflow {

class Graph {
public:
    // ------------------------------------------------------------ building
    // Both throw std::invalid_argument (with a descriptive message) if an
    // invariant would be broken. On failure the graph is left unchanged.

    // Adds a facility and returns its index.
    int addFacility(const Facility& facility);

    // Adds a route, registers it in the adjacency lists and returns its index.
    // Route::from / Route::to must already be valid node indices.
    int addRoute(const Route& route);

    // ------------------------------------------------------------ queries
    int nodeCount() const { return static_cast<int>(nodes_.size()); }
    int edgeCount() const { return static_cast<int>(edges_.size()); }

    // Bounds-checked access: throws std::out_of_range for a bad index.
    Facility& node(int index) { return nodes_.at(static_cast<std::size_t>(index)); }
    const Facility& node(int index) const { return nodes_.at(static_cast<std::size_t>(index)); }
    Route& edge(int index) { return edges_.at(static_cast<std::size_t>(index)); }
    const Route& edge(int index) const { return edges_.at(static_cast<std::size_t>(index)); }

    // Returns the index for an id, or -1 if no such facility / route exists.
    int findNode(const std::string& id) const;
    int findEdge(const std::string& id) const;

    // Indices of the routes leaving / entering a node. To get the neighbour use
    // edge(e).to (outgoing) or edge(e).from (incoming).
    // Do not keep these references across addRoute(): the vectors may reallocate.
    const std::vector<int>& outEdges(int node) const;
    const std::vector<int>& inEdges(int node) const;

private:
    bool isValidNode(int index) const { return index >= 0 && index < nodeCount(); }

    std::vector<Facility> nodes_;
    std::vector<Route> edges_;
    std::vector<std::vector<int>> out_;
    std::vector<std::vector<int>> in_;
};

}  // namespace gridflow