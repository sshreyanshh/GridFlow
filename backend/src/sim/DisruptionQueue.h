#pragma once

#include <cstddef>
#include <utility>

#include "gridflow/model/Disruption.h"
#include "../ds/Queue.h"

namespace gridflow {

class DisruptionQueue {
public:
    // O(1) amortized; O(n) when the underlying queue grows.
    void enqueue(const Disruption& disruption) { queue_.push(disruption); }

    // O(1) amortized; O(n) when the underlying queue grows.
    void enqueue(Disruption&& disruption) { queue_.push(std::move(disruption)); }

    // O(1)
    Disruption dequeue() {
        Disruption disruption = queue_.front();
        queue_.pop();
        return disruption;
    }

    // O(1)
    Disruption& front() { return queue_.front(); }

    // O(1)
    const Disruption& front() const { return queue_.front(); }

    // O(1)
    bool empty() const noexcept { return queue_.empty(); }

    // O(1)
    std::size_t size() const noexcept { return queue_.size(); }

private:
    Queue<Disruption> queue_;
};

}  // namespace gridflow
