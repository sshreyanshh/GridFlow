#pragma once

#include <string>
#include <vector>

#include "gridflow/core/Graph.h"
#include "gridflow/ds/HashTable.h"
#include "gridflow/model/Disruption.h"

namespace gridflow {

struct Scenario {
    std::string id;
    std::string name;
    std::string description;
    std::vector<Disruption> disruptions;
};

class ScenarioLibrary {
public:
    ScenarioLibrary(const std::string& path, const Graph& graph);
    const std::vector<Scenario>& scenarios() const noexcept { return scenarios_; }
    const Scenario* find(const std::string& id) const;

private:
    std::vector<Scenario> scenarios_;
    HashTable<std::string, int> index_;
};

}  // namespace gridflow
