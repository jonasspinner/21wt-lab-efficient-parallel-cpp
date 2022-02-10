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


    template<class Key, class T, template<class, class> class List, class KeyEqual, class Allocator, bool EnableDataWithHash>
    class ListBucket {
    private:
        struct DataWithHash;
        struct DataWithoutHash;
        using Data = std::conditional_t<EnableDataWithHash, DataWithHash, DataWithoutHash>;

        using DataAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Data>;
        using InnerList = List<Data, DataAllocator>;
        static_assert(concepts::List<InnerList>);

        template<class ValueType>
        class Handle;

    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<const Key, T>;
        using key_equal = KeyEqual;

        using allocator_type = Allocator;
        static_assert(std::is_same_v<typename std::allocator_traits<Allocator>::value_type, value_type>);

        using handle = Handle<mapped_type>;
        using const_handle = Handle<const mapped_type>;

        ListBucket() {
            static_assert(concepts::Bucket<std::remove_reference_t<decltype(*this)>>);
        }

        explicit ListBucket(const Allocator &alloc) : m_inner_list(DataAllocator(alloc)) {
            static_assert(concepts::Bucket<std::remove_reference_t<decltype(*this)>>);
        }

        /**
         * WARNING: Not thread-safe.
         */
        ListBucket &operator=(ListBucket &&other) noexcept {
            if (&other == this) return *this;
            m_inner_list = std::move(other.m_inner_list);
        }

        std::pair<handle, bool> insert(value_type &&value, [[maybe_unused]] std::size_t hash) {
            auto result = m_inner_list.insert(Data{hash, std::move(value)});
            return std::make_pair(handle(std::move(result.first)), result.second);
        }

        handle find(const key_type &key, [[maybe_unused]] std::size_t hash) {
            auto result = m_inner_list.find(key);
            return handle(std::move(result));
        }

        const_handle find(const key_type &key, [[maybe_unused]] std::size_t hash) const {
            auto result = m_inner_list.find(key);
            return const_handle(std::move(result));
        }

        bool erase(const Key &key, [[maybe_unused]] std::size_t hash) {
            return m_inner_list.erase(key);
        }

        handle end() { return handle(m_inner_list.end()); }

        [[nodiscard]] bool empty() const {
            return m_inner_list.empty();
        }

        [[nodiscard]] std::size_t size() const {
            return m_inner_list.size();
        }

        static std::string name() {
            std::stringstream ss;
            ss << InnerList::name();
            return ss.str();
        }

    private:
        InnerList m_inner_list;
    };

    template<class Key, class Value, template<class, class> class List, class KeyEqual, class Allocator, bool EnableDataWithHash>
    template<class ValueType>
    class ListBucket<Key, Value, List, KeyEqual, Allocator, EnableDataWithHash>::Handle {
    private:
        using InnerHandle = std::conditional_t<std::is_const_v<ValueType>, typename InnerList::const_handle, typename InnerList::handle>;
        friend ListBucket;
    public:
        Handle() = default;

        value_type *operator->() const { return &m_inner_handle->value; }

        value_type &operator*() const { return m_inner_handle->value; }

        void reset() { m_inner_handle.reset(); }

        [[nodiscard]] constexpr explicit operator bool() const { return (bool) m_inner_handle; }

        [[nodiscard]] constexpr bool operator==(const Handle &other) const {
            return m_inner_handle == other.m_inner_handle;
        }

        operator Handle<const value_type>() { return {m_inner_handle}; }

    private:
        explicit Handle(InnerHandle &&handle) : m_inner_handle(std::move(handle)) {}

        InnerHandle m_inner_handle;
    };

    template<class Key, class Value, template<class, class> class List, class KeyEqual, class Allocator, bool EnableDataWithHash>
    struct ListBucket<Key, Value, List, KeyEqual, Allocator, EnableDataWithHash>::DataWithHash {
        explicit DataWithHash(std::size_t hash, value_type &&value) : hash(hash), value(std::move(value)) {}

        std::size_t hash;
        value_type value;

        [[nodiscard]] constexpr bool operator==(const DataWithHash &other) const {
            return hash == other.hash && KeyEqual{}(value.first, other.value.first);
        }

        template<class OtherType>
        [[nodiscard]] constexpr bool operator==(const OtherType &other) const {
            return KeyEqual{}(value.first, other);
        }
    };


    template<class Key, class Value, template<class, class> class List, class KeyEqual, class Allocator, bool EnableDataWithHash>
    struct ListBucket<Key, Value, List, KeyEqual, Allocator, EnableDataWithHash>::DataWithoutHash {
        explicit DataWithoutHash(std::size_t, value_type &&value) : value(std::move(value)) {}

        value_type value;

        [[nodiscard]] constexpr bool operator==(const DataWithoutHash &other) const {
            return KeyEqual{}(value.first, other.value.first);
        }

        template<class OtherType>
        [[nodiscard]] constexpr bool operator==(const OtherType &other) const {
            return KeyEqual{}(value.first, other);
        }
    };
}

#endif //EXERCISE5_BUCKET_H
