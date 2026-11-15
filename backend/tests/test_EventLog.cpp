#include <string>
#include <vector>

#include "doctest.h"
#include "sim/EventLog.h"

using namespace gridflow;

TEST_CASE("EventLog: empty and chronological append") {
    EventLog log;
    CHECK(log.empty());
    CHECK(log.size() == 0);
    CHECK(log.toVector().empty());

    log.append("DISRUPTION_APPLIED", "D1 outage", {"D1"});
    log.append("ALLOCATION_RUN", "Reallocated network", {"D1", "E2"});
    log.append("RESET", "Restored baseline", {});

    const std::vector<Event> events = log.toVector();
    REQUIRE(events.size() == 3);
    for (std::size_t index = 0; index < events.size(); ++index) {
        CHECK(events[index].seq == index + 1);
        CHECK(events[index].tick == index + 1);
    }
    CHECK(events[0].kind == "DISRUPTION_APPLIED");
    CHECK(events[0].message == "D1 outage");
    CHECK(events[0].refs == std::vector<std::string>{"D1"});
    CHECK(events[1].kind == "ALLOCATION_RUN");
    CHECK((events[1].refs == std::vector<std::string>{"D1", "E2"}));
    CHECK(events[2].kind == "RESET");
    CHECK(events[2].refs.empty());
}
