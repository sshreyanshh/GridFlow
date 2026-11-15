#include "gridflow/core/NetworkLoader.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include "json.hpp"

namespace gridflow {
namespace {

using Json = nlohmann::json;

Json required(const Json& object, const char* field, const std::string& entity) {
    if (!object.is_object() || !object.contains(field)) {
        throw NetworkLoadError(entity + ": missing field '" + field + "'");
    }
    return object.at(field);
}

std::string stringField(const Json& object, const char* field, const std::string& entity) {
    const Json value = required(object, field, entity);
    if (!value.is_string()) throw NetworkLoadError(entity + ": field '" + field + "' must be a string");
    return value.get<std::string>();
}

double numberField(const Json& object, const char* field, const std::string& entity) {
    const Json value = required(object, field, entity);
    if (!value.is_number()) throw NetworkLoadError(entity + ": field '" + field + "' must be numeric");
    const double result = value.get<double>();
    if (!std::isfinite(result)) throw NetworkLoadError(entity + ": field '" + field + "' must be finite");
    return result;
}

}  // namespace

Graph parseNetwork(const std::string& jsonText) {
    Json root;
    try {
        root = Json::parse(jsonText);
    } catch (const std::exception& error) {
        throw NetworkLoadError(std::string("network JSON: ") + error.what());
    }
    if (!root.is_object()) throw NetworkLoadError("network: root must be an object");
    const Json nodes = required(root, "nodes", "network");
    const Json edges = required(root, "edges", "network");
    if (!nodes.is_array()) throw NetworkLoadError("network: field 'nodes' must be an array");
    if (!edges.is_array()) throw NetworkLoadError("network: field 'edges' must be an array");

    Graph graph;
    for (const Json& item : nodes) {
        std::string id = stringField(item, "id", "node");
        const std::string entity = "node '" + id + "'";
        Facility facility;
        facility.id = id;
        facility.name = stringField(item, "name", entity);
        const std::string type = stringField(item, "type", entity);
        if (!parseNodeType(type, facility.type)) throw NetworkLoadError(entity + ": unknown type '" + type + "'");
        facility.capacity = numberField(item, "capacity", entity);
        facility.supply = numberField(item, "supply", entity);
        facility.demand = numberField(item, "demand", entity);
        const Json priority = required(item, "priority", entity);
        if (priority.is_number_integer()) facility.priority = priority.get<int>();
        else if (!priority.is_null()) throw NetworkLoadError(entity + ": field 'priority' must be an integer or null");
        const Json position = required(item, "position", entity);
        if (!position.is_object()) throw NetworkLoadError(entity + ": field 'position' must be an object");
        facility.x = numberField(position, "x", entity + " position");
        facility.y = numberField(position, "y", entity + " position");
        if (facility.capacity <= 0) throw NetworkLoadError(entity + ": field 'capacity' must be > 0");
        if (facility.supply < 0) throw NetworkLoadError(entity + ": field 'supply' must be >= 0");
        if (facility.demand < 0) throw NetworkLoadError(entity + ": field 'demand' must be >= 0");
        bool supplier = facility.type == NodeType::Source || facility.type == NodeType::ImportTerminal;
        bool demand = facility.type == NodeType::DemandCentre;
        if (!supplier && facility.supply != 0) throw NetworkLoadError(entity + ": field 'supply' must be zero for this type");
        if (demand && (facility.demand <= 0 || facility.priority < 1 || facility.priority > 3))
            throw NetworkLoadError(entity + ": demand centres require demand > 0 and priority in 1..3");
        if (!demand && facility.demand != 0) throw NetworkLoadError(entity + ": field 'demand' must be zero for this type");
        try { graph.addFacility(facility); }
        catch (const std::invalid_argument& error) { throw NetworkLoadError(entity + ": " + error.what()); }
    }
    for (const Json& item : edges) {
        std::string id = stringField(item, "id", "edge");
        const std::string entity = "edge '" + id + "'";
        std::string fromId = stringField(item, "from", entity);
        std::string toId = stringField(item, "to", entity);
        const int from = graph.findNode(fromId), to = graph.findNode(toId);
        if (from < 0) throw NetworkLoadError(entity + ": unknown from node '" + fromId + "'");
        if (to < 0) throw NetworkLoadError(entity + ": unknown to node '" + toId + "'");
        if (from == to) throw NetworkLoadError(entity + ": self-loop is not allowed");
        Route route;
        route.id = id; route.from = from; route.to = to;
        route.capacity = numberField(item, "capacity", entity);
        route.cost = numberField(item, "cost", entity);
        route.transitTime = numberField(item, "transitTime", entity);
        if (route.capacity <= 0) throw NetworkLoadError(entity + ": field 'capacity' must be > 0");
        if (route.cost < 0) throw NetworkLoadError(entity + ": field 'cost' must be >= 0");
        if (route.transitTime < 0) throw NetworkLoadError(entity + ": field 'transitTime' must be >= 0");
        try { graph.addRoute(route); }
        catch (const std::invalid_argument& error) { throw NetworkLoadError(entity + ": " + error.what()); }
    }
    return graph;
}

Graph loadNetworkFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw NetworkLoadError("cannot read network file '" + path + "'");
    std::ostringstream contents;
    contents << input.rdbuf();
    if (contents.str().empty()) throw NetworkLoadError("network file '" + path + "' is empty");
    return parseNetwork(contents.str());
}

}  // namespace gridflow
