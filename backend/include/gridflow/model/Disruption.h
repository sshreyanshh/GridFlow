#pragma once

#include <string>

namespace gridflow {

enum class DisruptionType {
    NodeOutage,
    EdgeDisruption,
    SupplyReduction,
    DemandSurge
};

struct Disruption {
    DisruptionType type = DisruptionType::NodeOutage;
    std::string targetId;
    double severity = 1.0;
};

}  // namespace gridflow
