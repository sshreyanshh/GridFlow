#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include "gridflow/api/Server.h"
#include "gridflow/core/NetworkLoader.h"
#include "gridflow/sim/Simulation.h"

namespace {

gridflow::Server* activeServer = nullptr;

void stopServer(int) {
    if (activeServer != nullptr) activeServer->stop();
}

}  // namespace

int main(int argc, char** argv) {
    std::string dataDirectory;
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--data-dir" && i + 1 < argc) dataDirectory = argv[++i];
        else if (argument == "--port" && i + 1 < argc) {
            try { port = std::stoi(argv[++i]); }
            catch (...) { std::cerr << "Invalid port\n"; return 2; }
        } else {
            std::cerr << "Usage: gridflow_server [--data-dir PATH] [--port N]\n";
            return 2;
        }
    }
    if (dataDirectory.empty()) {
        std::ifstream relative("../data/network.json");
        dataDirectory = relative ? "../data" : "data";
    }
    try {
        gridflow::Graph baseline = gridflow::loadNetworkFile(dataDirectory + "/network.json");
        gridflow::Simulation simulation(std::move(baseline), dataDirectory + "/scenarios.json");
        gridflow::Server server(simulation);
        activeServer = &server;
        std::signal(SIGINT, stopServer);
        std::signal(SIGTERM, stopServer);
        if (!server.listen("127.0.0.1", port)) {
            std::cerr << "Could not start GridFlow server on 127.0.0.1:" << port << '\n';
            activeServer = nullptr;
            return 1;
        }
        activeServer = nullptr;
    } catch (const std::exception& error) {
        std::cerr << "GridFlow startup failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
