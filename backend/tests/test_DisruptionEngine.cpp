#include <limits>
#include <string>

#include "doctest.h"
#include "sim/DisruptionEngine.h"
#include "sim/DisruptionQueue.h"

using namespace gridflow;

namespace {

Facility makeNode(const std::string& id, NodeType type) {
    Facility node;
    node.id = id;
    node.name = id;
    node.type = type;
    node.effectiveSupply = 101.0;
    node.effectiveDemand = 202.0;
    node.served = 33.0;
    node.shortage = 44.0;
    node.throughput = 55.0;
    return node;
}

Route makeEdge(const std::string& id, int from, int to) {
    Route edge;
    edge.id = id;
    edge.from = from;
    edge.to = to;
    edge.effectiveCapacity = 66.0;
    edge.flow = 77.0;
    return edge;
}

Disruption makeDisruption(DisruptionType type, const std::string& target, double severity) {
    return Disruption{type, target, severity};
}

}  // namespace

TEST_CASE("DisruptionQueue: preserves FIFO order") {
    DisruptionQueue queue;
    CHECK(queue.empty());
    queue.enqueue(makeDisruption(DisruptionType::NodeOutage, "A", 1.0));
    queue.enqueue(makeDisruption(DisruptionType::DemandSurge, "D1", 0.4));
    CHECK(queue.size() == 2);
    CHECK(queue.front().targetId == "A");
    CHECK(queue.dequeue().targetId == "A");
    CHECK(queue.dequeue().targetId == "D1");
    CHECK(queue.empty());
}

TEST_CASE("DisruptionEngine: applies node outages") {
    Graph graph;
    graph.addFacility(makeNode("D1", NodeType::DemandCentre));

    applyDisruption(graph, makeDisruption(DisruptionType::NodeOutage, "D1", 0.0));

    CHECK(graph.node(0).status == NodeStatus::Failed);
    CHECK(graph.node(0).severity == 1.0);
    CHECK(graph.node(0).surge == 0.0);
}

TEST_CASE("DisruptionEngine: applies edge disruption status and severity") {
    Graph graph;
    const int source = graph.addFacility(makeNode("S1", NodeType::Source));
    const int demand = graph.addFacility(makeNode("D1", NodeType::DemandCentre));
    graph.addRoute(makeEdge("E1", source, demand));

    applyDisruption(graph, makeDisruption(DisruptionType::EdgeDisruption, "E1", 0.6));
    CHECK(graph.edge(0).status == EdgeStatus::Reduced);
    CHECK(graph.edge(0).severity == 0.6);
    applyDisruption(graph, makeDisruption(DisruptionType::EdgeDisruption, "E1", 1.0));
    CHECK(graph.edge(0).status == EdgeStatus::Failed);
    CHECK(graph.edge(0).severity == 1.0);
}

TEST_CASE("DisruptionEngine: applies supply reduction and demand surge") {
    Graph graph;
    graph.addFacility(makeNode("S1", NodeType::Source));
    graph.addFacility(makeNode("D1", NodeType::DemandCentre));

    applyDisruption(graph, makeDisruption(DisruptionType::SupplyReduction, "S1", 0.4));
    CHECK(graph.node(0).status == NodeStatus::Degraded);
    CHECK(graph.node(0).severity == 0.4);
    applyDisruption(graph, makeDisruption(DisruptionType::DemandSurge, "D1", 0.3));
    CHECK(graph.node(1).status == NodeStatus::Active);
    CHECK(graph.node(1).surge == 0.3);
}

TEST_CASE("DisruptionEngine: same-target disruptions replace previous values") {
    Graph graph;
    graph.addFacility(makeNode("S1", NodeType::Source));
    graph.addFacility(makeNode("D1", NodeType::DemandCentre));

    applyDisruption(graph, makeDisruption(DisruptionType::SupplyReduction, "S1", 0.8));
    applyDisruption(graph, makeDisruption(DisruptionType::SupplyReduction, "S1", 0.2));
    CHECK(graph.node(0).severity == 0.2);
    CHECK(graph.node(0).status == NodeStatus::Degraded);

    applyDisruption(graph, makeDisruption(DisruptionType::DemandSurge, "D1", 0.4));
    applyDisruption(graph, makeDisruption(DisruptionType::DemandSurge, "D1", 0.1));
    CHECK(graph.node(1).surge == 0.1);
}

TEST_CASE("DisruptionEngine: invalid IDs use the contract error codes") {
    Graph graph;
    try {
        applyDisruption(graph, makeDisruption(DisruptionType::NodeOutage, "missing", 1.0));
        FAIL("Expected a missing-node error");
    } catch (const InvalidDisruption& error) {
        CHECK(error.code() == "NODE_NOT_FOUND");
    }

    try {
        applyDisruption(graph, makeDisruption(DisruptionType::EdgeDisruption, "missing", 0.5));
        FAIL("Expected a missing-edge error");
    } catch (const InvalidDisruption& error) {
        CHECK(error.code() == "EDGE_NOT_FOUND");
    }
}

TEST_CASE("DisruptionEngine: invalid targets, severity, and target types are rejected") {
    Graph graph;
    graph.addFacility(makeNode("P1", NodeType::Processing));
    graph.addFacility(makeNode("S1", NodeType::Source));
    graph.addFacility(makeNode("D1", NodeType::DemandCentre));
    graph.addRoute(makeEdge("E1", 1, 2));

    try {
        applyDisruption(graph, makeDisruption(DisruptionType::SupplyReduction, "missing", 0.5));
        FAIL("Expected a missing-node error");
    } catch (const InvalidDisruption& error) {
        CHECK(error.code() == "NODE_NOT_FOUND");
    }

    try {
        applyDisruption(graph, makeDisruption(DisruptionType::EdgeDisruption, "missing", 0.5));
        FAIL("Expected a missing-edge error");
    } catch (const InvalidDisruption& error) {
        CHECK(error.code() == "EDGE_NOT_FOUND");
    }

    CHECK_THROWS_AS(applyDisruption(
                        graph, makeDisruption(DisruptionType::DemandSurge, "P1", 0.5)),
                    InvalidDisruption);
    CHECK_THROWS_AS(applyDisruption(
                        graph, makeDisruption(DisruptionType::DemandSurge, "D1", 0.0)),
                    InvalidDisruption);
    CHECK_THROWS_AS(applyDisruption(
                        graph, makeDisruption(DisruptionType::EdgeDisruption, "E1", 1.1)),
                    InvalidDisruption);
    CHECK_THROWS_AS(applyDisruption(
                        graph, makeDisruption(DisruptionType::SupplyReduction, "S1",
                                              std::numeric_limits<double>::quiet_NaN())),
                    InvalidDisruption);

    const Disruption unknownType{static_cast<DisruptionType>(999), "D1", 0.5};
    CHECK_THROWS_AS(applyDisruption(graph, unknownType), InvalidDisruption);
}

TEST_CASE("DisruptionEngine: allocation-owned fields remain untouched") {
    Graph graph;
    graph.addFacility(makeNode("S1", NodeType::Source));
    graph.addFacility(makeNode("D1", NodeType::DemandCentre));
    graph.addRoute(makeEdge("E1", 0, 1));

    applyDisruption(graph, makeDisruption(DisruptionType::SupplyReduction, "S1", 0.5));
    applyDisruption(graph, makeDisruption(DisruptionType::DemandSurge, "D1", 0.4));
    applyDisruption(graph, makeDisruption(DisruptionType::EdgeDisruption, "E1", 0.7));

    CHECK(graph.node(0).effectiveSupply == 101.0);
    CHECK(graph.node(0).effectiveDemand == 202.0);
    CHECK(graph.node(0).served == 33.0);
    CHECK(graph.node(0).shortage == 44.0);
    CHECK(graph.node(0).throughput == 55.0);
    CHECK(graph.node(1).effectiveSupply == 101.0);
    CHECK(graph.node(1).effectiveDemand == 202.0);
    CHECK(graph.node(1).served == 33.0);
    CHECK(graph.node(1).shortage == 44.0);
    CHECK(graph.node(1).throughput == 55.0);
    CHECK(graph.edge(0).effectiveCapacity == 66.0);
    CHECK(graph.edge(0).flow == 77.0);
}
