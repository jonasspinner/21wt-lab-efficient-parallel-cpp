
#ifndef EXERCISE5_LEAKING_ATOMIC_LIST_H
#define EXERCISE5_LEAKING_ATOMIC_LIST_H

#include <atomic>

#include "list_concept.h"

namespace epcpp {
    template<class T>
    class leaking_atomic_list {
    public:
        using value_type = T;

        class handle;

        class node_manager;

        leaking_atomic_list() {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        leaking_atomic_list(node_manager &) : leaking_atomic_list() {}

        template<class Key, class Compare = std::equal_to<>>
        handle find(const Key &key, Compare compare = Compare{}) const {
            node_ptr node = m_head.load(std::memory_order_acquire);
            while (node && !compare(node->value(), key)) {
                node = node->m_next.load(std::memory_order_acquire);
            }
            return handle(node);
        }

        handle end() { return handle(); }

        std::pair<handle, bool> insert(value_type value);

        template<class Key, class Compare = std::equal_to<>>
        bool erase(const Key &key, Compare compare = Compare{}) {
            auto result = internal_remove([&](const value_type &node_value) { return compare(node_value, key); });
            if (result.second)
                m_node_manager.reclaim_node(std::move(result.first));
            return result.second;
        }

    private:
        class node;

        using node_ptr = node *;

        template<class Compare = std::equal_to<value_type>>
        std::pair<node_ptr, bool> internal_insert(node_ptr new_node, Compare compare = Compare{});

        template<class Predicate>
        std::pair<node_ptr, bool> internal_remove(Predicate predicate);

        std::atomic<node *> m_head{nullptr};
        node_manager m_node_manager;
    };

    template<class T>
    class leaking_atomic_list<T>::node_manager {
    public:
        node_ptr create_node(value_type&& value) {
            return new node(std::move(value));
        }
        void destroy_node(node_ptr node) {
            delete node;
        }
        void reclaim_node(node_ptr node) {
            std::cout << "leaking node " << node << "\n";
        }
    };

    template<class T>
    auto leaking_atomic_list<T>::insert(value_type value) -> std::pair<handle, bool> {
        auto new_node = m_node_manager.create_node(std::move(value));
        auto result = internal_insert(new_node);
        if (!result.second) { m_node_manager.destroy_node(new_node); }
        return {handle(result.first), result.second};
    }


    template<class T>
    class leaking_atomic_list<T>::node {
    public:
        explicit node(const T &element) : m_element(element) {}

        explicit node(T &&element) : m_element(std::move(element)) {}

        const T &value() const { return m_element; }

        T &value() { return m_element; }

        node_ptr next() const { return m_next.load(std::memory_order_acquire); }

        bool try_set_next(node_ptr &expected, node_ptr new_next) {
            return m_next.compare_exchange_strong(expected, new_next);
        }

    private:
        node() = default;

        T m_element;
        std::atomic<node *> m_next{nullptr};

        friend class leaking_atomic_list;
    };


    template<class T>
    class leaking_atomic_list<T>::handle {
    public:
        using value_type = T;
        using pointer = value_type *;
        using reference = value_type &;

        handle() = default;

        handle(const handle &other) : m_node(other.m_node) {};

        handle(handle &&other) noexcept: m_node(std::move(other.m_node)) {};

        reference operator*() const noexcept {
            assert(m_node);
            return m_node->m_element;
        }

        pointer operator->() const noexcept {
            assert(m_node);
            return &m_node->m_element;
        }

        void reset() { m_node = nullptr; }

        [[nodiscard]] constexpr explicit operator bool() const { return (bool)m_node; }

        bool operator==(const handle &other) const { return m_node == other.m_node; }

        std::weak_ordering operator<=>(const handle &other) const { return m_node <=> other.m_node; };

    private:
        friend class leaking_atomic_list;

        explicit handle(node *node) : m_node(std::move(node)) {}

        node_ptr m_node;
    };

    template<class T>
    template<class Compare>
    auto
    leaking_atomic_list<T>::internal_insert(leaking_atomic_list::node_ptr new_node, Compare compare) -> std::pair<node_ptr, bool> {
        static_assert(std::is_invocable_r_v<bool, Compare, const value_type &, const value_type &>);
        node_ptr node = nullptr;
        if (m_head.compare_exchange_strong(node, new_node)) { return {new_node, true}; }

        assert(node);
        while (true) {
            if (compare(node->value(), new_node->value())) {
                // node->value() = std::move(new_node->value());
                return {node, false};
            }
            node_ptr expected = nullptr;
            if (node->try_set_next(expected, new_node)) { return {new_node, true}; }
            node = expected;
        }

    }

    template<class T>
    template<class Predicate>
    auto leaking_atomic_list<T>::internal_remove(Predicate predicate) -> std::pair<node_ptr, bool> {
        node_ptr prev_node = nullptr;
        node_ptr node = m_head.load(std::memory_order_acquire);
        while (node && !predicate(node->value())) {
            prev_node = node;
            node = node->m_next.load(std::memory_order_acquire);
        }
        if (!node) return {nullptr, false};
        if (!prev_node && m_head.compare_exchange_weak(node, node->next())) {
            return {node, true};
        }
        if (prev_node->try_set_next(node, node->next())) {
            return {node, true};
        }
        // prev_node->next only changes when node is removed
        // if the CAS fails, the node was removed by another thread
        return {nullptr, false};
    }
}

#endif //EXERCISE5_LEAKING_ATOMIC_LIST_H
