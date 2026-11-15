#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "gridflow/algo/Dijkstra.h"
#include "gridflow/sim/Allocation.h"
#include "gridflow/sim/Criticality.h"
#include "sim/DisruptionQueue.h"
#include "sim/EventLog.h"
#include "sim/ImpactAnalyzer.h"
#include "gridflow/sim/RecoveryPlanner.h"
#include "gridflow/sim/ScenarioLibrary.h"

namespace gridflow {

struct AppliedDisruption { std::size_t seq = 0; Disruption disruption; };
struct StateSnapshot { Graph graph; int version = 0; double baselineServedPercent = 100.0; };
struct RoutedPath { SearchPath path; Graph graph; };
struct DisruptionOutcome {
    std::vector<AppliedDisruption> disruptions;
    ImpactReport impact;
    RecoveryPlanData recovery;
    StateSnapshot state;
};
struct CriticalNodesResult {
    std::vector<std::string> articulationPoints;
    std::vector<CriticalityEntry> ranking;
};

class Simulation {
public:
    Simulation(Graph baseline, const std::string& scenariosPath);
    Graph snapshot() const;
    StateSnapshot stateSnapshot() const;
    DisruptionOutcome applyDisruption(const Disruption& disruption);
    DisruptionOutcome runScenario(const std::string& id);
    CriticalNodesResult criticalNodes();
    SearchPath route(const std::string& fromId, const std::string& toId, Metric metric) const;
    RoutedPath routeWithSnapshot(const std::string& fromId, const std::string& toId, Metric metric) const;
    std::vector<Event> events() const;
    std::vector<Scenario> scenarios() const;
    void reset();
    StateSnapshot resetAndSnapshot();
    int version() const;
    double baselineServedPercent() const;

private:
    DisruptionOutcome applyBatch(const std::vector<Disruption>& disruptions, bool scenario,
                                 const std::string& scenarioId);
    mutable std::mutex mutex_;
    Graph baseline_;
    Graph working_;
    ScenarioLibrary scenarios_;
    DisruptionQueue queue_;
    EventLog eventLog_;
    AllocationResult baselineAllocation_;
    std::vector<CriticalityEntry> criticalityCache_;
    bool criticalityValid_ = false;
    int version_ = 0;
    double baselineServedPercent_ = 100.0;
};

}  // namespace gridflow
