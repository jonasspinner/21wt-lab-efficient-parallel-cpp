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

namespace epcpp {
    namespace concepts {

        template<typename B>
        concept Bucket = requires(
                B b,
                typename B::key_type key,
                typename B::value_type value,
                typename B::key_value_type key_value,
                typename B::key_equal key_equal,
                std::size_t hash)
        {
            { key_equal(key, key) } -> std::convertible_to<bool>;
            { b.insert(std::move(key_value), hash) } -> std::convertible_to<bool>;
            { b.remove(key, hash) } -> std::convertible_to<bool>;
        };

    }


    template<class Key, class Value, class KeyEqual = std::equal_to<Key>>
    class ListBucket {
    public:
        using key_type = Key;
        using value_type = Value;
        using key_value_type = std::pair<const Key, Value>;
        using key_equal = KeyEqual;

        ListBucket() {
            static_assert(concepts::Bucket<std::remove_reference_t<decltype(*this)>>);
        }

        bool insert(key_value_type &&key_value_pair, [[maybe_unused]] std::size_t hash) {
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
                return false;
            }
            m_elements.insert(m_elements.end(), std::move(key_value_pair));
            return true;
        }

        bool remove(const Key &key, [[maybe_unused]] std::size_t hash) {
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

    private:
        std::mutex m_mutex;
        std::list<std::pair<Key, Value>> m_elements;
    };
}

#endif //EXERCISE5_BUCKET_H
