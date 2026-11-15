#include "Serializer.h"

#include <cstddef>

namespace gridflow {
namespace {

nlohmann::json nodeToJson(const Facility& node) {
    nlohmann::json priority = nullptr;
    if (node.type == NodeType::DemandCentre) priority = node.priority;

    return {
        {"id", node.id},
        {"name", node.name},
        {"type", toText(node.type)},
        {"capacity", node.capacity},
        {"supply", node.supply},
        {"demand", node.demand},
        {"priority", priority},
        {"position", {{"x", node.x}, {"y", node.y}}}
    };
}

}  // namespace

nlohmann::json toJson(const Graph& graph) {
    nlohmann::json nodes = nlohmann::json::array();
    nlohmann::json edges = nlohmann::json::array();

    for (int index = 0; index < graph.nodeCount(); ++index) {
        nodes.push_back(nodeToJson(graph.node(index)));
    }
    for (int index = 0; index < graph.edgeCount(); ++index) {
        const Route& route = graph.edge(index);
        edges.push_back({
            {"id", route.id},
            {"from", graph.node(route.from).id},
            {"to", graph.node(route.to).id},
            {"capacity", route.capacity},
            {"cost", route.cost},
            {"transitTime", route.transitTime}
        });
    }

    return {{"nodes", nodes}, {"edges", edges}};
}

nlohmann::json stateToJson(const Graph& graph) {
    nlohmann::json nodes = nlohmann::json::array();
    nlohmann::json edges = nlohmann::json::array();
    double totalSupply = 0.0;
    double totalDemand = 0.0;
    double totalServed = 0.0;

    for (int index = 0; index < graph.nodeCount(); ++index) {
        const Facility& node = graph.node(index);
        totalSupply += node.effectiveSupply;
        totalDemand += node.effectiveDemand;
        totalServed += node.served;
        nodes.push_back({
            {"id", node.id},
            {"status", toText(node.status)},
            {"effectiveSupply", node.effectiveSupply},
            {"effectiveDemand", node.effectiveDemand},
            {"served", node.served},
            {"shortage", node.shortage},
            {"throughput", node.throughput}
        });
    }
    for (int index = 0; index < graph.edgeCount(); ++index) {
        const Route& route = graph.edge(index);
        edges.push_back({
            {"id", route.id},
            {"status", toText(route.status)},
            {"effectiveCapacity", route.effectiveCapacity},
            {"flow", route.flow}
        });
    }

    const double unmetDemand = totalDemand - totalServed;
    const double servedPercent =
        totalDemand == 0.0 ? 100.0 : (totalServed / totalDemand) * 100.0;
    return {
        {"version", 0},
        {"nodes", nodes},
        {"edges", edges},
        {"summary", {
            {"totalSupply", totalSupply},
            {"totalDemand", totalDemand},
            {"totalServed", totalServed},
            {"unmetDemand", unmetDemand},
            {"servedPercent", servedPercent},
            {"baselineServedPercent", 100.0}
        }}
    };
}

nlohmann::json impactToJson(const ImpactReport& impact) {
    nlohmann::json exposedNodes = nlohmann::json::array();
    nlohmann::json exposedEdges = nlohmann::json::array();
    nlohmann::json isolatedDemandCentres = nlohmann::json::array();
    nlohmann::json shortages = nlohmann::json::array();
    for (const std::string& id : impact.exposedNodes) exposedNodes.push_back(id);
    for (const std::string& id : impact.exposedEdges) exposedEdges.push_back(id);
    for (const std::string& id : impact.components.isolatedDemandCentres) {
        isolatedDemandCentres.push_back(id);
    }
    for (const Shortage& shortage : impact.shortages) {
        shortages.push_back({
            {"nodeId", shortage.nodeId},
            {"name", shortage.name},
            {"priority", shortage.priority},
            {"demand", shortage.demand},
            {"served", shortage.served},
            {"shortage", shortage.shortage}
        });
    }

    return {
        {"exposedNodes", exposedNodes},
        {"exposedEdges", exposedEdges},
        {"components", {
            {"count", impact.components.count},
            {"isolatedDemandCentres", isolatedDemandCentres}
        }},
        {"shortages", shortages},
        {"unmetDemandBefore", impact.unmetDemandBefore},
        {"unmetDemandAfter", impact.unmetDemandAfter}
    };
}

nlohmann::json recoveryToJson(const RecoveryPlan& recovery) {
    nlohmann::json allocations = nlohmann::json::array();
    nlohmann::json unresolved = nlohmann::json::array();
    nlohmann::json recommendations = nlohmann::json::array();

    for (const Allocation& allocation : recovery.allocations) {
        allocations.push_back({
            {"demandCentreId", allocation.demandCentreId},
            {"path", allocation.path},
            {"edgeIds", allocation.edgeIds},
            {"flow", allocation.flow},
            {"totalCost", allocation.totalCost},
            {"totalTime", allocation.totalTime}
        });
    }
    for (const UnresolvedDemand& item : recovery.unresolved) {
        unresolved.push_back({
            {"nodeId", item.nodeId},
            {"shortage", item.shortage},
            {"reason", item.reason}
        });
    }
    for (const Recommendation& recommendation : recovery.recommendations) {
        recommendations.push_back({
            {"type", recommendation.type},
            {"targetId", recommendation.targetId},
            {"priority", recommendation.priority},
            {"message", recommendation.message}
        });
    }

    return {
        {"allocations", allocations},
        {"unresolved", unresolved},
        {"recommendations", recommendations}
    };
}

nlohmann::json pathToJson(const PathResult& path) {
    return {
        {"metric", path.metric},
        {"found", path.found},
        {"path", path.path},
        {"edgeIds", path.edgeIds},
        {"totalCost", path.totalCost},
        {"totalTime", path.totalTime},
        {"bottleneckCapacity", path.bottleneckCapacity}
    };
}

nlohmann::json eventsToJson(const std::vector<Event>& events) {
    nlohmann::json serialized = nlohmann::json::array();
    for (const Event& event : events) {
        serialized.push_back({
            {"seq", event.seq},
            {"tick", event.tick},
            {"kind", event.kind},
            {"message", event.message},
            {"refs", event.refs}
        });
    }
    return {{"events", serialized}};
}

nlohmann::json errorToJson(const std::string& code, const std::string& message,
                           const nlohmann::json& details) {
    return {
        {"ok", false},
        {"error", {
            {"code", code},
            {"message", message},
            {"details", details}
        }}
    };
}

}  // namespace gridflow
