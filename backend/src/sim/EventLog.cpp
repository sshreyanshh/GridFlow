#include "EventLog.h"

namespace gridflow {

void EventLog::append(const std::string& kind, const std::string& message,
                      const std::vector<std::string>& refs) {
    const std::size_t sequence = nextSequence_;
    events_.push_back(Event{sequence, sequence, kind, message, refs});
    ++nextSequence_;
}

std::vector<Event> EventLog::toVector() const {
    std::vector<Event> result;
    result.reserve(events_.size());
    for (const Event& event : events_) result.push_back(event);
    return result;
}

}  // namespace gridflow
