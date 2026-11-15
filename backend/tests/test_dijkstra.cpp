#include "doctest.h"

#include <string>

#include "gridflow/algo/Dijkstra.h"
#include "gridflow/core/NetworkLoader.h"
#include "gridflow/sim/Allocation.h"

namespace {

gridflow::Facility facility(const char* id, gridflow::NodeType type = gridflow::NodeType::Processing) {
    gridflow::Facility result;
    result.id = id; result.name = id; result.type = type; result.capacity = 100;
    return result;
}

gridflow::Route route(const char* id, int from, int to, double cost, double time) {
    gridflow::Route result;
    result.id = id; result.from = from; result.to = to;
    result.capacity = result.effectiveCapacity = 100;
    result.cost = cost; result.transitTime = time;
    return result;
}

}  // namespace

TEST_CASE("Dijkstra: cost and time select distinct deterministic paths") {
    gridflow::Graph graph;
    for (const char* id : {"S", "A", "B", "D"}) graph.addFacility(facility(id));
    graph.addRoute(route("SA", 0, 1, 1, 8)); graph.addRoute(route("AD", 1, 3, 1, 8));
    graph.addRoute(route("SB", 0, 2, 3, 1)); graph.addRoute(route("BD", 2, 3, 3, 1));
    auto cost = gridflow::shortestPath(graph, 0, 3, gridflow::Metric::Cost);
    auto time = gridflow::shortestPath(graph, 0, 3, gridflow::Metric::Time);
    REQUIRE(cost.found); REQUIRE(time.found);
    CHECK(cost.nodes == std::vector<int>{0, 1, 3});
    CHECK(time.nodes == std::vector<int>{0, 2, 3});
    CHECK(cost.totalCost == doctest::Approx(2));
    CHECK(time.totalTime == doctest::Approx(2));
    CHECK_FALSE(gridflow::shortestPath(graph, 3, 0, gridflow::Metric::Cost).found);
}

TEST_CASE("Dijkstra: failed nodes, failed edges, zero capacity and same-node paths") {
    gridflow::Graph graph;
    graph.addFacility(facility("S")); graph.addFacility(facility("D"));
    auto edge = route("E", 0, 1, 1, 1); edge.effectiveCapacity = 0; graph.addRoute(edge);
    CHECK_FALSE(gridflow::shortestPath(graph, 0, 1, gridflow::Metric::Cost).found);
    graph.edge(0).effectiveCapacity = 10; graph.edge(0).status = gridflow::EdgeStatus::Failed;
    CHECK_FALSE(gridflow::shortestPath(graph, 0, 1, gridflow::Metric::Cost).found);
    graph.edge(0).status = gridflow::EdgeStatus::Active; graph.node(1).status = gridflow::NodeStatus::Failed;
    CHECK_FALSE(gridflow::shortestPath(graph, 0, 1, gridflow::Metric::Cost).found);
    graph.node(1).status = gridflow::NodeStatus::Active;
    auto same = gridflow::shortestPath(graph, 0, 0, gridflow::Metric::Cost);
    CHECK(same.found); CHECK(same.nodes == std::vector<int>{0});
}

TEST_CASE("Dijkstra: reverse search returns supplier-to-demand order") {
    gridflow::Graph graph;
    graph.addFacility(facility("S")); graph.addFacility(facility("A"));
    graph.addFacility(facility("D", gridflow::NodeType::DemandCentre));
    graph.addRoute(route("SA", 0, 1, 1, 1)); graph.addRoute(route("AD", 1, 2, 1, 1));
    const std::vector<double> edgeResidual{10, 10};
    const std::vector<double> nodeResidual{10, 10, 10};
    const std::vector<double> supplyRemaining{10, 0, 0};
    auto path = gridflow::reverseResidualPath(graph, 2, edgeResidual, nodeResidual, supplyRemaining);
    REQUIRE(path.found);
    CHECK(path.supplier == 0);
    CHECK(path.nodes == std::vector<int>{0, 1, 2});
    CHECK(path.edges == std::vector<int>{0, 1});
}

TEST_CASE("Dijkstra: reference route agrees with the hand calculation") {
    auto graph = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    gridflow::runAllocation(graph);
    auto path = gridflow::shortestPath(graph, graph.findNode("S1"), graph.findNode("D1"),
                                       gridflow::Metric::Cost);
    REQUIRE(path.found);
    const std::vector<int> expectedEdges{graph.findEdge("E01"), graph.findEdge("E12"),
        graph.findEdge("E25"), graph.findEdge("E33")};
    // Cost 2+2+1+1=6: E01, E12, E25, E33 is the cheapest S1-to-D1 route.
    CHECK(path.edges == expectedEdges);
    CHECK(path.totalCost == doctest::Approx(6.0));
}
