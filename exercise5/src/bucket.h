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


    template<class Key, class Value, template<class> class List, class KeyEqual = std::equal_to<Key>>
    class ListBucket {
    private:
        struct Data;

        using InnerList = List<Data>;
        static_assert(concepts::List<InnerList>);

        template<class ValueType>
        class Handle;

    public:
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using key_equal = KeyEqual;

        using handle = Handle<mapped_type>;
        using const_handle = Handle<const mapped_type>;

        ListBucket() {
            static_assert(concepts::Bucket<std::remove_reference_t<decltype(*this)>>);
        }

        std::pair<handle, bool> insert(value_type &&value, [[maybe_unused]] std::size_t hash) {
            auto result = m_elements.insert(Data{hash, std::move(value)});
            return std::make_pair(handle(std::move(result.first)), result.second);
        }

        handle find(const key_type &key, [[maybe_unused]] std::size_t hash) {
            auto result = m_elements.find(key);
            return handle(std::move(result));
        }

        const_handle find(const key_type &key, [[maybe_unused]] std::size_t hash) const {
            auto result = m_elements.find(key);
            return const_handle(std::move(result));
        }

        bool erase(const Key &key, [[maybe_unused]] std::size_t hash) {
            return m_elements.erase(key);
        }

        handle end() { return handle(m_elements.end()); }

    private:
        InnerList m_elements;
    };

    template<class Key, class Value, template<class> class List, class KeyEqual>
    template<class ValueType>
    class ListBucket<Key, Value, List, KeyEqual>::Handle {
    private:
        using InnerHandle = std::conditional_t<std::is_const_v<ValueType>, typename InnerList::const_handle, typename InnerList::handle>;
        friend ListBucket;
    public:
        Handle() = default;

        value_type *operator->() const { return &m_inner_handle->value; }

        value_type &operator*() const { return m_inner_handle->value; }

        void reset() { m_inner_handle.reset(); }

        [[nodiscard]] constexpr explicit operator bool() const { return (bool) m_inner_handle; }

        [[nodiscard]] constexpr bool operator==(const Handle &other) const { return m_inner_handle == other.m_inner_handle; }

        operator Handle<const value_type>() { return {m_inner_handle}; }

    private:
        explicit Handle(InnerHandle &&handle) : m_inner_handle(std::move(handle)) {}

        InnerHandle m_inner_handle;
    };

    template<class Key, class Value, template<class> class List, class KeyEqual>
    struct ListBucket<Key, Value, List, KeyEqual>::Data {
        std::size_t hash;
        value_type value;

        [[nodiscard]] constexpr bool operator==(const Data &other) const {
            return hash == other.hash && KeyEqual{}(value.first, other.value.first);
        }

        template<class OtherType>
        [[nodiscard]] constexpr bool operator==(const OtherType &other) const {
            return KeyEqual{}(value.first, other);
        }
    };
}

#endif //EXERCISE5_BUCKET_H
