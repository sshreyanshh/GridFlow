#pragma once

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace gridflow {

template <typename T>
class LinkedList {
private:
    struct Node {
        template <typename U>
        explicit Node(U&& item) : value(std::forward<U>(item)) {}

        T value;
        Node* next = nullptr;
    };

public:
    using size_type = std::size_t;

    class const_iterator;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        // O(1)
        iterator() noexcept = default;

        // O(1)
        reference operator*() const { return node_->value; }

        // O(1)
        pointer operator->() const { return &node_->value; }

        // O(1)
        iterator& operator++() {
            node_ = node_->next;
            return *this;
        }

        // O(1)
        iterator operator++(int) {
            iterator previous(*this);
            ++(*this);
            return previous;
        }

        // O(1)
        bool operator==(const iterator& other) const noexcept { return node_ == other.node_; }

        // O(1)
        bool operator!=(const iterator& other) const noexcept { return !(*this == other); }

    private:
        explicit iterator(Node* node) noexcept : node_(node) {}
        Node* node_ = nullptr;
        friend class LinkedList;
        friend class const_iterator;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        // O(1)
        const_iterator() noexcept = default;

        // O(1)
        const_iterator(const iterator& other) noexcept : node_(other.node_) {}

        // O(1)
        reference operator*() const { return node_->value; }

        // O(1)
        pointer operator->() const { return &node_->value; }

        // O(1)
        const_iterator& operator++() {
            node_ = node_->next;
            return *this;
        }

        // O(1)
        const_iterator operator++(int) {
            const_iterator previous(*this);
            ++(*this);
            return previous;
        }

        // O(1)
        bool operator==(const const_iterator& other) const noexcept { return node_ == other.node_; }

        // O(1)
        bool operator!=(const const_iterator& other) const noexcept { return !(*this == other); }

    private:
        explicit const_iterator(const Node* node) noexcept : node_(node) {}
        const Node* node_ = nullptr;
        friend class LinkedList;
    };

    // O(1)
    LinkedList() noexcept = default;

    // O(n)
    LinkedList(const LinkedList& other) {
        try {
            for (const Node* node = other.head_; node != nullptr; node = node->next) {
                push_back(node->value);
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    // O(1)
    LinkedList(LinkedList&& other) noexcept
        : head_(std::exchange(other.head_, nullptr)),
          tail_(std::exchange(other.tail_, nullptr)),
          size_(std::exchange(other.size_, 0)) {}

    // O(n)
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            LinkedList copy(other);
            swap(copy);
        }
        return *this;
    }

    // O(n) in this list's current size; O(1) to take ownership.
    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = std::exchange(other.head_, nullptr);
            tail_ = std::exchange(other.tail_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    // O(n)
    ~LinkedList() { clear(); }

    // O(1)
    void push_back(const T& value) {
        Node* node = new Node(value);
        append(node);
    }

    // O(1)
    void push_back(T&& value) {
        Node* node = new Node(std::move(value));
        append(node);
    }

    // O(1)
    T& front() {
        if (head_ == nullptr) throw std::out_of_range("LinkedList::front on empty list");
        return head_->value;
    }

    // O(1)
    const T& front() const {
        if (head_ == nullptr) throw std::out_of_range("LinkedList::front on empty list");
        return head_->value;
    }

    // O(1)
    bool empty() const noexcept { return size_ == 0; }

    // O(1)
    size_type size() const noexcept { return size_; }

    // O(n)
    void clear() noexcept {
        while (head_ != nullptr) {
            Node* next = head_->next;
            delete head_;
            head_ = next;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    // O(1)
    iterator begin() noexcept { return iterator(head_); }

    // O(1)
    iterator end() noexcept { return iterator(nullptr); }

    // O(1)
    const_iterator begin() const noexcept { return const_iterator(head_); }

    // O(1)
    const_iterator end() const noexcept { return const_iterator(nullptr); }

    // O(1)
    const_iterator cbegin() const noexcept { return const_iterator(head_); }

    // O(1)
    const_iterator cend() const noexcept { return const_iterator(nullptr); }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    size_type size_ = 0;

    void append(Node* node) noexcept {
        if (tail_ == nullptr) {
            head_ = tail_ = node;
        } else {
            tail_->next = node;
            tail_ = node;
        }
        ++size_;
    }

    void swap(LinkedList& other) noexcept {
        using std::swap;
        swap(head_, other.head_);
        swap(tail_, other.tail_);
        swap(size_, other.size_);
    }
};

}  // namespace gridflow
