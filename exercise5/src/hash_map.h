
#ifndef EXERCISE5_CONCURRENT_HASHMAP_H
#define EXERCISE5_CONCURRENT_HASHMAP_H

#include <optional>
#include <list>
#include <mutex>
#include <vector>

#include "bucket.h"
#include "lists/leaking_atomic_list.h"
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
            { *handle } -> std::same_as<typename H::value_type&>;
            { hash_map.find_or_insert(std::move(key_value)) } -> std::same_as<std::pair<typename H::handle, bool>>;
            { hash_map.find(const_key) } -> std::same_as<typename H::handle>;
            { hash_map.erase(const_key) } -> std::same_as<bool>;
        };

    }

    template<class Key, class Value, class Hash = std::hash<Key>, class Equal = std::equal_to<>>
    class hash_map {
    public:
        using value_type = std::pair<const Key, Value>;

        class handle;

        hash_map(std::size_t capacity) : m_buckets(utils::next_power_of_two(capacity * 1.2)), m_mask(m_buckets.size() - 1) {
            assert(utils::is_power_of_two(m_buckets.size()));
        }

        std::pair<handle, bool> find_or_insert(Key key, Value value);

        handle find(const Key &key);

        bool erase(const Key &key);

        handle end() { return handle(); }

    private:
        std::size_t index(std::size_t hash) const { return hash & m_mask; }

        struct data;
        using list = leaking_atomic_list<data>;

        std::vector<list> m_buckets;
        std::size_t m_mask;
    };

    template<class Key, class Value, class Hash, class Equal>
    class hash_map<Key, Value, Hash, Equal>::handle {
    public:
        handle() = default;

        value_type *operator->() const { return &m_handle->value; }

        value_type &operator*() const { return m_handle->value; }

        void reset() { m_handle.reset(); }

        [[nodiscard]] constexpr explicit operator bool() const { return (bool)m_handle; }

        [[nodiscard]] constexpr bool operator==(const handle &other) const { return m_handle == other.m_handle; }
        [[nodiscard]] constexpr bool operator!=(const handle &other) const { return !(*this == other); }

    private:
        friend class hash_map;
        using list_handle = typename list::handle;

        handle(list_handle &&handle) : m_handle(std::move(handle)) {}

        list_handle m_handle;
    };

    template<class Key, class Value, class Hash, class Equal>
    struct hash_map<Key, Value, Hash, Equal>::data {
        /*
        data(std::size_t hash, const value_type & value) : hash(hash), value(value) {}
        data(std::size_t hash, value_type&& value) : hash(hash), value(std::move(value)) {}
        data(const data&) = default;
        data(data&&) noexcept = default;
        void operator=(data&& other) {
            hash = other.hash;
            value = std::move(other.value);
        }
         */
        std::size_t hash;
        value_type value;

        [[nodiscard]] constexpr bool operator==(const data &other) const {
            return hash == other.hash && Equal{}(value.first, other.value.first);
        }
        [[nodiscard]] constexpr bool operator==(const Key& key) const {
            return Equal{}(value.first, key);
        }
    };

    template<class Key, class Value, class Hash, class Equal>
    auto hash_map<Key, Value, Hash, Equal>::find_or_insert(Key key, Value value) -> std::pair<handle, bool> {
        auto hash = Hash{}(key);
        auto result = m_buckets[index(hash)].insert(data{hash, std::make_pair(std::move(key), std::move(value))});
        return {handle(std::move(result.first)), result.second};
    }

    template<class Key, class Value, class Hash, class Equal>
    auto hash_map<Key, Value, Hash, Equal>::find(const Key &key) -> handle {
        auto hash = Hash{}(key);
        auto result = m_buckets[index(hash)].find(key);
        return handle(std::move(result));
    }

    template<class Key, class Value, class Hash, class Equal>
    bool hash_map<Key, Value, Hash, Equal>::erase(const Key &key) {
        auto hash = Hash{}(key);
        auto result = m_buckets[index(hash)].erase(key);
        return result;
    }

}

#endif //EXERCISE5_CONCURRENT_HASHMAP_H
