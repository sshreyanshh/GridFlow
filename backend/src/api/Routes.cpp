#include "gridflow/api/Routes.h"

#include "api/Serializer.h"
#include "sim/DisruptionEngine.h"

#include <functional>
#include <iostream>

namespace gridflow {
namespace {

using Json = nlohmann::json;

void respond(httplib::Response& response, int status, const Json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json");
}

void error(httplib::Response& response, int status, const std::string& code,
           const std::string& message, const Json& details = Json::object()) {
    respond(response, status, errorToJson(code, message, details));
}

Json recoveryJson(const RecoveryPlanData& plan) {
    RecoveryPlan recovery;
    for (const auto& item : plan.allocations) {
        recovery.allocations.push_back({item.demandCentreId, item.path, item.edgeIds,
                                        item.flow, item.totalCost, item.totalTime});
    }
    for (const auto& item : plan.unresolved)
        recovery.unresolved.push_back({item.nodeId, item.shortage, item.reason});
    for (const auto& item : plan.recommendations)
        recovery.recommendations.push_back({item.type, item.targetId, item.priority, item.message});
    return recoveryToJson(recovery);
}

Json scenarioJson(const std::vector<Scenario>& scenarios) {
    Json list = Json::array();
    for (const Scenario& scenario : scenarios) {
        Json disruptions = Json::array();
        for (const Disruption& event : scenario.disruptions) {
            const char* type = "NODE_OUTAGE";
            if (event.type == DisruptionType::EdgeDisruption) type = "EDGE_DISRUPTION";
            if (event.type == DisruptionType::SupplyReduction) type = "SUPPLY_REDUCTION";
            if (event.type == DisruptionType::DemandSurge) type = "DEMAND_SURGE";
            disruptions.push_back({{"type", type}, {"targetId", event.targetId}, {"severity", event.severity}});
        }
        list.push_back({{"id", scenario.id}, {"name", scenario.name},
                        {"description", scenario.description}, {"disruptions", disruptions}});
    }
    return {{"scenarios", list}};
}

Json outcomeJson(const DisruptionOutcome& outcome) {
    Json disruptions = Json::array();
    for (const AppliedDisruption& applied : outcome.disruptions) {
        const Disruption& event = applied.disruption;
        const char* type = event.type == DisruptionType::NodeOutage ? "NODE_OUTAGE" :
            event.type == DisruptionType::EdgeDisruption ? "EDGE_DISRUPTION" :
            event.type == DisruptionType::SupplyReduction ? "SUPPLY_REDUCTION" : "DEMAND_SURGE";
        disruptions.push_back({{"seq", applied.seq}, {"type", type},
                               {"targetId", event.targetId}, {"severity", event.severity}});
    }
    return {{"disruptions", disruptions}, {"impact", impactToJson(outcome.impact)},
            {"recovery", recoveryJson(outcome.recovery)},
            {"state", stateToJson(outcome.state.graph, outcome.state.version,
                                  outcome.state.baselineServedPercent)}};
}

void handle(httplib::Response& response, const std::function<void()>& action) {
    try { action(); }
    catch (const InvalidDisruption& exception) {
        int status = exception.code() == "NODE_NOT_FOUND" || exception.code() == "EDGE_NOT_FOUND" ? 404 : 400;
        error(response, status, exception.code(), exception.what());
    } catch (const std::out_of_range& exception) {
        const std::string message = exception.what();
        if (message.find("scenario") != std::string::npos)
            error(response, 404, "SCENARIO_NOT_FOUND", message);
        else error(response, 404, "NODE_NOT_FOUND", message);
    } catch (const std::exception&) {
        error(response, 500, "INTERNAL_ERROR", "An internal error occurred");
    }
}

}  // namespace

void registerRoutes(httplib::Server& server, Simulation& simulation) {
    server.set_default_headers({{"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}});
    server.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& response) { response.status = 204; });
    server.set_logger([](const httplib::Request& request, const httplib::Response& response) {
        std::cerr << request.method << " " << request.path << " " << response.status << '\n';
    });
    server.set_exception_handler([](const httplib::Request&, httplib::Response& response, std::exception_ptr) {
        error(response, 500, "INTERNAL_ERROR", "An internal error occurred");
    });
    server.set_error_handler([](const httplib::Request&, httplib::Response& response) {
        if (response.body.empty()) error(response, response.status, "NOT_FOUND", "Route not found");
    });
    server.Get("/api/health", [](const httplib::Request&, httplib::Response& response) {
        respond(response, 200, {{"ok", true}, {"data", {{"service", "gridflow"}, {"version", "0.1"}}}});
    });
    server.Get("/api/network", [&simulation](const httplib::Request&, httplib::Response& response) {
        respond(response, 200, {{"ok", true}, {"data", toJson(simulation.snapshot())}});
    });
    server.Get("/api/state", [&simulation](const httplib::Request&, httplib::Response& response) {
        StateSnapshot state = simulation.stateSnapshot();
        respond(response, 200, {{"ok", true}, {"data", stateToJson(state.graph, state.version, state.baselineServedPercent)}});
    });
    server.Post("/api/disruptions", [&simulation](const httplib::Request& request, httplib::Response& response) {
        handle(response, [&] {
            Json body;
            try { body = Json::parse(request.body); }
            catch (...) { error(response, 400, "INVALID_JSON", "Request body must be valid JSON"); return; }
            if (!body.is_object() || !body.contains("type") || !body["type"].is_string() ||
                !body.contains("targetId") || !body["targetId"].is_string() ||
                !body.contains("severity") || !body["severity"].is_number()) {
                error(response, 400, "INVALID_PARAMETER", "type, targetId and severity are required"); return;
            }
            const std::string type = body["type"].get<std::string>();
            Disruption event;
            if (type == "NODE_OUTAGE") event.type = DisruptionType::NodeOutage;
            else if (type == "EDGE_DISRUPTION") event.type = DisruptionType::EdgeDisruption;
            else if (type == "SUPPLY_REDUCTION") event.type = DisruptionType::SupplyReduction;
            else if (type == "DEMAND_SURGE") event.type = DisruptionType::DemandSurge;
            else { error(response, 400, "INVALID_DISRUPTION", "Unknown disruption type"); return; }
            event.targetId = body["targetId"].get<std::string>();
            event.severity = body["severity"].get<double>();
            DisruptionOutcome outcome = simulation.applyDisruption(event);
            respond(response, 200, {{"ok", true}, {"data", outcomeJson(outcome)}});
        });
    });
    server.Get("/api/scenarios", [&simulation](const httplib::Request&, httplib::Response& response) {
        respond(response, 200, {{"ok", true}, {"data", scenarioJson(simulation.scenarios())}});
    });
    server.Post(R"(/api/scenarios/([^/]+)/run)", [&simulation](const httplib::Request& request, httplib::Response& response) {
        handle(response, [&] {
            DisruptionOutcome outcome = simulation.runScenario(request.matches[1]);
            respond(response, 200, {{"ok", true}, {"data", outcomeJson(outcome)}});
        });
    });
    server.Get("/api/analysis/critical-nodes", [&simulation](const httplib::Request&, httplib::Response& response) {
        auto result = simulation.criticalNodes(); Json ranking = Json::array();
        for (const auto& item : result.ranking) ranking.push_back({{"nodeId", item.nodeId}, {"name", item.name},
            {"unmetDemandIfRemoved", item.unmetDemandIfRemoved}, {"increasePercent", item.increasePercent}});
        respond(response, 200, {{"ok", true}, {"data", {{"articulationPoints", result.articulationPoints}, {"ranking", ranking}}}});
    });
    server.Get("/api/routes", [&simulation](const httplib::Request& request, httplib::Response& response) {
        handle(response, [&] {
            if (!request.has_param("from") || !request.has_param("to")) {
                error(response, 400, "INVALID_PARAMETER", "from and to are required"); return;
            }
            std::string metricText = request.has_param("metric") ? request.get_param_value("metric") : "cost";
            if (metricText != "cost" && metricText != "time") { error(response, 400, "INVALID_PARAMETER", "metric must be cost or time"); return; }
            RoutedPath routed = simulation.routeWithSnapshot(request.get_param_value("from"), request.get_param_value("to"),
                metricText == "cost" ? Metric::Cost : Metric::Time);
            const SearchPath& search = routed.path;
            PathResult path; path.metric = metricText; path.found = search.found;
            path.totalCost = search.totalCost; path.totalTime = search.totalTime; path.bottleneckCapacity = search.bottleneck;
            for (int node : search.nodes) path.path.push_back(routed.graph.node(node).id);
            for (int edge : search.edges) path.edgeIds.push_back(routed.graph.edge(edge).id);
            respond(response, 200, {{"ok", true}, {"data", pathToJson(path)}});
        });
    });
    server.Get("/api/events", [&simulation](const httplib::Request&, httplib::Response& response) {
        respond(response, 200, {{"ok", true}, {"data", eventsToJson(simulation.events())}});
    });
    server.Post("/api/reset", [&simulation](const httplib::Request&, httplib::Response& response) {
        StateSnapshot state = simulation.resetAndSnapshot();
        respond(response, 200, {{"ok", true}, {"data", {{"state", stateToJson(state.graph, state.version, state.baselineServedPercent)}}}});
    });
}

}  // namespace gridflow
