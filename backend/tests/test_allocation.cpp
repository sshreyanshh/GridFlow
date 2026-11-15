#include "doctest.h"

#include <string>
#include <utility>

#include "gridflow/core/NetworkLoader.h"
#include "gridflow/sim/Allocation.h"

TEST_CASE("Allocation: real network serves the required baseline and is repeatable") {
    gridflow::Graph graph = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    gridflow::AllocationResult first = gridflow::runAllocation(graph);
    double demand = 0.0, served = 0.0;
    for (int i = 0; i < graph.nodeCount(); ++i) {
        demand += graph.node(i).effectiveDemand;
        served += graph.node(i).served;
    }
    CHECK(served / demand >= 0.995);
    for (const auto& item : first.unresolved) CHECK(graph.node(item.node).priority != 1);
    gridflow::Graph secondGraph = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    auto second = gridflow::runAllocation(secondGraph);
    CHECK(first.allocations.size() == second.allocations.size());
    for (std::size_t i = 0; i < first.allocations.size(); ++i) {
        CHECK(first.allocations[i].demandNode == second.allocations[i].demandNode);
        CHECK(first.allocations[i].pathEdges == second.allocations[i].pathEdges);
        CHECK(first.allocations[i].flow == doctest::Approx(second.allocations[i].flow));
    }
}

TEST_CASE("Allocation: failed demand is recorded without serving") {
    gridflow::Graph graph = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    int demand = graph.findNode("D1");
    graph.node(demand).status = gridflow::NodeStatus::Failed;
    auto result = gridflow::runAllocation(graph);
    CHECK(graph.node(demand).served == 0.0);
    bool found = false;
    for (const auto& item : result.unresolved) if (item.node == demand) {
        found = true; CHECK(item.reason == gridflow::Reason::NoPath);
    }
    CHECK(found);
}

TEST_CASE("Allocation: priorities dominate shortage size and exhaustion is reported") {
    gridflow::Graph graph;
    gridflow::Facility source; source.id = "S"; source.name = "S";
    source.type = gridflow::NodeType::Source; source.capacity = 100; source.supply = 100;
    graph.addFacility(source);
    for (const auto& demand : {std::pair<const char*, int>{"P1", 1}, {"P3", 3}}) {
        gridflow::Facility node; node.id = demand.first; node.name = demand.first;
        node.type = gridflow::NodeType::DemandCentre; node.capacity = 100;
        node.demand = demand.second == 1 ? 40 : 80; node.priority = demand.second;
        int index = graph.addFacility(node);
        gridflow::Route edge; edge.id = std::string("E") + demand.first;
        edge.from = 0; edge.to = index; edge.capacity = 100;
        graph.addRoute(edge);
    }
    auto result = gridflow::runAllocation(graph);
    CHECK(graph.node(1).served == doctest::Approx(40));
    CHECK(graph.node(2).served == doctest::Approx(60));
    REQUIRE(result.unresolved.size() == 1);
    CHECK(result.unresolved[0].node == 2);
    CHECK(result.unresolved[0].reason == gridflow::Reason::CapacityExhausted);
}

TEST_CASE("Allocation: an isolated demand has no path") {
    gridflow::Graph graph;
    gridflow::Facility source; source.id = "S"; source.name = "S";
    source.type = gridflow::NodeType::Source; source.capacity = 10; source.supply = 10;
    graph.addFacility(source);
    gridflow::Facility demand; demand.id = "D"; demand.name = "D";
    demand.type = gridflow::NodeType::DemandCentre; demand.capacity = 10;
    demand.demand = 5; demand.priority = 1; graph.addFacility(demand);
    auto result = gridflow::runAllocation(graph);
    REQUIRE(result.unresolved.size() == 1);
    CHECK(result.unresolved[0].reason == gridflow::Reason::NoPath);
}
