#include "doctest.h"

#include "gridflow/core/NetworkLoader.h"
#include "gridflow/sim/Allocation.h"
#include "gridflow/sim/RecoveryPlanner.h"
#include "sim/DisruptionEngine.h"

TEST_CASE("RecoveryPlanner: emits disruption and shortage guidance") {
    auto baseline = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    auto baselineAllocation = gridflow::runAllocation(baseline);
    auto current = baseline;
    gridflow::Disruption event;
    event.type = gridflow::DisruptionType::NodeOutage;
    event.targetId = "H2";
    gridflow::applyDisruption(current, event);
    auto allocation = gridflow::runAllocation(current);
    auto plan = gridflow::buildRecoveryPlan(current, allocation, baselineAllocation, {event});
    REQUIRE_FALSE(plan.recommendations.empty());
    bool restore = false;
    for (const auto& item : plan.recommendations) if (item.type == "RESTORE") restore = true;
    CHECK(restore);
}

