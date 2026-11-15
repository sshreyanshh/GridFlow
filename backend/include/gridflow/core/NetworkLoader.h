#pragma once

#include <stdexcept>
#include <string>

#include "gridflow/core/Graph.h"

namespace gridflow {

// Parses and validates the external network document before building a Graph.
class NetworkLoadError : public std::runtime_error {
public:
    explicit NetworkLoadError(const std::string& message) : std::runtime_error(message) {}
};

Graph parseNetwork(const std::string& jsonText);
Graph loadNetworkFile(const std::string& path);

}  // namespace gridflow
