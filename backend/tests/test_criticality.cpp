#include "doctest.h"

#include "gridflow/core/NetworkLoader.h"
#include "gridflow/sim/Allocation.h"
#include "gridflow/sim/Criticality.h"

TEST_CASE("Criticality: ranks the real network without mutating it") {
    auto graph = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    gridflow::runAllocation(graph);
    const auto originalServed = graph.node(graph.findNode("D1")).served;
    auto ranking = gridflow::rankCriticality(graph);
    REQUIRE_FALSE(ranking.empty());
    CHECK(ranking.front().increasePercent > 0.0);
    for (std::size_t i = 1; i < ranking.size(); ++i)
        CHECK(ranking[i - 1].unmetDemandIfRemoved >= ranking[i].unmetDemandIfRemoved);
    CHECK(graph.node(graph.findNode("D1")).served == doctest::Approx(originalServed));
}

