#include <string>
 
#include "doctest.h"
#include "gridflow/model/Facility.h"
#include "gridflow/model/Route.h"
#include "gridflow/model/Types.h"

using namespace gridflow;

TEST_CASE("NodeType: every value survives toText -> parse") {
    const NodeType all[] = {NodeType::Source,          NodeType::ImportTerminal,
                            NodeType::Processing,      NodeType::DistributionHub,
                            NodeType::PowerPlant,      NodeType::DemandCentre};
    for (NodeType t : all) {
        NodeType parsed = NodeType::Source;
        CHECK(parseNodeType(toText(t), parsed));
        CHECK(parsed == t);
    }
}

TEST_CASE("NodeType: exact contract strings") {
    CHECK(std::string(toText(NodeType::Source)) == "SOURCE");
    CHECK(std::string(toText(NodeType::ImportTerminal)) == "IMPORT_TERMINAL");
    CHECK(std::string(toText(NodeType::Processing)) == "PROCESSING");
    CHECK(std::string(toText(NodeType::DistributionHub)) == "DISTRIBUTION_HUB");
    CHECK(std::string(toText(NodeType::PowerPlant)) == "POWER_PLANT");
    CHECK(std::string(toText(NodeType::DemandCentre)) == "DEMAND_CENTRE");
}
 
TEST_CASE("NodeType: rejects unknown text and leaves output untouched") {
    NodeType t = NodeType::PowerPlant;
    CHECK_FALSE(parseNodeType("", t));
    CHECK_FALSE(parseNodeType("source", t));   // case-sensitive on purpose
    CHECK_FALSE(parseNodeType("REFINERY", t));
    CHECK(t == NodeType::PowerPlant);
}

TEST_CASE("NodeStatus: every value survives toText -> parse") {
    const NodeStatus all[] = {NodeStatus::Active, NodeStatus::Degraded, NodeStatus::Failed};
    for (NodeStatus s : all) {
        NodeStatus parsed = NodeStatus::Active;
        CHECK(parseNodeStatus(toText(s), parsed));
        CHECK(parsed == s);
    }
}
 
TEST_CASE("NodeStatus: exact contract strings") {
    CHECK(std::string(toText(NodeStatus::Active)) == "ACTIVE");
    CHECK(std::string(toText(NodeStatus::Degraded)) == "DEGRADED");
    CHECK(std::string(toText(NodeStatus::Failed)) == "FAILED");
}
 
TEST_CASE("NodeStatus: rejects unknown text and leaves output untouched") {
    NodeStatus s = NodeStatus::Active;
    CHECK_FALSE(parseNodeStatus("failed", s));
    CHECK_FALSE(parseNodeStatus("REDUCED", s));  // that belongs to EdgeStatus
    CHECK(s == NodeStatus::Active);
}
 
TEST_CASE("EdgeStatus: every value survives toText -> parse") {
    const EdgeStatus all[] = {EdgeStatus::Active, EdgeStatus::Reduced, EdgeStatus::Failed};
    for (EdgeStatus s : all) {
        EdgeStatus parsed = EdgeStatus::Active;
        CHECK(parseEdgeStatus(toText(s), parsed));
        CHECK(parsed == s);
    }
}
 
TEST_CASE("EdgeStatus: exact contract strings") {
    CHECK(std::string(toText(EdgeStatus::Active)) == "ACTIVE");
    CHECK(std::string(toText(EdgeStatus::Reduced)) == "REDUCED");
    CHECK(std::string(toText(EdgeStatus::Failed)) == "FAILED");
}
 
TEST_CASE("EdgeStatus: rejects unknown text and leaves output untouched") {
    EdgeStatus s = EdgeStatus::Reduced;
    CHECK_FALSE(parseEdgeStatus("DEGRADED", s));  // that belongs to NodeStatus
    CHECK_FALSE(parseEdgeStatus("", s));
    CHECK(s == EdgeStatus::Reduced);
}
 
TEST_CASE("Facility: default-constructed values are safe") {
    Facility f;
    CHECK(f.id.empty());
    CHECK(f.name.empty());
    CHECK(f.type == NodeType::Processing);
    CHECK(f.capacity == 0.0);
    CHECK(f.supply == 0.0);
    CHECK(f.demand == 0.0);
    CHECK(f.priority == 0);
    CHECK(f.status == NodeStatus::Active);
    CHECK(f.severity == 0.0);
    CHECK(f.surge == 0.0);
    CHECK(f.effectiveSupply == 0.0);
    CHECK(f.effectiveDemand == 0.0);
    CHECK(f.served == 0.0);
    CHECK(f.shortage == 0.0);
    CHECK(f.throughput == 0.0);
}
 
TEST_CASE("Route: default-constructed values are safe") {
    Route r;
    CHECK(r.id.empty());
    CHECK(r.from == -1);   // -1 means "not set", so it can never be mistaken for node 0
    CHECK(r.to == -1);
    CHECK(r.capacity == 0.0);
    CHECK(r.cost == 0.0);
    CHECK(r.transitTime == 0.0);
    CHECK(r.status == EdgeStatus::Active);
    CHECK(r.severity == 0.0);
    CHECK(r.effectiveCapacity == 0.0);
    CHECK(r.flow == 0.0);
}
 
TEST_CASE("Facility and Route are copyable") {
    // The criticality ranking will copy a whole Graph, so copies must be independent.
    Facility a;
    a.id = "H2";
    a.status = NodeStatus::Active;
    Facility b = a;
    b.status = NodeStatus::Failed;
    CHECK(a.status == NodeStatus::Active);
    CHECK(b.status == NodeStatus::Failed);
    CHECK(b.id == "H2");
}
 
