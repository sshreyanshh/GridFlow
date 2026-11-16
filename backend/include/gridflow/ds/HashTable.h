#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace gridflow {

// Hash functor customisation point. Only the string version is provided;
// other key types need their own functor passed as the third template argument.
template <typename K>
struct DefaultHash;

template <>
struct DefaultHash<std::string> {
    std::size_t operator()(const std::string& text) const {
        std::uint64_t h = 14695981039346656037ULL;  // FNV offset basis
        for (unsigned char c : text) {
            h ^= c;
            h *= 1099511628211ULL;                  // FNV prime
        }
        return static_cast<std::size_t>(h);
    }
};

template <typename K, typename V, typename Hash = DefaultHash<K>>
class HashTable {
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v, Node* n) : key(k), value(v), next(n) {}
    };

public:
    explicit HashTable(std::size_t initialBuckets = 16, Hash hash = Hash())
        : hash_(hash),
          buckets_(initialBuckets == 0 ? 1 : initialBuckets, nullptr),
          size_(0) {}

    // Deep copy: every node is duplicated, so the two tables are independent.
    HashTable(const HashTable& other)
        : hash_(other.hash_), buckets_(other.buckets_.size(), nullptr), size_(0) {
        try {
            for (std::size_t b = 0; b < other.buckets_.size(); ++b) {
                Node** tail = &buckets_[b];  // address of the pointer to fill next
                for (const Node* n = other.buckets_[b]; n != nullptr; n = n->next) {
                    *tail = new Node(n->key, n->value, nullptr);
                    tail = &(*tail)->next;
                    ++size_;
                }
            }
        } catch (...) {
            clear();  // a constructor that throws never runs the destructor, so free here
            throw;
        }
    }

    // Copy-and-swap: take the argument BY VALUE (this makes the copy), then swap
    // it with *this. The old contents leave with the temporary and are destroyed.
    // Safe for self-assignment and leaves *this unchanged if the copy throws.
    HashTable& operator=(HashTable other) {
        swap(other);
        return *this;
    }

    ~HashTable() { clear(); }

    void swap(HashTable& other) {
        std::swap(hash_, other.hash_);
        buckets_.swap(other.buckets_);
        std::swap(size_, other.size_);
    }

    // Inserts the key, or overwrites its value if it already exists.
    // Returns true if a NEW key was inserted, false if an existing one was overwritten.
    bool put(const K& key, const V& value) {
        if (Node* existing = findNode(key)) {
            existing->value = value;
            return false;
        }
        if (needsGrow()) rehash(buckets_.size() * 2);
        const std::size_t idx = indexFor(key);
        buckets_[idx] = new Node(key, value, buckets_[idx]);  // insert at the head of the chain
        ++size_;
        return true;
    }

    // Returns a pointer to the stored value, or nullptr if the key is absent.
    V* get(const K& key) {
        Node* n = findNode(key);
        return n != nullptr ? &n->value : nullptr;
    }
    const V* get(const K& key) const {
        const Node* n = findNode(key);
        return n != nullptr ? &n->value : nullptr;
    }

    bool contains(const K& key) const { return findNode(key) != nullptr; }

    // Removes the key. Returns true if it existed.
    bool remove(const K& key) {
        const std::size_t idx = indexFor(key);
        Node* prev = nullptr;
        Node* cur = buckets_[idx];
        while (cur != nullptr) {
            if (cur->key == key) {
                if (prev != nullptr) {
                    prev->next = cur->next;      // unlink from the middle or end
                } else {
                    buckets_[idx] = cur->next;   // unlink the head
                }
                delete cur;
                --size_;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    // Deletes every entry. The bucket count is kept.
    void clear() {
        for (Node*& head : buckets_) {
            while (head != nullptr) {
                Node* next = head->next;
                delete head;
                head = next;
            }
        }
        size_ = 0;
    }

    // Calls fn(key, value) once for every entry, in no particular order.
    template <typename F>
    void forEach(F fn) const {
        for (const Node* head : buckets_) {
            for (const Node* n = head; n != nullptr; n = n->next) {
                fn(n->key, n->value);
            }
        }
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    std::size_t bucketCount() const { return buckets_.size(); }
    double loadFactor() const {
        return static_cast<double>(size_) / static_cast<double>(buckets_.size());
    }

    // Length of the longest chain: a quality measure for the hash function.
    std::size_t maxChainLength() const {
        std::size_t longest = 0;
        for (const Node* head : buckets_) {
            std::size_t length = 0;
            for (const Node* n = head; n != nullptr; n = n->next) ++length;
            if (length > longest) longest = length;
        }
        return longest;
    }

private:
    std::size_t indexFor(const K& key) const { return hash_(key) % buckets_.size(); }

    // True if inserting one more entry would push the load factor above 0.75.
    // Integer arithmetic: (size+1)/buckets > 3/4  <=>  (size+1)*4 > buckets*3
    bool needsGrow() const { return (size_ + 1) * 4 > buckets_.size() * 3; }

    Node* findNode(const K& key) const {
        for (Node* n = buckets_[indexFor(key)]; n != nullptr; n = n->next) {
            if (n->key == key) return n;
        }
        return nullptr;
    }

    // Moves every existing node into a bigger bucket array. No node is allocated
    // or copied, only relinked. The only step that can throw (allocating `fresh`)
    // happens before anything is modified.
    void rehash(std::size_t newCount) {
        std::vector<Node*> fresh(newCount, nullptr);
        for (Node* head : buckets_) {
            Node* n = head;
            while (n != nullptr) {
                Node* next = n->next;  // remember it: relinking overwrites n->next
                const std::size_t idx = hash_(n->key) % newCount;
                n->next = fresh[idx];
                fresh[idx] = n;
                n = next;
            }
        }
        buckets_.swap(fresh);
    }

    Hash hash_;
    std::vector<Node*> buckets_;
    std::size_t size_;
};

}  // namespace gridflow