#include "doctest.h"

#include <fstream>
#include <sstream>
#include <string>

#include "gridflow/core/NetworkLoader.h"

namespace {
std::string replaceOnce(std::string text, const std::string& oldValue, const std::string& newValue) {
    const std::size_t location = text.find(oldValue);
    if (location != std::string::npos) text.replace(location, oldValue.size(), newValue);
    return text;
}
}

TEST_CASE("NetworkLoader: reference network loads") {
    gridflow::Graph graph = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    CHECK(graph.nodeCount() == 22);
    CHECK(graph.edgeCount() == 40);
    double supply = 0.0, demand = 0.0;
    for (int i = 0; i < graph.nodeCount(); ++i) {
        supply += graph.node(i).supply;
        demand += graph.node(i).demand;
    }
    CHECK(supply == doctest::Approx(2600.0));
    CHECK(demand == doctest::Approx(2000.0));
}

TEST_CASE("NetworkLoader: rejects malformed and unreadable files") {
    CHECK_THROWS_AS(gridflow::parseNetwork(""), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork("{"), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork("[]"), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::loadNetworkFile("/no/such/gridflow/network.json"), gridflow::NetworkLoadError);
}

TEST_CASE("NetworkLoader: validates node and edge fields") {
    const std::string base = R"({"nodes":[{"id":"S","name":"Source","type":"SOURCE","capacity":10,"supply":10,"demand":0,"priority":null,"position":{"x":0,"y":0}},{"id":"D","name":"Demand","type":"DEMAND_CENTRE","capacity":10,"supply":0,"demand":5,"priority":1,"position":{"x":1,"y":1}}],"edges":[{"id":"E","from":"S","to":"D","capacity":5,"cost":1,"transitTime":2}]})";
    CHECK_NOTHROW(gridflow::parseNetwork(base));
    CHECK_THROWS_AS(gridflow::parseNetwork(R"({"nodes":[{"id":"S"}],"edges":[]})"), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(R"({"nodes":[{"id":"S","name":"x","type":"REFINERY","capacity":1,"supply":0,"demand":0,"priority":null,"position":{"x":0,"y":0}}],"edges":[]})"), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(R"({"nodes":[{"id":"S","name":"x","type":"SOURCE","capacity":0,"supply":0,"demand":0,"priority":null,"position":{"x":0,"y":0}}],"edges":[]})"), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(R"({"nodes":[{"id":"D","name":"x","type":"DEMAND_CENTRE","capacity":1,"supply":0,"demand":1,"priority":0,"position":{"x":0,"y":0}}],"edges":[]})"), gridflow::NetworkLoadError);
    const std::string dangling = R"({"nodes":[{"id":"S","name":"x","type":"SOURCE","capacity":1,"supply":1,"demand":0,"priority":null,"position":{"x":0,"y":0}}],"edges":[{"id":"E","from":"S","to":"X","capacity":1,"cost":0,"transitTime":0}]})";
    CHECK_THROWS_AS(gridflow::parseNetwork(dangling), gridflow::NetworkLoadError);
}

TEST_CASE("NetworkLoader: rejects duplicate ids, loops, dangling nodes and edge ranges") {
    const std::string base = R"({"nodes":[{"id":"S","name":"Source","type":"SOURCE","capacity":10,"supply":10,"demand":0,"priority":null,"position":{"x":0,"y":0}},{"id":"D","name":"Demand","type":"DEMAND_CENTRE","capacity":10,"supply":0,"demand":5,"priority":1,"position":{"x":1,"y":1}}],"edges":[{"id":"E","from":"S","to":"D","capacity":5,"cost":1,"transitTime":2}]})";
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"id\":\"D\"", "\"id\":\"S\"")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"to\":\"D\"", "\"to\":\"X\"")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"to\":\"D\"", "\"to\":\"S\"")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"capacity\":5", "\"capacity\":0")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"cost\":1", "\"cost\":-1")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"transitTime\":2", "\"transitTime\":-1")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"supply\":10", "\"supply\":-1")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"supply\":0", "\"supply\":2")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"demand\":5", "\"demand\":0")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"priority\":1", "\"priority\":4")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"x\":1", "\"x\":\"bad\"")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"y\":1", "\"y\":null")), gridflow::NetworkLoadError);
    CHECK_THROWS_AS(gridflow::parseNetwork(replaceOnce(base, "\"name\":\"Source\"", "\"name\":7")), gridflow::NetworkLoadError);
}
