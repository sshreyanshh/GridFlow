#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "doctest.h"
#include "api/Serializer.h"

using namespace gridflow;
using Json = nlohmann::json;

namespace {

Facility makeNode(const std::string& id, const std::string& name, NodeType type) {
    Facility node;
    node.id = id;
    node.name = name;
    node.type = type;
    node.capacity = 100.0;
    node.supply = type == NodeType::Source ? 80.0 : 0.0;
    node.demand = type == NodeType::DemandCentre ? 50.0 : 0.0;
    node.priority = type == NodeType::DemandCentre ? 1 : 0;
    node.x = 10.0;
    node.y = 20.0;
    node.effectiveSupply = node.supply;
    node.effectiveDemand = node.demand;
    return node;
}

Route makeEdge(const std::string& id, int from, int to) {
    Route edge;
    edge.id = id;
    edge.from = from;
    edge.to = to;
    edge.capacity = 70.0;
    edge.cost = 2.0;
    edge.transitTime = 3.0;
    edge.effectiveCapacity = edge.capacity;
    return edge;
}

Json readExample(const std::string& fileName) {
    const std::string path =
        std::string(GRIDFLOW_REPO_ROOT) + "/contracts/examples/" + fileName;
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Unable to read contract fixture: " + path);
    Json value;
    input >> value;
    return value;
}

Graph graphFromExamples(const Json& network, const Json& state) {
    Graph graph;
    for (const Json& item : network.at("nodes")) {
        Facility node;
        node.id = item.at("id").get<std::string>();
        node.name = item.at("name").get<std::string>();
        if (!parseNodeType(item.at("type").get<std::string>(), node.type)) {
            throw std::runtime_error("Invalid node type in network fixture");
        }
        node.capacity = item.at("capacity").get<double>();
        node.supply = item.at("supply").get<double>();
        node.demand = item.at("demand").get<double>();
        if (!item.at("priority").is_null()) {
            node.priority = item.at("priority").get<int>();
        }
        node.x = item.at("position").at("x").get<double>();
        node.y = item.at("position").at("y").get<double>();
        graph.addFacility(node);
    }

    for (const Json& item : network.at("edges")) {
        Route edge;
        edge.id = item.at("id").get<std::string>();
        edge.from = graph.findNode(item.at("from").get<std::string>());
        edge.to = graph.findNode(item.at("to").get<std::string>());
        edge.capacity = item.at("capacity").get<double>();
        edge.cost = item.at("cost").get<double>();
        edge.transitTime = item.at("transitTime").get<double>();
        graph.addRoute(edge);
    }

    for (const Json& item : state.at("nodes")) {
        const int index = graph.findNode(item.at("id").get<std::string>());
        if (index < 0) throw std::runtime_error("Unknown node in state fixture");
        Facility& node = graph.node(index);
        if (!parseNodeStatus(item.at("status").get<std::string>(), node.status)) {
            throw std::runtime_error("Invalid node status in state fixture");
        }
        node.effectiveSupply = item.at("effectiveSupply").get<double>();
        node.effectiveDemand = item.at("effectiveDemand").get<double>();
        node.served = item.at("served").get<double>();
        node.shortage = item.at("shortage").get<double>();
        node.throughput = item.at("throughput").get<double>();
    }

    for (const Json& item : state.at("edges")) {
        const int index = graph.findEdge(item.at("id").get<std::string>());
        if (index < 0) throw std::runtime_error("Unknown edge in state fixture");
        Route& edge = graph.edge(index);
        if (!parseEdgeStatus(item.at("status").get<std::string>(), edge.status)) {
            throw std::runtime_error("Invalid edge status in state fixture");
        }
        edge.effectiveCapacity = item.at("effectiveCapacity").get<double>();
        edge.flow = item.at("flow").get<double>();
    }
    return graph;
}

ImpactReport impactFromExample(const Json& value) {
    ImpactReport impact;
    impact.exposedNodes = value.at("exposedNodes").get<std::vector<std::string>>();
    impact.exposedEdges = value.at("exposedEdges").get<std::vector<std::string>>();
    impact.components.count = value.at("components").at("count").get<int>();
    impact.components.isolatedDemandCentres =
        value.at("components").at("isolatedDemandCentres").get<std::vector<std::string>>();
    impact.unmetDemandBefore = value.at("unmetDemandBefore").get<double>();
    impact.unmetDemandAfter = value.at("unmetDemandAfter").get<double>();
    for (const Json& item : value.at("shortages")) {
        impact.shortages.push_back({
            item.at("nodeId").get<std::string>(),
            item.at("name").get<std::string>(),
            item.at("priority").get<int>(),
            item.at("demand").get<double>(),
            item.at("served").get<double>(),
            item.at("shortage").get<double>()
        });
    }
    return impact;
}

RecoveryPlan recoveryFromExample(const Json& value) {
    RecoveryPlan recovery;
    for (const Json& item : value.at("allocations")) {
        recovery.allocations.push_back({
            item.at("demandCentreId").get<std::string>(),
            item.at("path").get<std::vector<std::string>>(),
            item.at("edgeIds").get<std::vector<std::string>>(),
            item.at("flow").get<double>(),
            item.at("totalCost").get<double>(),
            item.at("totalTime").get<double>()
        });
    }
    for (const Json& item : value.at("unresolved")) {
        recovery.unresolved.push_back({
            item.at("nodeId").get<std::string>(),
            item.at("shortage").get<double>(),
            item.at("reason").get<std::string>()
        });
    }
    for (const Json& item : value.at("recommendations")) {
        recovery.recommendations.push_back({
            item.at("type").get<std::string>(),
            item.at("targetId").get<std::string>(),
            item.at("priority").get<int>(),
            item.at("message").get<std::string>()
        });
    }
    return recovery;
}

}  // namespace

TEST_CASE("Serializer: empty graph has the contract's top-level fields") {
    const Graph graph;
    const Json network = toJson(graph);
    CHECK(network.size() == 2);
    CHECK(network.at("nodes").is_array());
    CHECK(network.at("edges").is_array());

    const Json state = stateToJson(graph);
    CHECK(state.at("version") == 0);
    CHECK(state.at("nodes").is_array());
    CHECK(state.at("edges").is_array());
    CHECK(state.at("summary").at("totalSupply") == 0.0);
    CHECK(state.at("summary").at("totalDemand") == 0.0);
}

TEST_CASE("Serializer: hand-built graph uses camelCase contract fields") {
    Graph graph;
    const int source = graph.addFacility(makeNode("S1", "Source", NodeType::Source));
    const int demand = graph.addFacility(makeNode("D1", "Demand", NodeType::DemandCentre));
    graph.node(demand).served = 40.0;
    graph.node(demand).shortage = 10.0;
    graph.addRoute(makeEdge("E1", source, demand));
    graph.edge(0).flow = 40.0;

    const Json network = toJson(graph);
    REQUIRE(network.at("nodes").size() == 2);
    CHECK(network.at("nodes")[0].at("type") == "SOURCE");
    CHECK(network.at("nodes")[0].at("priority").is_null());
    CHECK(network.at("nodes")[0].at("position").at("x") == 10.0);
    CHECK(network.at("nodes")[1].at("priority") == 1);
    CHECK(network.at("edges")[0].at("from") == "S1");
    CHECK(network.at("edges")[0].at("to") == "D1");
    CHECK(network.at("edges")[0].contains("transitTime"));

    const Json state = stateToJson(graph);
    CHECK(state.at("nodes")[0].at("status") == "ACTIVE");
    CHECK(state.at("nodes")[1].at("effectiveDemand") == 50.0);
    CHECK(state.at("edges")[0].at("effectiveCapacity") == 70.0);
    CHECK(state.at("edges")[0].at("flow") == 40.0);
    CHECK(state.at("summary").at("totalSupply") == 80.0);
    CHECK(state.at("summary").at("totalDemand") == 50.0);
    CHECK(state.at("summary").at("totalServed") == 40.0);
    CHECK(state.at("summary").at("unmetDemand") == 10.0);
    CHECK(state.at("summary").at("servedPercent") == 80.0);
    CHECK(state.at("summary").at("baselineServedPercent") == 100.0);
}

TEST_CASE("Serializer: output matches the network and baseline state fixtures") {
    const Json networkFixture = readExample("network.json");
    const Json stateFixture = readExample("state.baseline.json");
    const Graph graph = graphFromExamples(networkFixture, stateFixture);

    CHECK(toJson(graph) == networkFixture);
    CHECK(stateToJson(graph) == stateFixture);
}

TEST_CASE("Serializer: impact and recovery match the disruption response schemas") {
    const Json response = readExample("disruption-response.example.json");
    const ImpactReport impact = impactFromExample(response.at("impact"));
    const RecoveryPlan recovery = recoveryFromExample(response.at("recovery"));

    CHECK(impactToJson(impact) == response.at("impact"));
    CHECK(recoveryToJson(recovery) == response.at("recovery"));
}

TEST_CASE("Serializer: path, event, and error use exact contract keys") {
    const PathResult path{"cost", true, {"S1", "D1"}, {"E01"}, 2.0, 4.0, 70.0};
    Json expectedPath = {
        {"metric", "cost"},
        {"found", true},
        {"path", {"S1", "D1"}},
        {"edgeIds", {"E01"}},
        {"totalCost", 2.0},
        {"totalTime", 4.0},
        {"bottleneckCapacity", 70.0}
    };
    CHECK(pathToJson(path) == expectedPath);

    const std::vector<Event> events = {
        Event{1, 1, "DISRUPTION_APPLIED", "H2 outage", {"H2"}}
    };
    const Json expectedEvent = {
        {"seq", 1}, {"tick", 1}, {"kind", "DISRUPTION_APPLIED"},
        {"message", "H2 outage"}, {"refs", {"H2"}}
    };
    const Json expectedEvents = {{"events", Json::array({expectedEvent})}};
    CHECK(eventsToJson(events) == expectedEvents);

    const Json errorFixture = readExample("error.example.json");
    CHECK(errorToJson(
              errorFixture.at("error").at("code").get<std::string>(),
              errorFixture.at("error").at("message").get<std::string>(),
              errorFixture.at("error").at("details")) == errorFixture);
}
