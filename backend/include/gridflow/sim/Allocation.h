#pragma once

#include <vector>

#include "gridflow/core/Graph.h"

namespace gridflow {

enum class Reason { NoPath, CapacityExhausted };

struct AllocationEntry {
    int demandNode = -1;
    std::vector<int> pathNodes;
    std::vector<int> pathEdges;
    double flow = 0.0;
    double totalCost = 0.0;
    double totalTime = 0.0;
};

struct Unresolved {
    int node = -1;
    double shortage = 0.0;
    Reason reason = Reason::NoPath;
};

struct AllocationResult {
    std::vector<AllocationEntry> allocations;
    std::vector<Unresolved> unresolved;
};

// Greedy successive-cheapest-path allocation runs in O(A(V+E)logV), where A
// is the number of path augmentations. It is deterministic but not guaranteed
// optimal because it never cancels earlier flow; min-cost max-flow is the stretch goal.
AllocationResult runAllocation(Graph& graph);

}  // namespace gridflow
