
#ifndef EXERCISE5_CONCURRENT_HASHMAP_H
#define EXERCISE5_CONCURRENT_HASHMAP_H

#include <optional>
#include <list>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <sstream>

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

    template<class Key, class T, class Hash, concepts::Bucket Bucket>
    class HashMap {
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

        using allocator_type = typename Bucket::allocator_type;

        using handle = typename Bucket::handle;
        using const_handle = typename Bucket::const_handle;

        HashMap(std::size_t capacity) : HashMap(capacity, allocator_type()) {}

        HashMap(std::size_t capacity, const allocator_type &alloc) : m_allocator(alloc), m_capacity(utils::next_power_of_two(capacity * 1.2)), m_mask(m_capacity - 1) {
            m_buckets = std::allocator<Bucket>{}.allocate(m_capacity);
            for (std::size_t i = 0; i < m_capacity; ++i) {
                std::construct_at(&m_buckets[i], m_allocator);
            }
            assert(utils::is_power_of_two(m_capacity));
        }

        ~HashMap() {
            for (std::size_t i = 0; i < m_capacity; ++i) {
                std::destroy_at(&m_buckets[i]);
            }
            std::allocator<Bucket>{}.deallocate(m_buckets, m_capacity);
            m_buckets = nullptr;
        }

        std::pair<handle, bool> insert(value_type &&value);

        std::pair<handle, bool> insert(const value_type &value) {
            value_type copy = value;
            return insert(std::move(copy));
        };

        handle find(const Key &key);

        bool erase(const Key &key);

        handle end() { return handle(); }

        static std::string name() {
            std::stringstream ss;
            ss << "hash_map<" << Bucket::name() << ">";
            return ss.str();
        }

    private:
        std::size_t index(std::size_t hash) const { return hash & m_mask; }

        allocator_type m_allocator;
        std::size_t m_capacity;
        std::size_t m_mask;
        Bucket* m_buckets;
    };


    template<class Key, class T, class Hash, concepts::Bucket Bucket>
    auto HashMap<Key, T, Hash, Bucket>::insert(value_type &&value) -> std::pair<handle, bool> {
        auto hash = InnerHash{}(value.first);
        return m_buckets[index(hash)].insert(std::move(value), hash);
    }

    template<class Key, class T, class Hash, concepts::Bucket Bucket>
    auto HashMap<Key, T, Hash, Bucket>::find(const Key &key) -> handle {
        auto hash = InnerHash{}(key);
        return m_buckets[index(hash)].find(key, hash);
    }

    template<class Key, class T, class Hash, concepts::Bucket Bucket>
    bool HashMap<Key, T, Hash, Bucket>::erase(const Key &key) {
        auto hash = InnerHash{}(key);
        return m_buckets[index(hash)].erase(key, hash);
    }

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<std::pair<const Key, T>>>
    using hash_map = epcpp::HashMap<Key, T, Hash, epcpp::ListBucket<Key, T, epcpp::atomic_marked_list, KeyEqual, Allocator, false>>;

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<std::pair<const Key, T>>>
    using hash_map_a = epcpp::HashMap<Key, T, Hash, epcpp::ListBucket<Key, T, epcpp::single_mutex_list, KeyEqual, Allocator, false>>;

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<std::pair<const Key, T>>>
    using hash_map_b = epcpp::HashMap<Key, T, Hash, epcpp::ListBucket<Key, T, epcpp::node_mutex_list, KeyEqual, Allocator, false>>;

    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<std::pair<const Key, T>>>
    using hash_map_c = epcpp::HashMap<Key, T, Hash, epcpp::ListBucket<Key, T, epcpp::atomic_marked_list, KeyEqual, Allocator, false>>;
}

#endif //EXERCISE5_CONCURRENT_HASHMAP_H
