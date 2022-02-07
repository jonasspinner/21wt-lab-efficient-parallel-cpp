
#ifndef EXERCISE5__LEAKING_ATOMIC_LIST_H
#define EXERCISE5__LEAKING_ATOMIC_LIST_H

#include <atomic>

#include "list_concept.h"

namespace epcpp {
    template<class T>
    class leaking_atomic_list {
        struct node;

        using node_ptr = node *;

        template<class ValueType>
        class Handle;

    public:
        using value_type = T;

        using handle = Handle<T>;
        using const_handle = Handle<const T>;

        class node_manager;

        leaking_atomic_list() {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        leaking_atomic_list(node_manager &) : leaking_atomic_list() {}


        template<class Value, class Compare = std::equal_to<>>
        std::pair<handle, bool> insert(Value &&value, Compare compare = Compare{});

        template<class Value, class Compare = std::equal_to<>>
        handle find(const Value &key, Compare compare = Compare{});

        template<class Value, class Compare = std::equal_to<>>
        const_handle find(const Value &key, Compare compare = Compare{}) const {
            return const_cast<leaking_atomic_list *>(this)->find(key, compare);
        }

        template<class Value, class Compare = std::equal_to<>>
        bool erase(const Value &value, Compare compare = Compare{}) {
            auto result = internal_remove([&](const value_type &node_value) { return compare(node_value, value); });
            if (result.second)
                m_node_manager.reclaim_node(std::move(result.first));
            return result.second;
        }

        handle end() { return handle(); }

        const_handle cend() const { return const_handle(); }

        const_handle end() const { return cend(); }


    private:

        template<class Compare>
        std::pair<node_ptr, bool> internal_insert(node_ptr new_node, Compare compare = Compare{});

        template<class Predicate>
        std::pair<node_ptr, bool> internal_remove(Predicate predicate);

        std::atomic<node *> m_head{nullptr};
        node_manager m_node_manager;
    };


    template<class T>
    template<class Value, class Compare>
    auto leaking_atomic_list<T>::insert(Value &&value, Compare compare) -> std::pair<handle, bool> {
        auto new_node = m_node_manager.create_node(std::forward<Value>(value));
        auto result = internal_insert(new_node, compare);
        if (!result.second) { m_node_manager.destroy_node(new_node); }
        return {handle(result.first), result.second};
    }


    template<class T>
    template<class Compare>
    auto
    leaking_atomic_list<T>::internal_insert(leaking_atomic_list::node_ptr new_node,
                                            Compare compare) -> std::pair<node_ptr, bool> {
        static_assert(std::is_invocable_r_v<bool, Compare, const value_type &, const value_type &>);

        node_ptr node = nullptr;
        if (m_head.compare_exchange_strong(node, new_node)) { return {new_node, true}; }

        while (true) {
            assert(node);
            if (compare(node->value, new_node->value)) {
                return {node, false};
            }
            node_ptr expected = nullptr;
            if (node->next.compare_exchange_weak(expected, new_node, std::memory_order_acquire,
                                                 std::memory_order_relaxed)) { return {new_node, true}; }
            node = expected;
        }
    }

    template<class T>
    template<class Value, class Compare>
    auto leaking_atomic_list<T>::find(const Value &key, Compare compare) -> handle {
        node_ptr node = m_head.load(std::memory_order_acquire);
        while (node) {
            if (compare(node->value, key)) {
                return handle(node);
            }
            node = node->next.load(std::memory_order_acquire);
        }
        return {};
    }

    template<class T>
    template<class Predicate>
    auto leaking_atomic_list<T>::internal_remove(Predicate predicate) -> std::pair<node_ptr, bool> {
        node_ptr prev_node = nullptr;
        node_ptr node = m_head.load(std::memory_order_acquire);

        if (node) {
            if (predicate(node->value)) {
                auto next = node->next.load(std::memory_order_release);
                if (m_head.compare_exchange_weak(node, next)) {
                    return {node, true};
                } else {
                    assert(false);
                }
            }
            prev_node = node;
            node = node->next.load(std::memory_order_acquire);
        } else {
            return {nullptr, false};
        }


        while (node) {
            if (predicate(node->value)) {
                auto next = node->next.load(std::memory_order_release);
                if (prev_node->next.compare_exchange_weak(node, next)) {
                    return {node, true};
                } else {
                    assert(false);
                }
            }
            prev_node = node;
            node = node->next.load(std::memory_order_acquire);
        }
        return {nullptr, false};
    }


    template<class T>
    struct leaking_atomic_list<T>::node {
        node(const node &) = delete;

        node(node &&) = delete;

        template<class Value>
        explicit node(Value &&value) : value(std::forward<Value>(value)) {}

        T value;
        std::atomic<node *> next{nullptr};
    };


    template<class T>
    template<class ValueType>
    class leaking_atomic_list<T>::Handle {
    private:
        friend class leaking_atomic_list;

        node_ptr m_node;

        explicit Handle(node *node) : m_node(std::move(node)) {}

    public:
        using value_type = T;
        using pointer = value_type *;
        using reference = value_type &;

        Handle() = default;

        reference operator*() const noexcept {
            assert(m_node);
            return m_node->value;
        }

        pointer operator->() const noexcept {
            assert(m_node);
            return &m_node->value;
        }

        void reset() { m_node = nullptr; }

        [[nodiscard]] constexpr explicit operator bool() const { return (bool) m_node; }

        bool operator==(const Handle &other) const { return m_node == other.m_node; }

        std::weak_ordering operator<=>(const Handle &other) const { return m_node <=> other.m_node; };

        operator Handle<const ValueType>() { return {m_node}; }
    };

    template<class T>
    class leaking_atomic_list<T>::node_manager {
    public:
        template<class Value>
        node_ptr create_node(Value &&value) {
            return new node(std::forward<Value>(value));
        }

        void destroy_node(node_ptr node) {
            delete node;
        }

        void reclaim_node([[maybe_unused]] node_ptr node) {
            //std::cout << "leaking node " << node << "\n";
        }
    };
}

#endif //EXERCISE5__LEAKING_ATOMIC_LIST_H
