#include <string>
#include <vector>

#include "doctest.h"
#include "sim/ImpactAnalyzer.h"

using namespace gridflow;

namespace {

Facility makeNode(const std::string& id, NodeType type = NodeType::Processing,
                  int priority = 0, double shortage = 0.0) {
    Facility node;
    node.id = id;
    node.name = "Name " + id;
    node.type = type;
    node.priority = priority;
    node.effectiveDemand = 100.0;
    node.shortage = shortage;
    node.served = 100.0 - shortage;
    return node;
}

Route makeEdge(const std::string& id, int from, int to) {
    Route edge;
    edge.id = id;
    edge.from = from;
    edge.to = to;
    return edge;
}

}  // namespace

TEST_CASE("ImpactAnalyzer: reports downstream exposure, component delta, and sorted shortages") {
    Graph before;
    const int source = before.addFacility(makeNode("S", NodeType::Source));
    const int hub = before.addFacility(makeNode("H"));
    const int branch = before.addFacility(makeNode("B"));
    const int priorityThree = before.addFacility(makeNode("D3", NodeType::DemandCentre, 3));
    const int priorityOne = before.addFacility(makeNode("D1", NodeType::DemandCentre, 1));
    const int priorityTwo = before.addFacility(makeNode("D2", NodeType::DemandCentre, 2));
    before.addRoute(makeEdge("E1", source, hub));
    before.addRoute(makeEdge("E2", hub, branch));
    before.addRoute(makeEdge("E3", branch, priorityThree));
    before.addRoute(makeEdge("E4", branch, priorityOne));
    before.addRoute(makeEdge("E5", hub, priorityTwo));

    Graph after = before;
    after.node(hub).status = NodeStatus::Failed;
    after.node(priorityThree).shortage = 80.0;
    after.node(priorityOne).shortage = 50.0;
    after.node(priorityTwo).shortage = 100.0;

    const std::vector<Disruption> disruptions = {
        {DisruptionType::NodeOutage, "H", 1.0},
        {DisruptionType::EdgeDisruption, "E2", 0.5}
    };
    const ImpactReport report = analyzeImpact(before, after, disruptions);

    CHECK((report.exposedNodes ==
           std::vector<std::string>{"H", "B", "D2", "D3", "D1"}));
    CHECK(report.exposedEdges.size() == 4);
    CHECK(report.exposedEdges[0] == "E2");
    CHECK(report.componentCountBefore == 1);
    CHECK(report.componentCountAfter == 3);
    CHECK(report.components.count == 3);
    CHECK((report.components.isolatedDemandCentres ==
           std::vector<std::string>{"D3", "D1", "D2"}));
    CHECK(report.unmetDemandBefore == 0.0);
    CHECK(report.unmetDemandAfter == 230.0);
    REQUIRE(report.shortages.size() == 3);
    CHECK(report.shortages[0].nodeId == "D1");
    CHECK(report.shortages[1].nodeId == "D2");
    CHECK(report.shortages[2].nodeId == "D3");
}

TEST_CASE("ImpactAnalyzer: shortage ties are deterministic and disconnected graphs work") {
    Graph before;
    const int demandA = before.addFacility(makeNode("DA", NodeType::DemandCentre, 2));
    const int demandB = before.addFacility(makeNode("DB", NodeType::DemandCentre, 2));
    const int demandC = before.addFacility(makeNode("DC", NodeType::DemandCentre, 2));
    Graph after = before;
    after.node(demandA).shortage = 20.0;
    after.node(demandB).shortage = 30.0;
    after.node(demandC).shortage = 20.0;
    after.node(demandA).served = 80.0;
    after.node(demandB).served = 70.0;
    after.node(demandC).served = 80.0;

    const ImpactReport report = analyzeImpact(before, after, {});
    CHECK(report.componentCountBefore == 3);
    CHECK(report.componentCountAfter == 3);
    CHECK(report.unmetDemandBefore == 0.0);
    CHECK(report.unmetDemandAfter == 70.0);
    REQUIRE(report.shortages.size() == 3);
    CHECK(report.shortages[0].nodeId == "DB");
    CHECK(report.shortages[1].nodeId == "DA");
    CHECK(report.shortages[2].nodeId == "DC");
}
