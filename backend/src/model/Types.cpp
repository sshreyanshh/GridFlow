#include "gridflow/model/Types.h"

namespace gridflow {

    const char* toText(NodeType t) {
        switch (t) {
            case NodeType::Source: return "SOURCE";
            case NodeType::ImportTerminal: return "IMPORT_TERMINAL";
            case NodeType::Processing: return "PROCESSING";
            case NodeType::DistributionHub: return "DISTRIBUTION_HUB";
            case NodeType::PowerPlant: return "POWER_PLANT";
            case NodeType::DemandCentre: return "DEMAND_CENTRE";
            default: return "UNKNOWN";
        }
    }

    const char* toText(NodeStatus s) {
        switch (s) {
            case NodeStatus::Active: return "ACTIVE";
            case NodeStatus::Degraded: return "DEGRADED";
            case NodeStatus::Failed: return "FAILED";
            default: return "UNKNOWN";
        }
    }

    const char* toText(EdgeStatus s) {
        switch (s) {
            case EdgeStatus::Active: return "ACTIVE";
            case EdgeStatus::Reduced: return "REDUCED";
            case EdgeStatus::Failed: return "FAILED";
            default: return "UNKNOWN";
        }
    }

    bool parseNodeType(const std::string& text, NodeType& out){
        if (text == "SOURCE") { out = NodeType::Source; return true; }
        if (text == "IMPORT_TERMINAL") { out = NodeType::ImportTerminal; return true; }
        if (text == "PROCESSING") { out = NodeType::Processing; return true; }
        if (text == "DISTRIBUTION_HUB") { out = NodeType::DistributionHub; return true; }
        if (text == "POWER_PLANT") { out = NodeType::PowerPlant; return true; }
        if (text == "DEMAND_CENTRE") { out = NodeType::DemandCentre; return true; }
        return false;
    }

    bool parseNodeStatus(const std::string& text, NodeStatus& out){
        if (text == "ACTIVE") { out = NodeStatus::Active; return true; }
        if (text == "DEGRADED") { out = NodeStatus::Degraded; return true; }
        if (text == "FAILED") { out = NodeStatus::Failed; return true; }
        return false;
    }

    bool parseEdgeStatus(const std::string& text, EdgeStatus& out){
        if (text == "ACTIVE") { out = EdgeStatus::Active; return true; }
        if (text == "REDUCED") { out = EdgeStatus::Reduced; return true; }
        if (text == "FAILED") { out = EdgeStatus::Failed; return true; }
        return false;
    }
} 