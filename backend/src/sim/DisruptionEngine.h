#pragma once

#include <stdexcept>
#include <string>

#include "gridflow/core/Graph.h"
#include "gridflow/model/Disruption.h"

namespace gridflow {

class InvalidDisruption : public std::runtime_error {
public:
    // O(1)
    InvalidDisruption(const std::string& code, const std::string& message);

    // O(1)
    const std::string& code() const noexcept { return code_; }

private:
    std::string code_;
};

// O(n) due to linear graph ID lookup; applies only disruption-owned fields.
void applyDisruption(Graph& graph, const Disruption& disruption);

}  // namespace gridflow
