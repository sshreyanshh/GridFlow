#pragma once

#include <vector>

#include "gridflow/core/Graph.h"

namespace gridflow {

// O(V + E) time and O(V) auxiliary space over the non-failed graph projection.
std::vector<int> articulationPoints(const Graph& graph);

}  // namespace gridflow
