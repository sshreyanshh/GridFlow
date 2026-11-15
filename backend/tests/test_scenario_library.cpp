#include "doctest.h"

#include "gridflow/core/NetworkLoader.h"
#include "gridflow/sim/ScenarioLibrary.h"
#include "sim/DisruptionEngine.h"

TEST_CASE("ScenarioLibrary: loads all presets and resolves ids") {
    auto graph = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    gridflow::ScenarioLibrary library(std::string(GRIDFLOW_DATA_DIR) + "/scenarios.json", graph);
    REQUIRE(library.scenarios().size() == 5);
    CHECK(library.find("SC01") != nullptr);
    CHECK(library.find("SC99") == nullptr);
    for (const auto& scenario : library.scenarios()) {
        gridflow::Graph candidate = graph;
        for (const auto& event : scenario.disruptions) CHECK_NOTHROW(gridflow::applyDisruption(candidate, event));
    }
}

