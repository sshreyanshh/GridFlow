#include "doctest.h"

#include <atomic>
#include <thread>

#include "gridflow/core/NetworkLoader.h"
#include "gridflow/sim/Simulation.h"

TEST_CASE("Simulation: applies disruptions, increments version, and resets") {
    auto baseline = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    gridflow::Simulation simulation(std::move(baseline), std::string(GRIDFLOW_DATA_DIR) + "/scenarios.json");
    CHECK(simulation.version() == 0);
    CHECK(simulation.baselineServedPercent() >= 99.5);
    gridflow::Disruption outage;
    outage.type = gridflow::DisruptionType::NodeOutage; outage.targetId = "H2";
    auto result = simulation.applyDisruption(outage);
    REQUIRE(result.disruptions.size() == 1);
    CHECK(simulation.snapshot().node(simulation.snapshot().findNode("H2")).status == gridflow::NodeStatus::Failed);
    CHECK(simulation.version() == 1);
    simulation.reset();
    CHECK(simulation.version() == 2);
    CHECK(simulation.snapshot().node(simulation.snapshot().findNode("H2")).status == gridflow::NodeStatus::Active);
    CHECK(simulation.events().back().kind == "RESET");
}

TEST_CASE("Simulation: invalid scenario mutation preserves working state and version") {
    auto baseline = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    gridflow::Simulation simulation(std::move(baseline), std::string(GRIDFLOW_DATA_DIR) + "/scenarios.json");
    gridflow::Disruption bad;
    bad.type = gridflow::DisruptionType::NodeOutage; bad.targetId = "missing";
    CHECK_THROWS(simulation.applyDisruption(bad));
    CHECK(simulation.version() == 0);
    CHECK(simulation.snapshot().node(simulation.snapshot().findNode("H2")).status == gridflow::NodeStatus::Active);
}

TEST_CASE("Simulation: runs scenarios and serializes concurrent access") {
    auto baseline = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    gridflow::Simulation simulation(std::move(baseline), std::string(GRIDFLOW_DATA_DIR) + "/scenarios.json");
    auto scenario = simulation.runScenario("SC04");
    CHECK(scenario.disruptions.size() == 2);
    CHECK(simulation.snapshot().node(simulation.snapshot().findNode("D1")).effectiveDemand == doctest::Approx(840));
    simulation.reset();
    std::atomic<bool> failed{false};
    std::vector<std::thread> workers;
    for (int worker = 0; worker < 4; ++worker) {
        workers.emplace_back([&simulation, &failed, worker] {
            try {
                for (int i = 0; i < 6; ++i) {
                    if (worker < 2) {
                        gridflow::Disruption event;
                        event.type = gridflow::DisruptionType::NodeOutage;
                        event.targetId = worker == 0 ? "H1" : "H3";
                        simulation.applyDisruption(event);
                    } else {
                        (void)simulation.snapshot();
                        (void)simulation.events();
                    }
                }
            } catch (...) { failed = true; }
        });
    }
    for (auto& worker : workers) worker.join();
    CHECK_FALSE(failed.load());
    CHECK(simulation.version() == 14);
}

TEST_CASE("Simulation: each reference scenario applies to the baseline") {
    for (const std::string id : {"SC01", "SC02", "SC03", "SC04", "SC05"}) {
        auto baseline = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
        gridflow::Simulation simulation(std::move(baseline), std::string(GRIDFLOW_DATA_DIR) + "/scenarios.json");
        auto result = simulation.runScenario(id);
        CHECK_FALSE(result.disruptions.empty());
        auto state = simulation.snapshot();
        if (id == "SC01") CHECK(state.node(state.findNode("T1")).status == gridflow::NodeStatus::Failed);
        if (id == "SC02") CHECK(state.node(state.findNode("H2")).status == gridflow::NodeStatus::Failed);
        if (id == "SC03") CHECK(state.node(state.findNode("T2")).effectiveSupply < state.node(state.findNode("T2")).supply);
        if (id == "SC04") CHECK(state.node(state.findNode("D1")).effectiveDemand == doctest::Approx(840));
        if (id == "SC05") CHECK(state.node(state.findNode("P2")).status == gridflow::NodeStatus::Failed);
    }
}

