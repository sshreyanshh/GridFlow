#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

#include "gridflow/algo/Dijkstra.h"
#include "gridflow/core/NetworkLoader.h"
#include "gridflow/sim/Simulation.h"

namespace {

using namespace gridflow;

void printSummary(const StateSnapshot& snapshot) {
    const Graph& graph = snapshot.graph;
    double supply = 0.0;
    double demand = 0.0;
    double served = 0.0;
    for (int i = 0; i < graph.nodeCount(); ++i) {
        const Facility& node = graph.node(i);
        supply += node.effectiveSupply;
        demand += node.effectiveDemand;
        served += node.served;
    }
    const double service = demand == 0.0 ? 100.0 : served / demand * 100.0;
    std::cout << "\n  STATE v" << snapshot.version << "  |  " << graph.nodeCount()
              << " nodes  |  " << graph.edgeCount() << " links\n"
              << "  Supply " << std::fixed << std::setprecision(0) << supply
              << " EU/day   Demand " << demand << " EU/day   Served " << served
              << " EU/day   Service " << std::setprecision(1) << service << "%\n";
}

void printNetwork(const Graph& graph) {
    std::cout << "\n  FACILITY NETWORK\n  " << std::left << std::setw(5) << "ID"
              << std::setw(34) << "FACILITY" << std::setw(22) << "TYPE"
              << std::right << std::setw(10) << "SUPPLY" << std::setw(10) << "DEMAND"
              << std::setw(10) << "SERVED" << "\n";
    for (int i = 0; i < graph.nodeCount(); ++i) {
        const Facility& n = graph.node(i);
        std::cout << "  " << std::left << std::setw(5) << n.id << std::setw(34) << n.name
                  << std::setw(22) << toText(n.type) << std::right << std::setw(10)
                  << std::fixed << std::setprecision(0) << n.effectiveSupply << std::setw(10)
                  << n.effectiveDemand << std::setw(10) << n.served << "\n";
    }
}

void printOutcome(const DisruptionOutcome& result) {
    printSummary(result.state);
    std::cout << "\n  IMPACT\n  Newly exposed facilities: "
              << result.impact.exposedNodes.size() << "  |  affected links: "
              << result.impact.exposedEdges.size() << "\n";
    if (result.impact.shortages.empty()) {
        std::cout << "  No demand centres have unmet demand.\n";
    } else {
        std::cout << "  Shortages:\n";
        for (const Shortage& shortage : result.impact.shortages)
            std::cout << "    " << shortage.name << " (" << shortage.nodeId << "): "
                      << std::fixed << std::setprecision(0) << shortage.shortage
                      << " EU/day unmet\n";
    }
    std::cout << "\n  RECOVERY\n  Suggested flow allocations: "
              << result.recovery.allocations.size() << "  |  unresolved centres: "
              << result.recovery.unresolved.size() << "\n";
    for (const PlannedRecommendation& recommendation : result.recovery.recommendations)
        std::cout << "    - " << recommendation.message << "\n";
}

std::string prompt(const std::string& label, const std::string& fallback) {
    std::cout << "  " << label << " [" << fallback << "]: ";
    std::string value;
    std::getline(std::cin, value);
    return value.empty() ? fallback : value;
}

}  // namespace

int main(int argc, char** argv) {
    std::string dataDirectory = "data";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) dataDirectory = argv[++i];
        else {
            std::cerr << "Usage: gridflow_terminal [--data-dir PATH]\n";
            return 2;
        }
    }

    try {
        Graph baseline = loadNetworkFile(dataDirectory + "/network.json");
        Simulation simulation(std::move(baseline), dataDirectory + "/scenarios.json");
        std::cout << "\n  GRIDFLOW  /  ENERGY NETWORK RESILIENCE\n"
                  << "  C++ graph algorithms · live terminal demonstration\n"
                  << "  ─────────────────────────────────────────────────────────\n";
        printSummary(simulation.stateSnapshot());

        bool running = true;
        while (running) {
            std::cout << "\n  DEMO MENU\n"
                      << "  1  Network overview\n"
                      << "  2  Run a disruption scenario\n"
                      << "  3  Find a route (Dijkstra)\n"
                      << "  4  Critical facilities\n"
                      << "  5  Reset to baseline\n"
                      << "  0  Exit\n"
                      << "  Choose: ";
            std::string choice;
            if (!std::getline(std::cin, choice)) break;

            if (choice == "1") {
                printNetwork(simulation.snapshot());
            } else if (choice == "2") {
                const auto scenarios = simulation.scenarios();
                std::cout << "\n  PRESET SCENARIOS\n";
                for (const Scenario& scenario : scenarios)
                    std::cout << "  " << scenario.id << "  " << scenario.name << "\n"
                              << "      " << scenario.description << "\n";
                const std::string id = prompt("Scenario ID", "SC02");
                try {
                    printOutcome(simulation.runScenario(id));
                } catch (const std::out_of_range& error) {
                    std::cout << "  " << error.what() << "\n";
                }
            } else if (choice == "3") {
                const std::string from = prompt("From facility", "S1");
                const std::string to = prompt("To facility", "D1");
                const SearchPath path = simulation.route(from, to, Metric::Cost);
                if (!path.found) {
                    std::cout << "  No operational route found.\n";
                } else {
                    const Graph graph = simulation.snapshot();
                    std::cout << "\n  LOWEST-COST ROUTE\n  ";
                    for (std::size_t i = 0; i < path.nodes.size(); ++i)
                        std::cout << (i ? " -> " : "") << graph.node(path.nodes[i]).id;
                    std::cout << "\n  Total cost " << path.totalCost << "  |  time "
                              << path.totalTime << " h  |  bottleneck "
                              << path.bottleneck << " EU/day\n";
                }
            } else if (choice == "4") {
                const auto critical = simulation.criticalNodes();
                std::cout << "\n  MOST CRITICAL FACILITIES (simulated removal)\n";
                const std::size_t count = std::min<std::size_t>(5, critical.ranking.size());
                for (std::size_t i = 0; i < count; ++i) {
                    const auto& entry = critical.ranking[i];
                    std::cout << "  " << i + 1 << ". " << entry.name << " ["
                              << entry.nodeId << "]  +" << std::fixed << std::setprecision(0)
                              << entry.unmetDemandIfRemoved << " EU/day unmet\n";
                }
                std::cout << "  Structural articulation points: ";
                for (const std::string& id : critical.articulationPoints)
                    std::cout << id << " ";
                std::cout << "\n";
            } else if (choice == "5") {
                printSummary(simulation.resetAndSnapshot());
                std::cout << "  Network restored to baseline.\n";
            } else if (choice == "0" || choice == "q" || choice == "Q") {
                running = false;
            } else {
                std::cout << "  Choose one of the listed menu options.\n";
            }
        }
        std::cout << "\n  Thanks for exploring GridFlow.\n\n";
    } catch (const std::exception& error) {
        std::cerr << "GridFlow terminal demo failed: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
