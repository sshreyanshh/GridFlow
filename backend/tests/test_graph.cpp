#include <stdexcept>
#include <string>
#include <vector>

#include "doctest.h"
#include "gridflow/core/Graph.h"

using namespace gridflow;

namespace {

Facility makeNode(const std::string& id, NodeType type = NodeType::Processing) {
    Facility f;
    f.id = id;
    f.name = "Node " + id;
    f.type = type;
    return f;
}

Route makeRoute(const std::string& id, int from, int to) {
    Route r;
    r.id = id;
    r.from = from;
    r.to = to;
    r.capacity = 100.0;
    return r;
}

// Small diamond-ish graph:  A -> B (E0),  A -> C (E1),  B -> C (E2)
struct Triangle {
    Graph g;
    int a, b, c;
    Triangle() {
        a = g.addFacility(makeNode("A"));
        b = g.addFacility(makeNode("B"));
        c = g.addFacility(makeNode("C"));
        g.addRoute(makeRoute("E0", a, b));
        g.addRoute(makeRoute("E1", a, c));
        g.addRoute(makeRoute("E2", b, c));
    }
};

}  // namespace

TEST_CASE("Graph: empty graph") {
    Graph g;
    CHECK(g.nodeCount() == 0);
    CHECK(g.edgeCount() == 0);
    CHECK(g.findNode("X") == -1);
    CHECK(g.findEdge("E1") == -1);
}

TEST_CASE("Graph: addFacility returns sequential indices and find works") {
    Graph g;
    CHECK(g.addFacility(makeNode("S1", NodeType::Source)) == 0);
    CHECK(g.addFacility(makeNode("H1", NodeType::DistributionHub)) == 1);
    CHECK(g.nodeCount() == 2);
    CHECK(g.findNode("S1") == 0);
    CHECK(g.findNode("H1") == 1);
    CHECK(g.findNode("nope") == -1);
    CHECK(g.node(1).type == NodeType::DistributionHub);
    CHECK(g.node(0).name == "Node S1");
}

TEST_CASE("Graph: rejects empty and duplicate facility ids, graph unchanged") {
    Graph g;
    g.addFacility(makeNode("A"));
    CHECK_THROWS_AS(g.addFacility(makeNode("A")), std::invalid_argument);
    CHECK_THROWS_AS(g.addFacility(makeNode("")), std::invalid_argument);
    CHECK(g.nodeCount() == 1);
}

TEST_CASE("Graph: addRoute fills out and in adjacency lists") {
    Triangle t;
    CHECK(t.g.edgeCount() == 3);

    const std::vector<int> outA = {0, 1};  // E0, E1 leave A
    const std::vector<int> outB = {2};     // E2 leaves B
    const std::vector<int> inB = {0};      // E0 enters B
    const std::vector<int> inC = {1, 2};   // E1, E2 enter C
    CHECK(t.g.outEdges(t.a) == outA);
    CHECK(t.g.outEdges(t.b) == outB);
    CHECK(t.g.outEdges(t.c).empty());
    CHECK(t.g.inEdges(t.a).empty());
    CHECK(t.g.inEdges(t.b) == inB);
    CHECK(t.g.inEdges(t.c) == inC);
}

TEST_CASE("Graph: neighbours are reached through the edge") {
    Triangle t;
    const int e = t.g.outEdges(t.a)[0];
    CHECK(t.g.edge(e).from == t.a);
    CHECK(t.g.edge(e).to == t.b);
    CHECK(t.g.findEdge("E2") == 2);
}

TEST_CASE("Graph: rejects invalid routes, graph unchanged") {
    Triangle t;
    CHECK_THROWS_AS(t.g.addRoute(makeRoute("E9", 0, 99)), std::invalid_argument);   // missing endpoint
    CHECK_THROWS_AS(t.g.addRoute(makeRoute("E9", -1, 1)), std::invalid_argument);   // "not set" endpoint
    CHECK_THROWS_AS(t.g.addRoute(makeRoute("E9", 1, 1)), std::invalid_argument);    // self loop
    CHECK_THROWS_AS(t.g.addRoute(makeRoute("E0", 1, 2)), std::invalid_argument);    // duplicate id
    CHECK_THROWS_AS(t.g.addRoute(makeRoute("", 0, 1)), std::invalid_argument);      // empty id
    CHECK(t.g.edgeCount() == 3);
    CHECK(t.g.outEdges(t.a).size() == 2);
    CHECK(t.g.inEdges(t.b).size() == 1);
}

TEST_CASE("Graph: bounds-checked access throws on bad index") {
    Triangle t;
    CHECK_THROWS_AS(t.g.node(3), std::out_of_range);
    CHECK_THROWS_AS(t.g.node(-1), std::out_of_range);
    CHECK_THROWS_AS(t.g.edge(3), std::out_of_range);
    CHECK_THROWS_AS(t.g.outEdges(7), std::out_of_range);
    CHECK_THROWS_AS(t.g.inEdges(-1), std::out_of_range);
}

TEST_CASE("Graph: copy is a fully independent deep copy") {
    Triangle t;
    Graph copy = t.g;

    // Same structure...
    CHECK(copy.nodeCount() == 3);
    CHECK(copy.edgeCount() == 3);
    CHECK(copy.outEdges(t.a) == t.g.outEdges(t.a));

    // ...but changes to the copy never touch the original.
    copy.node(t.b).status = NodeStatus::Failed;
    copy.edge(0).flow = 42.0;
    CHECK(t.g.node(t.b).status == NodeStatus::Active);
    CHECK(t.g.edge(0).flow == 0.0);

    // Adding to the copy does not change the original either.
    copy.addFacility(makeNode("D"));
    CHECK(copy.nodeCount() == 4);
    CHECK(t.g.nodeCount() == 3);
}

TEST_CASE("Graph: handles a larger chain") {
    Graph g;
    const int n = 500;
    for (int i = 0; i < n; ++i) g.addFacility(makeNode("N" + std::to_string(i)));
    for (int i = 0; i + 1 < n; ++i) g.addRoute(makeRoute("R" + std::to_string(i), i, i + 1));

    CHECK(g.nodeCount() == n);
    CHECK(g.edgeCount() == n - 1);
    CHECK(g.findNode("N499") == 499);
    CHECK(g.outEdges(0).size() == 1);
    CHECK(g.outEdges(n - 1).empty());
    CHECK(g.inEdges(0).empty());
    CHECK(g.inEdges(n - 1).size() == 1);
}