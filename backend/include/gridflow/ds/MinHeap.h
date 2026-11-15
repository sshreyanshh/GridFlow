#pragma once
// MinHeap<T, Compare>: a binary heap (priority queue) stored in a std::vector.
//
// Convention: Compare(a, b) returns true when `a` must come out BEFORE `b`.
//   * Default Compare is std::less<T>, so the SMALLEST element is on top.
//   * Pass std::greater<T> to get a max-heap.
//   * For structs, write a comparator that orders by whatever matters first and
//     breaks ties explicitly (see "Determinism" below).
//
// The heap is a complete binary tree laid out in an array, level by level:
//
//         index:   0
//                 / \   <- the two children of index 0
//                1   2          parent(i) = (i - 1) / 2
//               / \ / \         left(i)   = 2i + 1
//              3  4 5  6        right(i)  = 2i + 2
//
// Heap property: no element comes out after one of its children, so the top (index 0)
// is always the next element to leave.
//
//   push:  append at the end, then SIFT UP  (swap with the parent while it beats it)
//   pop:   move the last element to the root, then SIFT DOWN (swap with the better child)
//   top:   data_[0]
//
// Complexity: push O(log n), pop O(log n), top O(1), size/empty O(1).
//
// There is no decrease-key. Dijkstra uses LAZY DELETION instead: push a new, better
// entry and skip stale ones when they are popped.
//
// Determinism: a heap is NOT stable, so entries that compare "equal" may leave in any
// order. Where the output must be reproducible (Dijkstra, shortage ordering), include a
// final tie-breaker such as the node index inside the comparator.
//
// Copying: the only member that owns memory is a std::vector, so the compiler-generated
// copy is a correct deep copy (rule of zero).

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