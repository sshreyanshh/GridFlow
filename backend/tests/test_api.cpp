#include "doctest.h"

#include <thread>

#include "gridflow/api/Server.h"
#include "gridflow/core/NetworkLoader.h"
#include "json.hpp"

namespace {

class RunningServer {
public:
    explicit RunningServer(gridflow::Simulation& simulation) : server_(simulation) {
        port_ = server_.bindToAnyPort("127.0.0.1");
        if (port_ > 0) thread_ = std::thread([this] { server_.listenAfterBind(); });
    }
    ~RunningServer() { server_.stop(); if (thread_.joinable()) thread_.join(); }
    int port() const { return port_; }
private:
    gridflow::Server server_;
    int port_ = -1;
    std::thread thread_;
};

}  // namespace

TEST_CASE("API: endpoints return contract envelopes and mutations reset") {
    auto graph = gridflow::loadNetworkFile(std::string(GRIDFLOW_DATA_DIR) + "/network.json");
    gridflow::Simulation simulation(std::move(graph), std::string(GRIDFLOW_DATA_DIR) + "/scenarios.json");
    RunningServer server(simulation);
    REQUIRE(server.port() > 0);
    httplib::Client client("127.0.0.1", server.port());
    auto health = client.Get("/api/health");
    REQUIRE(health); CHECK(health->status == 200);
    auto healthJson = nlohmann::json::parse(health->body);
    CHECK(healthJson["ok"] == true);
    CHECK(healthJson["data"]["service"] == "gridflow");
    auto network = client.Get("/api/network"); REQUIRE(network);
    auto networkJson = nlohmann::json::parse(network->body);
    CHECK(networkJson["data"]["nodes"].size() == 22);
    auto state = client.Get("/api/state"); REQUIRE(state);
    CHECK(nlohmann::json::parse(state->body)["data"]["version"] == 0);
    auto scenarios = client.Get("/api/scenarios"); REQUIRE(scenarios);
    CHECK(nlohmann::json::parse(scenarios->body)["data"]["scenarios"].size() == 5);
    auto route = client.Get("/api/routes?from=S1&to=D1&metric=cost"); REQUIRE(route);
    CHECK(nlohmann::json::parse(route->body)["data"]["found"] == true);
    auto badJson = client.Post("/api/disruptions", "{", "application/json"); REQUIRE(badJson);
    CHECK(badJson->status == 400);
    CHECK(nlohmann::json::parse(badJson->body)["error"]["code"] == "INVALID_JSON");
    auto outage = client.Post("/api/disruptions",
        R"({"type":"NODE_OUTAGE","targetId":"H2","severity":1})", "application/json");
    REQUIRE(outage); CHECK(outage->status == 200);
    auto reset = client.Post("/api/reset", "", "application/json"); REQUIRE(reset);
    CHECK(nlohmann::json::parse(reset->body)["data"]["state"]["version"] == 2);
    auto events = client.Get("/api/events"); REQUIRE(events);
    CHECK(nlohmann::json::parse(events->body)["data"]["events"].size() >= 3);
    auto missing = client.Get("/api/no-such-route"); REQUIRE(missing);
    CHECK(missing->status == 404);
}

