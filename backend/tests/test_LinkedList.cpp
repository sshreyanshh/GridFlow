#include <stdexcept>
#include <utility>
#include <vector>

#include "doctest.h"
#include "../src/ds/LinkedList.h"

using namespace gridflow;

TEST_CASE("LinkedList: empty list") {
    LinkedList<int> list;
    CHECK(list.empty());
    CHECK(list.size() == 0);
    CHECK(list.begin() == list.end());
    CHECK_THROWS_AS(list.front(), std::out_of_range);
}

TEST_CASE("LinkedList: single value and iteration") {
    LinkedList<int> list;
    list.push_back(7);
    REQUIRE_FALSE(list.empty());
    CHECK(list.size() == 1);
    CHECK(list.front() == 7);
    CHECK(*list.begin() == 7);
    CHECK(++list.begin() == list.end());

    const LinkedList<int>& const_list = list;
    CHECK(*const_list.cbegin() == 7);
    CHECK(const_list.cbegin() != const_list.cend());
}

TEST_CASE("LinkedList: push_back preserves order and supports range iteration") {
    LinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);

    std::vector<int> values;
    for (int value : list) values.push_back(value);
    CHECK((values == std::vector<int>{1, 2, 3}));
}

TEST_CASE("LinkedList: copy and move construction and assignment are independent") {
    LinkedList<int> original;
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);

    LinkedList<int> copied(original);
    *copied.begin() = 10;
    CHECK(original.front() == 1);
    CHECK(copied.front() == 10);

    LinkedList<int> copy_assigned;
    copy_assigned.push_back(99);
    copy_assigned = original;
    CHECK(copy_assigned.size() == 3);
    copy_assigned.front() = 11;
    CHECK(original.front() == 1);

    LinkedList<int> moved(std::move(copied));
    CHECK(copied.empty());
    CHECK(moved.front() == 10);

    LinkedList<int> move_assigned;
    move_assigned.push_back(77);
    move_assigned = std::move(moved);
    CHECK(moved.empty());
    CHECK(move_assigned.size() == 3);
    CHECK(move_assigned.front() == 10);
}

TEST_CASE("LinkedList: clear releases nodes and list remains usable") {
    LinkedList<int> list;
    for (int i = 0; i < 20; ++i) list.push_back(i);
    list.clear();
    CHECK(list.empty());
    CHECK(list.size() == 0);
    CHECK(list.begin() == list.end());

    list.push_back(42);
    CHECK(list.front() == 42);
    CHECK(list.size() == 1);
}
