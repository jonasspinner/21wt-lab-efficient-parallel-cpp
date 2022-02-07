
#ifndef EXERCISE5_ATOMIC_MARKED_LIST_H
#define EXERCISE5_ATOMIC_MARKED_LIST_H

#include <atomic>

#include "list_concept.h"
#include "../marked_ptr.h"


namespace epcpp {
    template<class T>
    class atomic_marked_list {
    private:
        using Compare = std::equal_to<>;
        constexpr static Compare compare{};

        template<class ValueType>
        class Handle;

    public:
        using value_type = T;

        using handle = Handle<T>;
        using const_handle = Handle<const T>;

        class node_manager;

        atomic_marked_list() {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        atomic_marked_list(node_manager &) : atomic_marked_list() {}

        ~atomic_marked_list();

        template<class Value>
        std::pair<handle, bool> insert(Value &&value);

        template<class Value>
        handle find(const Value &value);

        template<class Value>
        const_handle find(const Value &value) const {
            return const_cast<atomic_marked_list*>(this)->find(value);
        }

        template<class Value>
        bool erase(const Value &value);

        handle end() { return {}; }

        const_handle cend() const { return {}; }

        const_handle end() const { return cend(); }

    private:
        template<class InnerValue>
        struct Node;
        using node = Node<T>;

        using node_ptr = node *;

        std::atomic<marked_ptr<node>> m_head{nullptr};
        node_manager m_node_manager;

        bool try_skip_node(std::atomic<marked_ptr<node>> *&prev_next_ptr, marked_ptr<node> &prev_next,
                           marked_ptr<node> node_to_skip, marked_ptr<node> next) {
            if (prev_next_ptr) {
                assert(!prev_next.is_marked());
                if (prev_next_ptr->compare_exchange_weak(prev_next, next.as_unmarked())) {
                    m_node_manager.reclaim_node(node_to_skip.get_unmarked());
                    return true;
                } else {
                    if (prev_next.is_marked()) {
                        prev_next_ptr = nullptr;
                        prev_next = nullptr;
                    }
                }
            }
            return false;
        }
    };

    template<class T>
    template<class Value>
    auto atomic_marked_list<T>::insert(Value &&value) -> std::pair<handle, bool> {
        node_ptr new_node = m_node_manager.create_node(std::forward<Value>(value));

        restart:

        auto prev_next_ptr = &m_head;
        marked_ptr<atomic_marked_list<T>::node> node;
        if (m_head.compare_exchange_weak(
                node, new_node,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            return {handle(new_node), true};
        }
        auto prev_next = node;

        assert(!prev_next.is_marked());

        while (node) {
            auto next = node->next.load(std::memory_order_acquire);
            if (next.is_marked()) {
                try_skip_node(prev_next_ptr, prev_next, node, next);
            } else {
                if (compare(node->value, value)) {
                    m_node_manager.destroy_node(new_node);
                    return {handle(node.get_unmarked()), false};
                }

                assert(!next.is_marked());
                prev_next_ptr = &node->next;
                prev_next = next;
            }
            node = next;
        }

        if (prev_next_ptr && prev_next_ptr->compare_exchange_weak(prev_next, new_node)) {
            for (auto n = prev_next; n; n = n->next.load(std::memory_order_relaxed)) {
                assert(n->next.load().is_marked());
                m_node_manager.reclaim_node(n.get_unmarked());
            }
            return {handle(new_node), true};
        }
        goto restart;
    }

    template<class T>
    template<class Value>
    auto atomic_marked_list<T>::find(const Value &value) -> handle {
        marked_ptr node = m_head.load(std::memory_order_acquire);
        while (node) {
            auto next = node->next.load(std::memory_order_acquire);
            if (!next.is_marked() && compare(node->value, value)) {
                return handle(node.get_unmarked()); // the pointer to the node might be marked
            }
            node = next;
        }
        return {};
    }

    template<class T>
    template<class Value>
    bool atomic_marked_list<T>::erase(const Value &value) {
        auto prev_next_ptr = &m_head;
        auto node = prev_next_ptr->load(std::memory_order_acquire);
        auto prev_next = node;

        assert(!prev_next.is_marked());

        while (node) {
            auto next = node->next.load(std::memory_order_acquire);
            if (next.is_marked()) {
                try_skip_node(prev_next_ptr, prev_next, node, next);
            } else {
                if (compare(node->value, value)) {
                    while (true) {
                        if (node->next.compare_exchange_weak(next, next.as_marked())) {
                            try_skip_node(prev_next_ptr, prev_next, node, next);
                            return true;
                        }
                        if (next.is_marked()) {
                            return false;
                        }
                    }
                }

                assert(!next.is_marked());
                prev_next_ptr = &node->next;
                prev_next = next;
            }
            node = next;
        }

        return false;
    }


    template<class T>
    atomic_marked_list<T>::~atomic_marked_list() {
        for (auto n = m_head.load(std::memory_order_relaxed); n;) {
            auto next = n->next.load(std::memory_order_relaxed);
            delete n.get();
            n = next;
        }
    }

    template<class T>
    class atomic_marked_list<T>::node_manager {
    public:
        template<class Value>
        node_ptr create_node(Value &&value) {
            return new node(std::forward<Value>(value));
        }

        void destroy_node(node_ptr node) {
            delete node;
        }

        void reclaim_node(node_ptr node) {
            auto n = marked_ptr(new Node<node_ptr>(node));
            n->next = m_reclaimed.load(std::memory_order_relaxed);
            while (true) {
                auto next = n->next.load(std::memory_order_relaxed);
                if (m_reclaimed.compare_exchange_weak(next, n, std::memory_order_relaxed)) {
                    return;
                }
                n->next = next;
            }
        }

        ~node_manager() {
            std::vector<Node<T> *> elements;

            for (auto n = m_reclaimed.load(std::memory_order_relaxed); n;) {
                auto next = n->next.load(std::memory_order_relaxed);
                elements.push_back(n->value);
                delete n.get();
                n = next;
            }

            std::sort(elements.begin(), elements.end());
            auto elements_end = std::unique(elements.begin(), elements.end());
            elements.erase(elements_end, elements.end());

            for (auto n: elements) {
                delete n;
            }
        }

    private:
        std::atomic<marked_ptr<Node<Node<T> *>>> m_reclaimed;
    };

    template<class T>
    template<class InnerValue>
    struct atomic_marked_list<T>::Node {
        Node(const Node &) = delete;

        Node(Node &&) = delete;

        template<class Value>
        explicit Node(Value &&value) : value(std::forward<Value>(value)) {}

        InnerValue value;
        std::atomic<marked_ptr<Node>> next{nullptr};
    };


    template<class T>
    template<class ValueType>
    class atomic_marked_list<T>::Handle {
    public:
        using value_type = ValueType;
        using pointer = value_type *;
        using reference = value_type &;

        Handle() = default;

        Handle(const Handle &other) : m_node(other.m_node) {};

        Handle(Handle &&other) noexcept: m_node(std::move(other.m_node)) {};

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

        operator Handle<const ValueType>() const { return {m_node}; }

    private:
        friend class atomic_marked_list;

        explicit Handle(node *node) : m_node(std::move(node)) {}

        node_ptr m_node;
    };
}

#endif //EXERCISE5_ATOMIC_MARKED_LIST_H
