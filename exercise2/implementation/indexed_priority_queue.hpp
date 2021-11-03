#pragma once

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/**
 * A priority queue that is addressable by an index, i.e., a number
 * from 0 to a user-specified maximum.
 * K is the key type, which must be an integral type.
 * V is the value type, which must be default constructible and movable.
 * Comp is the comparison operator.
 *
 * NOTE: This is a max-heap, i.e., top() is the largest element in the queue.
 */
template <class K, class V, class Comp = std::less<>>
class IndexedPriorityQueue {
 public:
    /**
     * (Default) constructor.
     * capacity is the maximum number of keys that can be inserted.
     */
    IndexedPriorityQueue(std::size_t capacity = 0, Comp comp = {})
            : index_(capacity), comp_(std::move(comp)) {}

    /**
     * Returns the maximum number of keys that can be inserted.
     */
    std::size_t capacity() const { return index_.size(); }

    /**
     * Increases the maximum number of keys that can be inserted.
     */
    void reserve(std::size_t capacity) {
        if (capacity > index_.size()) index_.resize(capacity);
    }

    /**
     * Returns true if the queue contains no elements.
     */
    bool empty() const { return heap_.size() == 1; }

    /**
     * Returns the number of elements in the queue.
     */
    std::size_t size() const { return heap_.size() - 1; }


    /**
     * Returns the largest element in the queue.
     * The queue must not be empty.
     */
    const std::pair<K, V>& top() const { return heap_[1]; }

    /**
     * Returns true if the given key exists in the queue.
     */
    bool hasKey(K key) const { return index_[key] != 0; }

    /**
     * Returns the priority for the given key.
     * The key must exist in the queue.
     */
    const V& priority(K key) const { return heap_[index_[key]].second; }


    /**
     * Inserts a new key with the given value into the queue.
     * The key must not already exist in the queue.
     */
    void push(K key, V value) {
        heap_.emplace_back();
        siftUp(heap_.size() - 1, key, std::move(value));
    }

    /**
     * Changes the value for the given key.
     * The key must exist in the queue.
     */
    void changePriority(K key, V new_value) {
        if (comp_(new_value, priority(key)))
            siftDown(index_[key], key, std::move(new_value));
        else
            siftUp(index_[key], key, std::move(new_value));
    }

    /**
     * Inserts the key if it does not yet exist, or changes its priority if it does.
     */
    void pushOrChangePriority(K key, V new_value) {
        if (hasKey(key))
            changePriority(key, std::move(new_value));
        else
            push(key, std::move(new_value));
    }

    /**
     * Removes the largest element from the queue.
     * The queue must not be empty.
     */
    std::pair<K, V> pop() {
        auto r = std::move(heap_[1]);
        siftDown(1, heap_.back().first, std::move(heap_.back().second));
        index_[r.first] = 0;
        heap_.pop_back();
        return r;
    }

    /**
     * Clears all elements from the queue.
     */
    void clear() {
        heap_.resize(1);
        std::fill(index_.begin(), index_.end(), 0);
    }

 private:
    // the heap, indexing starts at 1, index 0 is unused
    std::vector<std::pair<K, V>> heap_{1};
    // mapping from keys to indices in the heap
    std::vector<K> index_;
    Comp comp_;

    // performs sift-up of (key, value) starting at index i
    void siftUp(std::size_t i, K key, V&& value) {
        std::size_t parent = i / 2;

        // move hole upwards until correct position found
        while (parent > 0 && comp_(heap_[parent].second, value)) {
            heap_[i] = std::move(heap_[parent]);
            index_[heap_[i].first] = i;
            i = parent;
            parent = i / 2;
        }

        heap_[i] = {key, std::move(value)};
        index_[key] = i;
    }

    // performs sift-down of (key, value), starting at index 1
    void siftDown(std::size_t i, K key, V&& value) {
        std::size_t child = i * 2;

        const auto end = heap_.size() - 1;
        while (child < end) {
            // choose larger child
            if (comp_(heap_[child].second, heap_[child + 1].second)) ++child;

            // stop if correct position found
            if (!comp_(value, heap_[child].second)) break;

            // move hole downwards
            heap_[i] = std::move(heap_[child]);
            index_[heap_[i].first] = i;

            i = child;
            child = 2 * i;
        }

        // When sifting down the last element, the above loop is correct for all edge
        // cases (i.e., if no early break from the loop).
        // With r the root, l the last element, s its sibling, and p its parent:
        // - r is l           =>  loop won't execute, i points to r, and
        //                        the following is a no-op move (r = std::move(l))
        // - s does not exist =>  i points to p, and the following is p = std::move(l)
        // - l <= s           =>  i points to s, and the following is s = std::move(l)
        // - l >  s           =>  early break, i points to p, and the following is p = std::move(l)

        heap_[i] = {key, std::move(value)};
        index_[key] = i;
    }
};
