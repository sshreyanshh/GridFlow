#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

#include "doctest.h"
#include "gridflow/ds/MinHeap.h"

using namespace gridflow;

namespace {

// Tiny deterministic pseudo-random generator, so tests are reproducible.
struct Lcg {
    unsigned long long state;
    explicit Lcg(unsigned long long seed) : state(seed) {}
    int next(int modulo) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<int>((state >> 33) % static_cast<unsigned long long>(modulo));
    }
};

// Dijkstra-style entry: smaller distance first, ties broken by node index.
struct Entry {
    double dist;
    int node;
};
struct EntryOrder {
    bool operator()(const Entry& a, const Entry& b) const {
        if (a.dist != b.dist) return a.dist < b.dist;
        return a.node < b.node;
    }
};

// Shortage-style entry: priority 1 first, then BIGGER shortage first, then lower index.
struct Shortage {
    int priority;
    double amount;
    int node;
};
struct ShortageOrder {
    bool operator()(const Shortage& a, const Shortage& b) const {
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.amount != b.amount) return a.amount > b.amount;
        return a.node < b.node;
    }
};

}  // namespace

TEST_CASE("MinHeap: empty heap") {
    MinHeap<int> h;
    CHECK(h.empty());
    CHECK(h.size() == 0);
    CHECK_THROWS_AS(h.top(), std::out_of_range);
    CHECK_THROWS_AS(h.pop(), std::out_of_range);
}

TEST_CASE("MinHeap: single element") {
    MinHeap<int> h;
    h.push(42);
    CHECK(h.size() == 1);
    CHECK(h.top() == 42);
    h.pop();
    CHECK(h.empty());
}

TEST_CASE("MinHeap: elements leave in ascending order") {
    MinHeap<int> h;
    for (int v : {5, 3, 8, 1, 9, 2, 7}) h.push(v);
    CHECK(h.size() == 7);

    const std::vector<int> expected = {1, 2, 3, 5, 7, 8, 9};
    std::vector<int> out;
    while (!h.empty()) {
        out.push_back(h.top());
        h.pop();
    }
    CHECK(out == expected);
}

TEST_CASE("MinHeap: top changes when a smaller element arrives") {
    MinHeap<int> h;
    h.push(10);
    CHECK(h.top() == 10);
    h.push(4);
    CHECK(h.top() == 4);
    h.push(7);
    CHECK(h.top() == 4);
}

TEST_CASE("MinHeap: duplicates are all kept") {
    MinHeap<int> h;
    for (int v : {3, 1, 3, 1, 2, 3}) h.push(v);
    const std::vector<int> expected = {1, 1, 2, 3, 3, 3};
    std::vector<int> out;
    while (!h.empty()) {
        out.push_back(h.top());
        h.pop();
    }
    CHECK(out == expected);
}

TEST_CASE("MinHeap: std::greater gives a max-heap") {
    MinHeap<int, std::greater<int>> h;
    for (int v : {5, 3, 8, 1}) h.push(v);
    CHECK(h.top() == 8);
    h.pop();
    CHECK(h.top() == 5);
}

TEST_CASE("MinHeap: 10000 random values come out sorted") {
    Lcg rng(12345);
    MinHeap<int> h;
    std::vector<int> reference;
    for (int i = 0; i < 10000; ++i) {
        const int v = rng.next(1000);  // many duplicates on purpose
        h.push(v);
        reference.push_back(v);
    }
    std::sort(reference.begin(), reference.end());

    std::vector<int> out;
    while (!h.empty()) {
        out.push_back(h.top());
        h.pop();
    }
    CHECK(out == reference);
}

TEST_CASE("MinHeap: interleaved push and pop always returns the current minimum") {
    Lcg rng(777);
    MinHeap<int> h;
    std::vector<int> reference;  // slow but obviously correct
    for (int step = 0; step < 3000; ++step) {
        if (reference.empty() || rng.next(3) != 0) {  // push about two thirds of the time
            const int v = rng.next(500);
            h.push(v);
            reference.push_back(v);
        } else {
            auto smallest = std::min_element(reference.begin(), reference.end());
            REQUIRE(h.top() == *smallest);
            h.pop();
            reference.erase(smallest);
        }
        REQUIRE(h.size() == reference.size());
    }
}

TEST_CASE("MinHeap: ties are broken by the comparator, so the order is deterministic") {
    MinHeap<Entry, EntryOrder> h;
    h.push({2.0, 9});
    h.push({2.0, 3});
    h.push({1.0, 7});
    h.push({2.0, 5});

    CHECK(h.top().node == 7);
    h.pop();
    CHECK(h.top().node == 3);
    h.pop();
    CHECK(h.top().node == 5);
    h.pop();
    CHECK(h.top().node == 9);
}

TEST_CASE("MinHeap: shortage ordering is priority first, then bigger shortage") {
    MinHeap<Shortage, ShortageOrder> h;
    h.push({3, 500.0, 1});
    h.push({1, 50.0, 2});
    h.push({2, 300.0, 3});
    h.push({1, 120.0, 4});
    h.push({2, 300.0, 0});  // exact tie with node 3, so the lower index wins

    const std::vector<int> expectedNodes = {4, 2, 0, 3, 1};
    std::vector<int> out;
    while (!h.empty()) {
        out.push_back(h.top().node);
        h.pop();
    }
    CHECK(out == expectedNodes);
}

TEST_CASE("MinHeap: lazy deletion pattern (what Dijkstra will do)") {
    // Node 5 is first reached with distance 9, later with the better distance 4.
    // We cannot decrease a key, so we push a second entry and skip the stale one.
    MinHeap<Entry, EntryOrder> h;
    h.push({9.0, 5});
    h.push({4.0, 5});
    h.push({6.0, 2});

    double best[6] = {1e18, 1e18, 1e18, 1e18, 1e18, 1e18};
    best[5] = 4.0;
    best[2] = 6.0;

    std::vector<int> settled;
    while (!h.empty()) {
        const Entry e = h.top();
        h.pop();
        if (e.dist > best[e.node]) continue;  // stale entry, skip it
        settled.push_back(e.node);
    }
    const std::vector<int> expected = {5, 2};
    CHECK(settled == expected);
}

TEST_CASE("MinHeap: copy is independent") {
    MinHeap<int> a;
    for (int v : {4, 2, 6}) a.push(v);
    MinHeap<int> b = a;
    b.pop();
    b.push(100);
    CHECK(a.size() == 3);
    CHECK(a.top() == 2);
    CHECK(b.size() == 3);
    CHECK(b.top() == 4);
}

TEST_CASE("MinHeap: clear empties the heap and it stays usable") {
    MinHeap<int> h;
    for (int i = 0; i < 100; ++i) h.push(i);
    h.clear();
    CHECK(h.empty());
    h.push(5);
    CHECK(h.top() == 5);
}