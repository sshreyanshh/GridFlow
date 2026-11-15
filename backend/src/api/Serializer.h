#pragma once

#include <string>
#include <vector>

#include "gridflow/core/Graph.h"
#include "../sim/EventLog.h"
#include "../sim/ImpactAnalyzer.h"
#include "json.hpp"

namespace gridflow {

struct Allocation {
    std::string demandCentreId;
    std::vector<std::string> path;
    std::vector<std::string> edgeIds;
    double flow = 0.0;
    double totalCost = 0.0;
    double totalTime = 0.0;
};

struct UnresolvedDemand {
    std::string nodeId;
    double shortage = 0.0;
    std::string reason;
};

struct Recommendation {
    std::string type;
    std::string targetId;
    int priority = 0;
    std::string message;
};

struct RecoveryPlan {
    std::vector<Allocation> allocations;
    std::vector<UnresolvedDemand> unresolved;
    std::vector<Recommendation> recommendations;
};

struct PathResult {
    std::string metric;
    bool found = false;
    std::vector<std::string> path;
    std::vector<std::string> edgeIds;
    double totalCost = 0.0;
    double totalTime = 0.0;
    double bottleneckCapacity = 0.0;
};

// O(V + E): serializes the graph's static nodes and directed edges.
nlohmann::json toJson(const Graph& graph);

// O(V + E): serializes dynamic node/edge state and its aggregate summary.
// Graph has no version or baseline metadata; those contract fields use 0 and 100%.
nlohmann::json stateToJson(const Graph& graph);

// O(V + E + S): serializes exposure lists, components, and sorted shortages.
nlohmann::json impactToJson(const ImpactReport& impact);

// O(A + U + R + P): P is the total path and edge-ID entries across allocations.
nlohmann::json recoveryToJson(const RecoveryPlan& recovery);

// O(P + E): serializes a route path and its edge IDs.
nlohmann::json pathToJson(const PathResult& path);

// O(N + R): R is the total number of reference IDs in the events.
nlohmann::json eventsToJson(const std::vector<Event>& events);

// O(D): serializes an error envelope; details is copied into the error object.
nlohmann::json errorToJson(const std::string& code, const std::string& message,
                           const nlohmann::json& details = nlohmann::json::object());

}  // namespace gridflow
