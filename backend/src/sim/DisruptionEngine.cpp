#include "DisruptionEngine.h"

#include <cmath>
#include <string>

namespace gridflow {

InvalidDisruption::InvalidDisruption(const std::string& code, const std::string& message)
    : std::runtime_error(message), code_(code) {}

void applyDisruption(Graph& graph, const Disruption& disruption) {
    if (!std::isfinite(disruption.severity) || disruption.severity < 0.0 ||
        disruption.severity > 1.0) {
        throw InvalidDisruption("INVALID_DISRUPTION",
                                "Disruption severity must be between 0 and 1");
    }

    switch (disruption.type) {
        case DisruptionType::NodeOutage: {
            const int index = graph.findNode(disruption.targetId);
            if (index < 0) {
                throw InvalidDisruption("NODE_NOT_FOUND",
                                        "Node '" + disruption.targetId + "' does not exist");
            }
            Facility& node = graph.node(index);
            node.status = NodeStatus::Failed;
            node.severity = 1.0;
            node.surge = 0.0;
            return;
        }

        case DisruptionType::EdgeDisruption: {
            if (disruption.severity <= 0.0) {
                throw InvalidDisruption("INVALID_DISRUPTION",
                                        "Edge disruption severity must be greater than 0");
            }
            const int index = graph.findEdge(disruption.targetId);
            if (index < 0) {
                throw InvalidDisruption("EDGE_NOT_FOUND",
                                        "Edge '" + disruption.targetId + "' does not exist");
            }
            Route& edge = graph.edge(index);
            edge.severity = disruption.severity;
            edge.status = disruption.severity == 1.0 ? EdgeStatus::Failed : EdgeStatus::Reduced;
            return;
        }

        case DisruptionType::SupplyReduction: {
            if (disruption.severity <= 0.0) {
                throw InvalidDisruption("INVALID_DISRUPTION",
                                        "Supply reduction severity must be greater than 0");
            }
            const int index = graph.findNode(disruption.targetId);
            if (index < 0) {
                throw InvalidDisruption("NODE_NOT_FOUND",
                                        "Node '" + disruption.targetId + "' does not exist");
            }
            Facility& node = graph.node(index);
            if (node.type != NodeType::Source && node.type != NodeType::ImportTerminal) {
                throw InvalidDisruption("INVALID_DISRUPTION",
                                        "Supply reduction requires a source or import terminal");
            }
            node.severity = disruption.severity;
            node.surge = 0.0;
            node.status = disruption.severity == 1.0 ? NodeStatus::Failed : NodeStatus::Degraded;
            return;
        }

        case DisruptionType::DemandSurge: {
            if (disruption.severity <= 0.0) {
                throw InvalidDisruption("INVALID_DISRUPTION",
                                        "Demand surge severity must be greater than 0");
            }
            const int index = graph.findNode(disruption.targetId);
            if (index < 0) {
                throw InvalidDisruption("NODE_NOT_FOUND",
                                        "Node '" + disruption.targetId + "' does not exist");
            }
            Facility& node = graph.node(index);
            if (node.type != NodeType::DemandCentre) {
                throw InvalidDisruption("INVALID_DISRUPTION",
                                        "Demand surge requires a demand centre");
            }
            node.status = NodeStatus::Active;
            node.severity = 0.0;
            node.surge = disruption.severity;
            return;
        }

        default:
            throw InvalidDisruption("INVALID_DISRUPTION", "Unknown disruption type");
    }
}

}  // namespace gridflow
