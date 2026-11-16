#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gridflow {

template <typename T, typename Compare = std::less<T>>
class MinHeap {
public:
    explicit MinHeap(Compare comesBefore = Compare()) : comesBefore_(comesBefore) {}

    void push(const T& value) {
        data_.push_back(value);
        siftUp(data_.size() - 1);
    }

    // Returns the element that would leave next. Throws std::out_of_range if empty.
    const T& top() const {
        if (data_.empty()) throw std::out_of_range("MinHeap::top on an empty heap");
        return data_.front();
    }

    // Removes the top element. Throws std::out_of_range if empty.
    void pop() {
        if (data_.empty()) throw std::out_of_range("MinHeap::pop on an empty heap");
        std::swap(data_.front(), data_.back());  // the last element takes the root...
        data_.pop_back();                        // ...and the old top is discarded
        if (!data_.empty()) siftDown(0);         // restore the heap property
    }

    bool empty() const { return data_.empty(); }
    std::size_t size() const { return data_.size(); }
    void clear() { data_.clear(); }

private:
    // Moves the element at `i` up while it should come out before its parent.
    void siftUp(std::size_t i) {
        while (i > 0) {
            const std::size_t parent = (i - 1) / 2;
            if (!comesBefore_(data_[i], data_[parent])) break;
            std::swap(data_[i], data_[parent]);
            i = parent;
        }
    }

    // Moves the element at `i` down while one of its children should come out before it.
    void siftDown(std::size_t i) {
        const std::size_t n = data_.size();
        while (true) {
            const std::size_t left = 2 * i + 1;
            const std::size_t right = left + 1;
            std::size_t best = i;
            if (left < n && comesBefore_(data_[left], data_[best])) best = left;
            if (right < n && comesBefore_(data_[right], data_[best])) best = right;
            if (best == i) break;  // heap property holds here
            std::swap(data_[i], data_[best]);
            i = best;
        }
    }

    std::vector<T> data_;
    Compare comesBefore_;
};

}  // namespace gridflow