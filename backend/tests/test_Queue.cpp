#include <stdexcept>
#include <utility>

#include "doctest.h"
#include "../src/ds/Queue.h"

using namespace gridflow;

TEST_CASE("Queue: empty queue") {
    Queue<int> queue;
    CHECK(queue.empty());
    CHECK(queue.size() == 0);
    CHECK_THROWS_AS(queue.front(), std::out_of_range);
    CHECK_THROWS_AS(queue.pop(), std::out_of_range);
}

TEST_CASE("Queue: single value can be read and removed") {
    Queue<int> queue;
    queue.push(42);
    REQUIRE_FALSE(queue.empty());
    CHECK(queue.size() == 1);
    CHECK(queue.front() == 42);
    queue.pop();
    CHECK(queue.empty());
}

TEST_CASE("Queue: circular wrap-around and growth preserve FIFO order") {
    Queue<int> queue;
    for (int i = 0; i < 4; ++i) queue.push(i);
    queue.pop();
    queue.pop();
    queue.push(4);
    queue.push(5);
    queue.push(6);
    queue.push(7);
    CHECK(queue.size() == 6);

    for (int expected = 2; expected <= 7; ++expected) {
        REQUIRE_FALSE(queue.empty());
        CHECK(queue.front() == expected);
        queue.pop();
    }
    CHECK(queue.empty());
}

TEST_CASE("Queue: copy and move construction and assignment are independent and reusable") {
    Queue<int> original;
    original.push(1);
    original.push(2);
    original.push(3);

    Queue<int> copied(original);
    copied.pop();
    CHECK(original.front() == 1);
    CHECK(copied.front() == 2);

    Queue<int> copy_assigned;
    copy_assigned.push(99);
    copy_assigned = original;
    CHECK(copy_assigned.size() == 3);
    copy_assigned.pop();
    CHECK(original.front() == 1);

    Queue<int> moved(std::move(copied));
    CHECK(copied.empty());
    CHECK(moved.front() == 2);

    Queue<int> move_assigned;
    move_assigned.push(77);
    move_assigned = std::move(moved);
    CHECK(moved.empty());
    CHECK(move_assigned.size() == 2);
    CHECK(move_assigned.front() == 2);
    move_assigned.pop();
    CHECK(move_assigned.front() == 3);
}

TEST_CASE("Queue: clear destroys contents and queue remains usable") {
    Queue<int> queue;
    queue.push(1);
    queue.push(2);
    queue.clear();
    CHECK(queue.empty());
    CHECK(queue.size() == 0);
    queue.push(3);
    CHECK(queue.front() == 3);
}
