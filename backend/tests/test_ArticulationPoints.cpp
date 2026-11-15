#include <string>
#include <vector>

#include "doctest.h"
#include "algo/ArticulationPoints.h"

using namespace gridflow;

namespace {

Facility makeNode(const std::string& id, NodeStatus status = NodeStatus::Active) {
    Facility node;
    node.id = id;
    node.name = id;
    node.status = status;
    return node;
}

Route makeEdge(const std::string& id, int from, int to,
               EdgeStatus status = EdgeStatus::Active) {
    Route edge;
    edge.id = id;
    edge.from = from;
    edge.to = to;
    edge.status = status;
    return edge;
}

}  // namespace

TEST_CASE("ArticulationPoints: empty graph") {
    CHECK(articulationPoints(Graph{}).empty());
}

TEST_CASE("ArticulationPoints: single node is not a cut vertex") {
    Graph graph;
    graph.addFacility(makeNode("A"));
    CHECK(articulationPoints(graph).empty());
}

TEST_CASE("ArticulationPoints: a two-node edge including reverse parallel edge") {
    Graph graph;
    const int a = graph.addFacility(makeNode("A"));
    const int b = graph.addFacility(makeNode("B"));
    graph.addRoute(makeEdge("E1", a, b));
    graph.addRoute(makeEdge("E2", b, a));
    CHECK(articulationPoints(graph).empty());
}

TEST_CASE("ArticulationPoints: every interior vertex of a chain is a cut vertex") {
    Graph graph;
    for (int index = 0; index < 5; ++index) {
        graph.addFacility(makeNode("N" + std::to_string(index)));
    }
    for (int index = 0; index < 4; ++index) {
        graph.addRoute(makeEdge("E" + std::to_string(index), index, index + 1));
    }
    CHECK((articulationPoints(graph) == std::vector<int>{1, 2, 3}));
}

TEST_CASE("ArticulationPoints: a simple cycle has no cut vertices") {
    Graph graph;
    for (int index = 0; index < 5; ++index) {
        graph.addFacility(makeNode("N" + std::to_string(index)));
    }
    for (int index = 0; index < 5; ++index) {
        graph.addRoute(makeEdge("E" + std::to_string(index), index, (index + 1) % 5));
    }
    CHECK(articulationPoints(graph).empty());
}

TEST_CASE("ArticulationPoints: center of a star is the sole cut vertex") {
    Graph graph;
    const int center = graph.addFacility(makeNode("C"));
    for (int index = 0; index < 5; ++index) {
        const int leaf = graph.addFacility(makeNode("L" + std::to_string(index)));
        graph.addRoute(makeEdge("E" + std::to_string(index), center, leaf));
    }
    CHECK(articulationPoints(graph) == std::vector<int>{center});
}

TEST_CASE("ArticulationPoints: disconnected components are handled independently") {
    Graph graph;
    for (int index = 0; index < 6; ++index) {
        graph.addFacility(makeNode("N" + std::to_string(index)));
    }
    graph.addRoute(makeEdge("E1", 0, 1));
    graph.addRoute(makeEdge("E2", 1, 2));
    graph.addRoute(makeEdge("E3", 3, 4));
    graph.addRoute(makeEdge("E4", 4, 5));
    graph.addRoute(makeEdge("E5", 5, 3));
    CHECK(articulationPoints(graph) == std::vector<int>{1});
}

TEST_CASE("ArticulationPoints: a failed cycle edge creates a chain") {
    Graph graph;
    for (int index = 0; index < 4; ++index) {
        graph.addFacility(makeNode("N" + std::to_string(index)));
    }
    graph.addRoute(makeEdge("E0", 0, 1));
    graph.addRoute(makeEdge("E1", 1, 2));
    graph.addRoute(makeEdge("E2", 2, 3));
    graph.addRoute(makeEdge("E3", 3, 0));
    graph.edge(3).status = EdgeStatus::Failed;
    CHECK((articulationPoints(graph) == std::vector<int>{1, 2}));
}

TEST_CASE("ArticulationPoints: parallel routes prevent a false cut vertex") {
    Graph graph;
    graph.addFacility(makeNode("A"));
    graph.addFacility(makeNode("B"));
    graph.addFacility(makeNode("C"));
    graph.addRoute(makeEdge("E1", 0, 1));
    graph.addRoute(makeEdge("E2", 0, 1));
    graph.addRoute(makeEdge("E3", 1, 2));
    CHECK((articulationPoints(graph) == std::vector<int>{1}));
}

TEST_CASE("ArticulationPoints: failed nodes and their incident edges are ignored") {
    Graph graph;
    graph.addFacility(makeNode("A"));
    graph.addFacility(makeNode("F", NodeStatus::Failed));
    graph.addFacility(makeNode("B"));
    graph.addRoute(makeEdge("E1", 0, 1));
    graph.addRoute(makeEdge("E2", 1, 2));
    CHECK(articulationPoints(graph).empty());
}
