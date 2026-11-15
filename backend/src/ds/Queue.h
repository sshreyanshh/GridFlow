#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace gridflow {

template <typename T>
class Queue {
public:
    using size_type = std::size_t;

    // O(1)
    Queue() noexcept = default;

    // O(n)
    Queue(const Queue& other) : capacity_(other.capacity_) {
        if (capacity_ == 0) return;

        data_ = Traits::allocate(allocator_, capacity_);
        try {
            for (; size_ < other.size_; ++size_) {
                Traits::construct(allocator_, data_ + size_,
                                  other.data_[(other.head_ + size_) % other.capacity_]);
            }
        } catch (...) {
            destroy_elements();
            Traits::deallocate(allocator_, data_, capacity_);
            data_ = nullptr;
            capacity_ = 0;
            throw;
        }
    }

    // O(1)
    Queue(Queue&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          capacity_(std::exchange(other.capacity_, 0)),
          head_(std::exchange(other.head_, 0)),
          size_(std::exchange(other.size_, 0)) {}

    // O(n)
    Queue& operator=(const Queue& other) {
        if (this != &other) {
            Queue copy(other);
            swap(copy);
        }
        return *this;
    }

    // O(n) in this queue's current size; O(1) to take ownership.
    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            release();
            data_ = std::exchange(other.data_, nullptr);
            capacity_ = std::exchange(other.capacity_, 0);
            head_ = std::exchange(other.head_, 0);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    // O(n)
    ~Queue() { release(); }

    // O(1) amortized; O(n) when the circular buffer grows.
    void push(const T& value) {
        if (size_ == capacity_) {
            T copy(value);
            grow();
            Traits::construct(allocator_, data_ + ((head_ + size_) % capacity_),
                              std::move_if_noexcept(copy));
        } else {
            Traits::construct(allocator_, data_ + ((head_ + size_) % capacity_), value);
        }
        ++size_;
    }

    // O(1) amortized; O(n) when the circular buffer grows.
    void push(T&& value) {
        if (size_ == capacity_) {
            T moved(std::move(value));
            grow();
            Traits::construct(allocator_, data_ + ((head_ + size_) % capacity_),
                              std::move_if_noexcept(moved));
        } else {
            Traits::construct(allocator_, data_ + ((head_ + size_) % capacity_),
                              std::move(value));
        }
        ++size_;
    }

    // O(1)
    void pop() {
        if (empty()) throw std::out_of_range("Queue::pop on empty queue");
        Traits::destroy(allocator_, data_ + head_);
        head_ = (head_ + 1) % capacity_;
        --size_;
        if (size_ == 0) head_ = 0;
    }

    // O(1)
    T& front() {
        if (empty()) throw std::out_of_range("Queue::front on empty queue");
        return data_[head_];
    }

    // O(1)
    const T& front() const {
        if (empty()) throw std::out_of_range("Queue::front on empty queue");
        return data_[head_];
    }

    // O(1)
    bool empty() const noexcept { return size_ == 0; }

    // O(1)
    size_type size() const noexcept { return size_; }

    // O(n); retains allocated capacity for reuse.
    void clear() noexcept {
        destroy_elements();
        head_ = 0;
        size_ = 0;
    }

private:
    using Allocator = std::allocator<T>;
    using Traits = std::allocator_traits<Allocator>;

    Allocator allocator_;
    T* data_ = nullptr;
    size_type capacity_ = 0;
    size_type head_ = 0;
    size_type size_ = 0;

    void destroy_elements() noexcept {
        for (size_type i = 0; i < size_; ++i) {
            Traits::destroy(allocator_, data_ + ((head_ + i) % capacity_));
        }
        size_ = 0;
    }

    void release() noexcept {
        destroy_elements();
        if (data_ != nullptr) Traits::deallocate(allocator_, data_, capacity_);
        data_ = nullptr;
        capacity_ = 0;
        head_ = 0;
    }

    void grow() {
        const size_type max_size = Traits::max_size(allocator_);
        if (capacity_ == max_size) throw std::length_error("Queue capacity exceeded");
        const size_type new_capacity =
            capacity_ == 0 ? (max_size < 4 ? max_size : 4)
                           : (capacity_ > max_size / 2 ? max_size : capacity_ * 2);
        T* new_data = Traits::allocate(allocator_, new_capacity);
        size_type constructed = 0;
        try {
            for (; constructed < size_; ++constructed) {
                Traits::construct(allocator_, new_data + constructed,
                                  std::move_if_noexcept(data_[(head_ + constructed) % capacity_]));
            }
        } catch (...) {
            while (constructed > 0) Traits::destroy(allocator_, new_data + --constructed);
            Traits::deallocate(allocator_, new_data, new_capacity);
            throw;
        }

        destroy_elements();
        if (data_ != nullptr) Traits::deallocate(allocator_, data_, capacity_);
        data_ = new_data;
        capacity_ = new_capacity;
        head_ = 0;
        size_ = constructed;
    }

    void swap(Queue& other) noexcept {
        using std::swap;
        swap(data_, other.data_);
        swap(capacity_, other.capacity_);
        swap(head_, other.head_);
        swap(size_, other.size_);
    }
};

}  // namespace gridflow
