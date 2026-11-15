#include "gridflow/sim/Simulation.h"

#include <stdexcept>

#include "algo/ArticulationPoints.h"
#include "gridflow/sim/Criticality.h"
#include "sim/DisruptionEngine.h"

namespace gridflow {

Simulation::Simulation(Graph baseline, const std::string& scenariosPath)
    : baseline_(std::move(baseline)), working_(baseline_), scenarios_(scenariosPath, baseline_) {
    baselineAllocation_ = runAllocation(baseline_);
    working_ = baseline_;
    double demand = 0.0, served = 0.0;
    for (int i = 0; i < baseline_.nodeCount(); ++i) {
        demand += baseline_.node(i).effectiveDemand;
        served += baseline_.node(i).served;
    }
    baselineServedPercent_ = demand == 0.0 ? 100.0 : served / demand * 100.0;
}

Graph Simulation::snapshot() const { std::lock_guard<std::mutex> lock(mutex_); return working_; }
StateSnapshot Simulation::stateSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {working_, version_, baselineServedPercent_};
}
int Simulation::version() const { std::lock_guard<std::mutex> lock(mutex_); return version_; }
double Simulation::baselineServedPercent() const { std::lock_guard<std::mutex> lock(mutex_); return baselineServedPercent_; }
std::vector<Event> Simulation::events() const { std::lock_guard<std::mutex> lock(mutex_); return eventLog_.toVector(); }
std::vector<Scenario> Simulation::scenarios() const { std::lock_guard<std::mutex> lock(mutex_); return scenarios_.scenarios(); }

DisruptionOutcome Simulation::applyDisruption(const Disruption& disruption) {
    return applyBatch({disruption}, false, {});
}

DisruptionOutcome Simulation::runScenario(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const Scenario* scenario = scenarios_.find(id);
    if (scenario == nullptr) throw std::out_of_range("scenario '" + id + "' not found");
    const std::vector<Disruption> events = scenario->disruptions;
    Graph before = working_;
    Graph candidate = working_;
    queue_ = DisruptionQueue();
    std::vector<Disruption> applied;
    try {
        for (const auto& event : events) queue_.enqueue(event);
        while (!queue_.empty()) {
            Disruption event = queue_.dequeue();
            gridflow::applyDisruption(candidate, event);
            applied.push_back(event);
        }
    } catch (...) { queue_ = DisruptionQueue(); throw; }
    AllocationResult allocation = runAllocation(candidate);
    ImpactReport impact = analyzeImpact(before, candidate, applied);
    RecoveryPlanData recovery = buildRecoveryPlan(candidate, allocation, baselineAllocation_, applied);
    eventLog_.append("SCENARIO_RUN", "Scenario " + id + " applied", {id});
    DisruptionOutcome outcome;
    for (const auto& event : applied) {
        eventLog_.append("DISRUPTION_APPLIED", "Disruption applied to " + event.targetId, {event.targetId});
        outcome.disruptions.push_back({eventLog_.size(), event});
    }
    eventLog_.append("ALLOCATION_RUN", "Allocation recalculated", {});
    outcome.impact = std::move(impact); outcome.recovery = std::move(recovery);
    working_ = std::move(candidate); ++version_; criticalityValid_ = false;
    outcome.state = {working_, version_, baselineServedPercent_};
    return outcome;
}

DisruptionOutcome Simulation::applyBatch(const std::vector<Disruption>& disruptions, bool scenario,
                                         const std::string& scenarioId) {
    std::lock_guard<std::mutex> lock(mutex_);
    Graph before = working_;
    Graph candidate = working_;
    queue_ = DisruptionQueue();
    std::vector<Disruption> applied;
    try {
        for (const auto& event : disruptions) queue_.enqueue(event);
        while (!queue_.empty()) {
            Disruption event = queue_.dequeue();
            gridflow::applyDisruption(candidate, event);
            applied.push_back(event);
        }
    } catch (...) { queue_ = DisruptionQueue(); throw; }
    AllocationResult allocation = runAllocation(candidate);
    ImpactReport impact = analyzeImpact(before, candidate, applied);
    RecoveryPlanData recovery = buildRecoveryPlan(candidate, allocation, baselineAllocation_, applied);
    DisruptionOutcome outcome;
    if (scenario) eventLog_.append("SCENARIO_RUN", "Scenario " + scenarioId + " applied", {scenarioId});
    for (const auto& event : applied) {
        eventLog_.append("DISRUPTION_APPLIED", "Disruption applied to " + event.targetId, {event.targetId});
        outcome.disruptions.push_back({eventLog_.size(), event});
    }
    eventLog_.append("ALLOCATION_RUN", "Allocation recalculated", {});
    outcome.impact = std::move(impact); outcome.recovery = std::move(recovery);
    working_ = std::move(candidate); ++version_; criticalityValid_ = false;
    outcome.state = {working_, version_, baselineServedPercent_};
    return outcome;
}

CriticalNodesResult Simulation::criticalNodes() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!criticalityValid_) { criticalityCache_ = rankCriticality(working_); criticalityValid_ = true; }
    CriticalNodesResult result;
    for (int node : articulationPoints(working_)) result.articulationPoints.push_back(working_.node(node).id);
    result.ranking = criticalityCache_;
    return result;
}

SearchPath Simulation::route(const std::string& fromId, const std::string& toId, Metric metric) const {
    std::lock_guard<std::mutex> lock(mutex_);
    int from = working_.findNode(fromId), to = working_.findNode(toId);
    if (from < 0 || to < 0) throw std::out_of_range(from < 0 ? "node '" + fromId + "' not found" : "node '" + toId + "' not found");
    return shortestPath(working_, from, to, metric);
}

RoutedPath Simulation::routeWithSnapshot(const std::string& fromId, const std::string& toId, Metric metric) const {
    std::lock_guard<std::mutex> lock(mutex_);
    int from = working_.findNode(fromId), to = working_.findNode(toId);
    if (from < 0 || to < 0) throw std::out_of_range(from < 0 ? "node '" + fromId + "' not found" : "node '" + toId + "' not found");
    return {shortestPath(working_, from, to, metric), working_};
}

void Simulation::reset() {
    (void)resetAndSnapshot();
}

StateSnapshot Simulation::resetAndSnapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    working_ = baseline_;
    eventLog_.append("RESET", "Simulation reset to baseline", {});
    ++version_; criticalityValid_ = false;
    return {working_, version_, baselineServedPercent_};
}

}  // namespace gridflow
