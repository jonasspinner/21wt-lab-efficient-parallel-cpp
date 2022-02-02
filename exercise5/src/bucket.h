#ifndef EXERCISE5_BUCKET_H
#define EXERCISE5_BUCKET_H


#include <atomic>
#include <cstdint>
#include <cassert>
#include <limits>
#include <utility>
#include <concepts>
#include <list>
#include <mutex>
#include <bitset>


#include "lists/list_concept.h"

namespace epcpp {
    namespace concepts {

        template<typename B>
        concept Bucket = requires(
                B b,
                typename B::key_type key,
                typename B::mapped_type mapped,
                typename B::value_type value,
                typename B::key_equal key_equal,
                std::size_t hash)
        {
            { key_equal(key, key) } -> std::same_as<bool>;
            { b.insert(std::move(value), hash) } -> std::same_as<std::pair<typename B::handle, bool>>;
            { b.find(key, hash) } -> std::same_as<typename B::handle>;
            { b.erase(key, hash) } -> std::same_as<bool>;
            { b.end() } -> std::equality_comparable_with<typename B::handle>;
        };

    }


    template<class Key, class Value, class KeyEqual = std::equal_to<Key>>
    class StdListBucket {
    public:
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using key_equal = KeyEqual;

        using handle = typename std::list<std::pair<Key, Value>>::iterator;

        StdListBucket() {
            static_assert(concepts::Bucket<std::remove_reference_t<decltype(*this)>>);
        }

        std::pair<handle, bool> insert(value_type &&key_value_pair, [[maybe_unused]] std::size_t hash) {
            std::scoped_lock lock(m_mutex);
            auto it = m_elements.begin();
            while (it != m_elements.end()) {
                if (key_equal{}(it->first, key_value_pair.first)) {
                    break;
                }
                ++it;
            }
            if (it != m_elements.end()) {
                *it = std::move(key_value_pair);
                return {it, false};
            }
            it = m_elements.insert(m_elements.end(), std::move(key_value_pair));
            return {it, true};
        }

        handle find(const key_type& key, [[maybe_unused]] std::size_t hash) {
            std::scoped_lock lock(m_mutex);
            return std::find(m_elements.begin(), m_elements.end(), [&](const auto &kv) { key_equal{}(kv.first, key); });
        }

        bool erase(const Key &key, [[maybe_unused]] std::size_t hash) {
            std::scoped_lock lock(m_mutex);
            auto it = m_elements.begin();
            while (it != m_elements.end()) {
                if (key_equal{}(it->first, key)) {
                    break;
                }
                ++it;
            }
            if (it != m_elements.end()) {
                m_elements.erase(it);
                return true;
            }
            return false;
        }

        handle end() {
            std::scoped_lock lock(m_mutex);
            return m_elements.end();
        }

    private:
        std::mutex m_mutex;
        std::list<std::pair<Key, Value>> m_elements;
    };

    template<class Key, class Value, template<class> class List, class KeyEqual = std::equal_to<Key>>
    class ListBucketT {
    public:
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using key_equal = KeyEqual;

        class handle;

        ListBucketT() {
            static_assert(concepts::Bucket<std::remove_reference_t<decltype(*this)>>);
        }

        std::pair<handle, bool> insert(value_type &&value, [[maybe_unused]] std::size_t hash) {
            return m_elements.insert(std::move(value));
        }

        handle find(const key_type &key, [[maybe_unused]] std::size_t hash) const {
            return m_elements.find(key);
        }

        handle erase(const Key &key, [[maybe_unused]] std::size_t hash) {
            return m_elements.erase(key);
        }

    private:
        using data = std::pair<const Key, Value>;
        static_assert(concepts::List<List<data>>);

        List<data> m_elements;
    };
}

#endif //EXERCISE5_BUCKET_H
