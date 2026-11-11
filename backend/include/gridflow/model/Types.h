#pragma once
#include <string>

namespace gridflow{

    enum class NodeType{
        Source, ImportTerminal, Processing, DistributionHub, PowerPlant, DemandCentre
    };

    enum class NodeStatus {
        Active, Degraded, Failed
    };

    enum class EdgeStatus {
        Active, Reduced, Failed
    };

    const char* toText(NodeType t);
    const char* toText(NodeStatus s);
    const char* toText(EdgeStatus s);

    bool parseNodeType(const std::string& text, NodeType& out);
    bool parseNodeStatus(const std::string& text, NodeStatus& out);
    bool parseEdgeStatus(const std::string& text, EdgeStatus& out);
}