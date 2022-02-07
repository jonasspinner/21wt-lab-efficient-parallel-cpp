#ifndef EXERCISE5__ATOMIC_SHARED_PTR_LIST_H
#define EXERCISE5__ATOMIC_SHARED_PTR_LIST_H

#include <utility>
#include <memory>
#include <atomic>

#include "list_concept.h"

namespace epcpp {
    template<class T>
    class atomic_shared_ptr_list {
    private:
        struct node;

    public:
        using value_type = T;

        class handle;

        class node_manager {
        };

        atomic_shared_ptr_list() {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        explicit atomic_shared_ptr_list(node_manager &) : atomic_shared_ptr_list() {}

        template<class Value, class Compare = std::equal_to<>>
        std::pair<handle, bool> insert(Value &&value, Compare compare = Compare{});

        template<class Value, class Compare = std::equal_to<>>
        handle find(const Value &value, Compare compare = Compare{});

        template<class Value, class Compare = std::equal_to<>>
        bool erase(const Value &value, Compare compare = Compare{});

        handle end() const { return handle(); }

    private:
        using node_ptr = std::shared_ptr<node>;
        node_ptr m_head_atomic{};
    };


    template<class T>
    template<class Value, class Compare>
    auto atomic_shared_ptr_list<T>::insert(Value &&value, Compare compare) -> std::pair<handle, bool> {

        auto new_node = std::make_shared<node>(std::forward<Value>(value));

        node_ptr node;
        if (std::atomic_compare_exchange_weak(&m_head_atomic, &node, new_node)) {
            return {handle(std::move(new_node)), true};
        }

        while (true) {
            //std::cerr << node.get() << " " << node->value << " " << node->next.get() << std::endl;
            if (compare(node->value, new_node->value)) {
                // node->value() = std::move(new_node->value());
                return {handle(std::move(node)), false};
            }

            node_ptr expected = nullptr;
            if (std::atomic_compare_exchange_weak_explicit(&node->next_atomic, &expected, new_node,
                                                           std::memory_order_acquire, std::memory_order_relaxed)) {
                return {handle(std::move(new_node)), true};
            }
            node = std::move(expected);
        }
    }

    template<class T>
    template<class Value, class Compare>
    auto atomic_shared_ptr_list<T>::find(const Value &value, Compare compare) -> handle {

        auto node = std::atomic_load_explicit(&m_head_atomic, std::memory_order_acquire);

        while (node && !(compare(node->value, value))) {
            node = std::atomic_load_explicit(&node->next_atomic, std::memory_order_acquire);
        }

        return handle(std::move(node));
    }

    template<class T>
    template<class Value, class Compare>
    bool atomic_shared_ptr_list<T>::erase(const Value &value, Compare compare) {
        node_ptr *prev_node_next = &m_head_atomic;
        node_ptr node = std::atomic_load_explicit(prev_node_next, std::memory_order_acquire);
        while (node && !compare(node->value, value)) {
            prev_node_next = &node->next_atomic;
            assert(prev_node_next);
            node = std::atomic_load_explicit(prev_node_next, std::memory_order_acquire);
        }
        if (!node) return false;
        // TODO: might not be correct
        auto next = std::atomic_load_explicit(&node->next_atomic, std::memory_order_acquire);
        if (std::atomic_compare_exchange_weak(prev_node_next, &node, std::move(next))) {
            return true;
        }
        // prev_node->next only changes when node is removed
        // if the CAS fails, the node was removed by another thread
        return false;
    }


    template<class T>
    struct atomic_shared_ptr_list<T>::node {
        node(const node &) = delete;

        node(node &&) = delete;

        template<class Value>
        explicit node(Value &&value) : value(std::forward<Value>(value)) {}

        T value;
        std::shared_ptr<node> next_atomic;
    };

    template<class T>
    class atomic_shared_ptr_list<T>::handle {
    private:
        friend class atomic_shared_ptr_list;

    public:
        using value_type = T;
        using pointer = value_type *;
        using reference = value_type &;

        handle() = default;

        handle(const handle &other) : m_node(other.m_node) {};

        reference operator*() const noexcept {
            assert(m_node);
            return m_node->value;
        }

        pointer operator->() const noexcept {
            assert(m_node);
            return &m_node->value;
        }

        bool operator==(const handle &other) const { return m_node == other.m_node; }

        std::weak_ordering operator<=>(const handle &other) const { return m_node <=> other.m_node; };

    private:
        explicit handle(node_ptr node) : m_node(std::move(node)) {}

        node_ptr m_node;
    };

}

#endif //EXERCISE5__ATOMIC_SHARED_PTR_LIST_H
