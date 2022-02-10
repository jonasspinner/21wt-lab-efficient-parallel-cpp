#ifndef EXERCISE5_OTHER_HASH_MAPS_H
#define EXERCISE5_OTHER_HASH_MAPS_H

#include <unordered_map>
#include <mutex>
#include <shared_mutex>

#include "tbb/concurrent_unordered_map.h"

namespace epcpp {
    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<std::pair<const Key, T>>>
    class std_hash_map {
    private:
        using InnerMap = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;
    public:
        using key_type = typename InnerMap::key_type;
        using mapped_type = typename InnerMap::mapped_type;
        using value_type = typename InnerMap::value_type;

        using hasher = typename InnerMap::hasher;
        using key_equal = typename InnerMap::key_equal;
        using allocator_type = typename InnerMap::allocator_type;

        using handle = typename InnerMap::iterator;
        using const_handle = typename InnerMap::const_iterator;

        std_hash_map(std::size_t capacity) : m_inner_map(capacity) {}

        std::pair<handle, bool> insert(value_type &&value) {
            std::unique_lock lock(m_mutex);
            return m_inner_map.insert(std::move(value));
        }

        std::pair<handle, bool> insert(const value_type &value) {
            std::unique_lock lock(m_mutex);
            return m_inner_map.insert(value);
        }

        handle find(const key_type &key) {
            std::shared_lock lock(m_mutex);
            return m_inner_map.find(key);
        }

        bool erase(const key_type &key) {
            std::unique_lock lock(m_mutex);
            return m_inner_map.erase(key);
        }

        handle end() { return m_inner_map.end(); }

        static std::string_view name() {
            return "std::unordered_map";
        }

    private:
        mutable std::shared_mutex m_mutex;
        InnerMap m_inner_map;
    };

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<std::pair<const Key, T>>>
    class tbb_hash_map {
    private:
        using InnerMap = tbb::concurrent_unordered_map<Key, T, Hash, KeyEqual, Allocator>;
    public:
        using key_type = typename InnerMap::key_type;
        using mapped_type = typename InnerMap::mapped_type;
        using value_type = typename InnerMap::value_type;

        using hasher = typename InnerMap::hasher;
        using key_equal = typename InnerMap::key_equal;
        using allocator_type = typename InnerMap::allocator_type;

        using handle = typename InnerMap::iterator;
        using const_handle = typename InnerMap::const_iterator;

        tbb_hash_map(std::size_t capacity) : m_inner_map(capacity) {}

        std::pair<handle, bool> insert(value_type &&value) {
            return m_inner_map.insert(std::move(value));
        }

        std::pair<handle, bool> insert(const value_type &value) {
            return m_inner_map.insert(value);
        }

        handle find(const key_type &key) {
            return m_inner_map.find(key);
        }

        bool erase(const key_type &key) {
            throw std::runtime_error("erase not implemented");
            // return m_inner_map.erase(key);
            return false;
        }

        handle end() { return m_inner_map.end(); }

        static std::string_view name() {
            return "tbb::concurrent_unordered_map";
        }

    private:
        InnerMap m_inner_map;
    };
}

#endif //EXERCISE5_OTHER_HASH_MAPS_H
