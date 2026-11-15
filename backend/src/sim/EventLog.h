#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../ds/LinkedList.h"

namespace gridflow {

struct Event {
    std::size_t seq = 0;
    std::size_t tick = 0;
    std::string kind;
    std::string message;
    std::vector<std::string> refs;
};

class EventLog {
public:
    // O(1)
    void append(const std::string& kind, const std::string& message,
                const std::vector<std::string>& refs);

    // O(n)
    std::vector<Event> toVector() const;

    // O(1)
    std::size_t size() const noexcept { return events_.size(); }

    // O(1)
    bool empty() const noexcept { return events_.empty(); }

private:
    LinkedList<Event> events_;
    std::size_t nextSequence_ = 1;
};

}  // namespace gridflow
