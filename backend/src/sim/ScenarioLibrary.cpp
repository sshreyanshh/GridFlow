#include "gridflow/sim/ScenarioLibrary.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include "json.hpp"

namespace gridflow {
namespace {
DisruptionType disruptionType(const std::string& type) {
    if (type == "NODE_OUTAGE") return DisruptionType::NodeOutage;
    if (type == "EDGE_DISRUPTION") return DisruptionType::EdgeDisruption;
    if (type == "SUPPLY_REDUCTION") return DisruptionType::SupplyReduction;
    if (type == "DEMAND_SURGE") return DisruptionType::DemandSurge;
    throw std::runtime_error("unknown disruption type '" + type + "'");
}
}

ScenarioLibrary::ScenarioLibrary(const std::string& path, const Graph& graph) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot read scenarios file '" + path + "'");
    nlohmann::json root;
    try { file >> root; }
    catch (const std::exception& error) { throw std::runtime_error(std::string("invalid scenarios JSON: ") + error.what()); }
    if (!root.is_object() || !root.contains("scenarios") || !root["scenarios"].is_array())
        throw std::runtime_error("scenarios file must contain an array field 'scenarios'");
    for (const auto& input : root["scenarios"]) {
        if (!input.is_object() || !input.contains("id") || !input["id"].is_string() ||
            !input.contains("name") || !input["name"].is_string() ||
            !input.contains("description") || !input["description"].is_string() ||
            !input.contains("disruptions") || !input["disruptions"].is_array())
            throw std::runtime_error("scenario has missing or invalid required fields");
        Scenario scenario;
        scenario.id = input["id"].get<std::string>();
        scenario.name = input["name"].get<std::string>();
        scenario.description = input["description"].get<std::string>();
        if (index_.contains(scenario.id)) throw std::runtime_error("duplicate scenario id '" + scenario.id + "'");
        for (const auto& item : input["disruptions"]) {
            if (!item.is_object() || !item.contains("type") || !item["type"].is_string() ||
                !item.contains("targetId") || !item["targetId"].is_string() ||
                !item.contains("severity") || !item["severity"].is_number())
                throw std::runtime_error("scenario '" + scenario.id + "' has an invalid disruption");
            Disruption event;
            try { event.type = disruptionType(item["type"].get<std::string>()); }
            catch (const std::exception& error) { throw std::runtime_error("scenario '" + scenario.id + "': " + error.what()); }
            event.targetId = item["targetId"].get<std::string>();
            event.severity = item["severity"].get<double>();
            if (!std::isfinite(event.severity) || event.severity < 0.0 || event.severity > 1.0)
                throw std::runtime_error("scenario '" + scenario.id + "' has out-of-range severity");
            if (event.type != DisruptionType::NodeOutage && event.severity <= 0.0)
                throw std::runtime_error("scenario '" + scenario.id + "' disruption severity must be greater than zero");
            int target = event.type == DisruptionType::EdgeDisruption
                ? graph.findEdge(event.targetId) : graph.findNode(event.targetId);
            if (target < 0) throw std::runtime_error("scenario '" + scenario.id + "' target '" + event.targetId + "' was not found");
            const Facility* node = event.type == DisruptionType::EdgeDisruption
                ? nullptr : &graph.node(target);
            if (event.type == DisruptionType::SupplyReduction && node->type != NodeType::Source && node->type != NodeType::ImportTerminal)
                throw std::runtime_error("scenario '" + scenario.id + "' supply target has the wrong type");
            if (event.type == DisruptionType::DemandSurge && node->type != NodeType::DemandCentre)
                throw std::runtime_error("scenario '" + scenario.id + "' demand target has the wrong type");
            scenario.disruptions.push_back(event);
        }
        index_.put(scenario.id, static_cast<int>(scenarios_.size()));
        scenarios_.push_back(scenario);
    }
}

const Scenario* ScenarioLibrary::find(const std::string& id) const {
    const int* position = index_.get(id);
    return position == nullptr ? nullptr : &scenarios_.at(static_cast<std::size_t>(*position));
}

}  // namespace gridflow
