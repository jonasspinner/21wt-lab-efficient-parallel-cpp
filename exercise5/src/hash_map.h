
#ifndef EXERCISE5_CONCURRENT_HASHMAP_H
#define EXERCISE5_CONCURRENT_HASHMAP_H

#include <optional>
#include <list>
#include <mutex>
#include <vector>

#include "bucket.h"
#include "lists/single_mutex_list.h"
#include "lists/node_mutex_list.h"
#include "lists/atomic_marked_list.h"
#include "utils.h"

namespace epcpp {
    namespace concepts {

        template<class H>
        concept HashMap = requires(
                H hash_map,
                typename H::key_type key,
                const typename H::key_type &const_key,
                typename H::value_type value,
                typename H::key_value_type key_value,
                typename H::key_equal key_equal,
                typename H::handle handle
        ) {
            { *handle } -> std::same_as<typename H::value_type &>;
            { hash_map.insert(std::move(key_value)) } -> std::same_as<std::pair<typename H::handle, bool>>;
            { hash_map.find(const_key) } -> std::same_as<typename H::handle>;
            { hash_map.erase(const_key) } -> std::same_as<bool>;
        };

    }

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>, class Bucket = ListBucket<Key, T, atomic_marked_list, KeyEqual>>
    class hash_map {
    private:
        struct InnerHash {
            std::size_t operator()(const Key &key) const {
                auto x = Hash{}(key);
                // murmurhash3
                // https://github.com/martinus/robin-hood-hashing/blob/master/src/include/robin_hood.h#L748-L759
                x ^= x >> 33U;
                x *= UINT64_C(0xff51afd7ed558ccd);
                x ^= x >> 33U;
                //x *= UINT64_C(0xc4ceb9fe1a85ec53);
                //x ^= x >> 33U;
                return static_cast<size_t>(x);
            }
        };

    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<const Key, T>;

        using handle = typename Bucket::handle;
        using const_handle = typename Bucket::const_handle;

        hash_map(std::size_t capacity) : m_buckets(utils::next_power_of_two(capacity * 1.2)),
                                         m_mask(m_buckets.size() - 1) {
            assert(utils::is_power_of_two(m_buckets.size()));
        }

        std::pair<handle, bool> insert(value_type &&value);

        std::pair<handle, bool> insert(const value_type &value) {
            value_type copy = value;
            return insert(std::move(copy));
        };

        handle find(const Key &key);

        bool erase(const Key &key);

        handle end() { return handle(); }

    private:
        std::size_t index(std::size_t hash) const { return hash & m_mask; }

        std::vector<Bucket> m_buckets;
        std::size_t m_mask;
    };


    template<class Key, class T, class Hash, class Equal, class Bucket>
    auto hash_map<Key, T, Hash, Equal, Bucket>::insert(value_type &&value) -> std::pair<handle, bool> {
        auto hash = InnerHash{}(value.first);
        return m_buckets[index(hash)].insert(std::move(value), hash);
    }

    template<class Key, class T, class Hash, class Equal, class Bucket>
    auto hash_map<Key, T, Hash, Equal, Bucket>::find(const Key &key) -> handle {
        auto hash = InnerHash{}(key);
        return m_buckets[index(hash)].find(key, hash);
    }

    template<class Key, class T, class Hash, class Equal, class Bucket>
    bool hash_map<Key, T, Hash, Equal, Bucket>::erase(const Key &key) {
        auto hash = InnerHash{}(key);
        return m_buckets[index(hash)].erase(key, hash);
    }

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>>
    using hash_map_a = epcpp::hash_map<Key, T, Hash, KeyEqual, epcpp::ListBucket<Key, T, epcpp::single_mutex_list, KeyEqual>>;

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>>
    using hash_map_b = epcpp::hash_map<Key, T, Hash, KeyEqual, epcpp::ListBucket<Key, T, epcpp::node_mutex_list, KeyEqual>>;

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>>
    using hash_map_c = epcpp::hash_map<Key, T, Hash, KeyEqual, epcpp::ListBucket<Key, T, epcpp::atomic_marked_list, KeyEqual>>;

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>, class Bucket = ListBucket<Key, T, atomic_marked_list, KeyEqual>>
    class hash_map_std {
    private:
        using InnerMap = std::unordered_map<Key, T, Hash, KeyEqual>;
    public:
        using value_type = typename InnerMap::value_type;
        using handle = typename InnerMap::iterator;
        using const_handle = typename InnerMap::const_iterator;

        hash_map_std(std::size_t capacity) : m_inner_map(capacity) {}

        std::pair<handle, bool> insert(value_type &&value) {
            std::unique_lock lock(m_mutex);
            return m_inner_map.insert(std::move(value));
        }

        std::pair<handle, bool> insert(const value_type &value) {
            std::unique_lock lock(m_mutex);
            return m_inner_map.insert(value);
        }

        handle find(const Key &key) {
            std::shared_lock lock(m_mutex);
            return m_inner_map.find(key);
        }

        bool erase(const Key &key) {
            std::unique_lock lock(m_mutex);
            return m_inner_map.erase(key);
        }

        handle end() { return m_inner_map.end(); }

    private:
        mutable std::shared_mutex m_mutex;
        InnerMap m_inner_map;
    };
}

#endif //EXERCISE5_CONCURRENT_HASHMAP_H
