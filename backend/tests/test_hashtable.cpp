#include <cstddef>
#include <string>

#include "doctest.h"
#include "gridflow/ds/HashTable.h"

using namespace gridflow;

namespace {

using StringTable = HashTable<std::string, int>;

// A deliberately terrible hash: every key collides into the same bucket.
// It lets us test chain handling (head / middle / tail removal) deterministically.
struct ConstHash {
    std::size_t operator()(const int&) const { return 7; }
};

}  // namespace

TEST_CASE("HashTable: empty table") {
    StringTable t;
    CHECK(t.size() == 0);
    CHECK(t.empty());
    CHECK(t.get("x") == nullptr);
    CHECK_FALSE(t.contains("x"));
    CHECK_FALSE(t.remove("x"));
    CHECK(t.bucketCount() == 16);
}

TEST_CASE("HashTable: put and get") {
    StringTable t;
    CHECK(t.put("S1", 10));
    CHECK(t.put("H2", 20));
    CHECK(t.size() == 2);
    REQUIRE(t.get("S1") != nullptr);
    CHECK(*t.get("S1") == 10);
    CHECK(*t.get("H2") == 20);
    CHECK(t.contains("S1"));
    CHECK(t.get("D9") == nullptr);
}

TEST_CASE("HashTable: put overwrites an existing key") {
    StringTable t;
    CHECK(t.put("A", 1));
    CHECK_FALSE(t.put("A", 2));  // false = overwritten, not inserted
    CHECK(t.size() == 1);
    CHECK(*t.get("A") == 2);
}

TEST_CASE("HashTable: get lets the caller modify the value in place") {
    StringTable t;
    t.put("A", 1);
    *t.get("A") += 5;
    CHECK(*t.get("A") == 6);
}

TEST_CASE("HashTable: remove") {
    StringTable t;
    t.put("A", 1);
    t.put("B", 2);
    CHECK(t.remove("A"));
    CHECK(t.size() == 1);
    CHECK_FALSE(t.contains("A"));
    CHECK(t.contains("B"));
    CHECK_FALSE(t.remove("A"));  // already gone
    CHECK(t.remove("B"));
    CHECK(t.empty());
}

TEST_CASE("HashTable: empty string is a valid key") {
    StringTable t;
    t.put("", 5);
    REQUIRE(t.get("") != nullptr);
    CHECK(*t.get("") == 5);
}

TEST_CASE("HashTable: grows and rehashes, keeping every entry") {
    StringTable t;
    CHECK(t.bucketCount() == 16);
    const int n = 1000;
    for (int i = 0; i < n; ++i) CHECK(t.put("key" + std::to_string(i), i));

    CHECK(t.size() == static_cast<std::size_t>(n));
    CHECK(t.bucketCount() > 16);
    CHECK(t.size() * 4 <= t.bucketCount() * 3);  // load factor never above 0.75
    for (int i = 0; i < n; ++i) {
        const int* v = t.get("key" + std::to_string(i));
        REQUIRE(v != nullptr);
        CHECK(*v == i);
    }
    CHECK(t.maxChainLength() < 12);  // the hash spreads keys reasonably well
}

TEST_CASE("HashTable: pointers from get() survive a rehash") {
    StringTable t;
    t.put("anchor", 7);
    int* before = t.get("anchor");
    for (int i = 0; i < 500; ++i) t.put("filler" + std::to_string(i), i);  // forces several rehashes
    CHECK(t.get("anchor") == before);
    CHECK(*before == 7);
}

TEST_CASE("HashTable: collisions share a chain; head, middle and tail removal work") {
    HashTable<int, int, ConstHash> t;
    for (int k = 1; k <= 5; ++k) t.put(k, k * 10);  // all in one bucket
    CHECK(t.size() == 5);
    CHECK(t.maxChainLength() == 5);
    for (int k = 1; k <= 5; ++k) {
        REQUIRE(t.get(k) != nullptr);
        CHECK(*t.get(k) == k * 10);
    }

    CHECK(t.remove(5));  // inserted last, so it is the head of the chain
    CHECK(t.remove(1));  // inserted first, so it is the tail
    CHECK(t.remove(3));  // somewhere in the middle
    CHECK(t.size() == 2);
    CHECK_FALSE(t.contains(5));
    CHECK_FALSE(t.contains(1));
    CHECK_FALSE(t.contains(3));
    CHECK(*t.get(2) == 20);
    CHECK(*t.get(4) == 40);
}

TEST_CASE("HashTable: clear empties the table and it stays usable") {
    StringTable t;
    for (int i = 0; i < 100; ++i) t.put("k" + std::to_string(i), i);
    t.clear();
    CHECK(t.size() == 0);
    CHECK(t.get("k5") == nullptr);
    CHECK(t.put("again", 1));
    CHECK(*t.get("again") == 1);
}

TEST_CASE("HashTable: forEach visits every entry exactly once") {
    StringTable t;
    for (int i = 1; i <= 50; ++i) t.put("k" + std::to_string(i), i);
    int count = 0;
    int sum = 0;
    t.forEach([&](const std::string&, const int& value) {
        ++count;
        sum += value;
    });
    CHECK(count == 50);
    CHECK(sum == 50 * 51 / 2);
}

TEST_CASE("HashTable: copy constructor makes an independent deep copy") {
    StringTable a;
    a.put("x", 1);
    a.put("y", 2);
    StringTable b = a;

    b.put("x", 99);
    b.put("z", 3);
    b.remove("y");

    CHECK(a.size() == 2);
    CHECK(*a.get("x") == 1);
    CHECK(a.contains("y"));
    CHECK_FALSE(a.contains("z"));

    CHECK(b.size() == 2);
    CHECK(*b.get("x") == 99);
    CHECK(b.contains("z"));
    CHECK_FALSE(b.contains("y"));
}

TEST_CASE("HashTable: copy assignment is independent and self-assignment is safe") {
    StringTable a;
    a.put("x", 1);
    StringTable c;
    c.put("old", 5);
    c = a;
    CHECK_FALSE(c.contains("old"));  // old contents were released
    CHECK(*c.get("x") == 1);
    c.put("q", 5);
    CHECK_FALSE(a.contains("q"));

    StringTable& alias = a;
    a = alias;  // self-assignment through an alias
    CHECK(a.size() == 1);
    CHECK(*a.get("x") == 1);
}

TEST_CASE("HashTable: works with other value types") {
    HashTable<std::string, std::string> t;
    t.put("H2", "Central Hub");
    REQUIRE(t.get("H2") != nullptr);
    CHECK(*t.get("H2") == "Central Hub");
}

TEST_CASE("DefaultHash<string>: FNV-1a 64-bit reference values") {
    DefaultHash<std::string> h;
    CHECK(h("") == static_cast<std::size_t>(0xcbf29ce484222325ULL));
    CHECK(h("a") == static_cast<std::size_t>(0xaf63dc4c8601ec8cULL));
    CHECK(h("foobar") == static_cast<std::size_t>(0x85944171f73967e8ULL));
}