#include "doctest.h"
#include "json.hpp"

TEST_CASE("toolchain works") {
    nlohmann::json j = {{"a", 1}};
    CHECK(j["a"] == 1);
}