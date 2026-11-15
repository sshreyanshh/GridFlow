#include <stdexcept>
#include <string>

#include "doctest.h"
#include "algo/Traversal.h"

using namespace gridflow;

namespace {

Facility makeNode(const std::string& id, NodeType type = NodeType::Processing,
                  NodeStatus status = NodeStatus::Active) {
    Facility node;
    node.id = id;
    node.name = id;
    node.type = type;
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

TEST_CASE("Traversal: empty graph") {
    Graph graph;
    const ComponentInfo components = connectedComponents(graph);
    CHECK(components.count == 0);
    CHECK(components.isolatedDemandCentres.empty());
    CHECK_THROWS_AS(bfsDownstream(graph, 0), std::out_of_range);
}

TEST_CASE("Traversal: BFS follows outgoing routes and ignores failed nodes and edges") {
    Graph graph;
    const int source = graph.addFacility(makeNode("S", NodeType::Source));
    const int hub = graph.addFacility(makeNode("H"));
    const int demand = graph.addFacility(makeNode("D", NodeType::DemandCentre));
    const int blocked = graph.addFacility(
        makeNode("X", NodeType::Processing, NodeStatus::Failed));
    const int disconnected = graph.addFacility(makeNode("Y"));
    graph.addRoute(makeEdge("E1", source, hub));
    graph.addRoute(makeEdge("E2", hub, demand));
    graph.addRoute(makeEdge("E3", source, blocked));
    graph.addRoute(makeEdge("E4", source, disconnected, EdgeStatus::Failed));

    const TraversalResult result = bfsDownstream(graph, source);
    REQUIRE(result.exposedNodes.size() == 3);
    CHECK(result.exposedNodes[0] == "S");
    CHECK(result.exposedNodes[1] == "H");
    CHECK(result.exposedNodes[2] == "D");
    REQUIRE(result.exposedEdges.size() == 2);
    CHECK(result.exposedEdges[0] == "E1");
    CHECK(result.exposedEdges[1] == "E2");
}

TEST_CASE("Traversal: a failed intermediate node blocks downstream exposure") {
    Graph graph;
    const int source = graph.addFacility(makeNode("S", NodeType::Source));
    const int failed = graph.addFacility(
        makeNode("F", NodeType::Processing, NodeStatus::Failed));
    const int demand = graph.addFacility(makeNode("D", NodeType::DemandCentre));
    graph.addRoute(makeEdge("E1", source, failed));
    graph.addRoute(makeEdge("E2", failed, demand));

    const TraversalResult result = bfsDownstream(graph, source);
    REQUIRE(result.exposedNodes.size() == 1);
    CHECK(result.exposedNodes[0] == "S");
    CHECK(result.exposedEdges.empty());
}

TEST_CASE("Traversal: components cover disconnected islands and report isolated demand") {
    Graph graph;
    const int source = graph.addFacility(makeNode("S", NodeType::Source));
    const int hub = graph.addFacility(makeNode("H"));
    const int demand = graph.addFacility(makeNode("D", NodeType::DemandCentre));
    const int island = graph.addFacility(makeNode("I"));
    const int isolatedDemand = graph.addFacility(makeNode("D2", NodeType::DemandCentre));
    graph.addRoute(makeEdge("E1", source, hub));
    graph.addRoute(makeEdge("E2", hub, demand));

    const ComponentInfo components = connectedComponents(graph);
    CHECK(components.count == 3);
    REQUIRE(components.isolatedDemandCentres.size() == 1);
    CHECK(components.isolatedDemandCentres[0] == "D2");
    CHECK(island != source);
}

TEST_CASE("Traversal: a failed edge splits undirected components") {
    Graph graph;
    const int source = graph.addFacility(makeNode("S", NodeType::Source));
    const int demand = graph.addFacility(makeNode("D", NodeType::DemandCentre));
    const int failed = graph.addFacility(
        makeNode("F", NodeType::Processing, NodeStatus::Failed));
    graph.addRoute(makeEdge("E1", source, demand, EdgeStatus::Failed));
    graph.addRoute(makeEdge("E2", failed, demand));

    const ComponentInfo components = connectedComponents(graph);
    CHECK(components.count == 2);
    REQUIRE(components.isolatedDemandCentres.size() == 1);
    CHECK(components.isolatedDemandCentres[0] == "D");
}
