
#ifndef EXERCISE5_ATOMIC_MARKED_LIST_H
#define EXERCISE5_ATOMIC_MARKED_LIST_H

#include <atomic>
#include <memory>

#include "list_concept.h"
#include "../marked_ptr.h"


namespace epcpp {
    template<class T, class Allocator = std::allocator<T>>
    class atomic_marked_list {
    private:
        using Compare = std::equal_to<>;
        constexpr static Compare compare{};
        static_assert(std::is_invocable_r_v<bool, Compare, const T&, const T&>);

        template<class InnerValue>
        struct Node;

        template<class ValueType>
        class Handle;

    public:
        using value_type = T;
        using allocator_type = Allocator;

        static_assert(std::is_same_v<typename Allocator::value_type, T>);

        using handle = Handle<T>;
        using const_handle = Handle<const T>;

        class node_manager;

        explicit atomic_marked_list(const Allocator &alloc = Allocator()) noexcept(noexcept(Allocator()))
                : m_node_manager(alloc) {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        ~atomic_marked_list();

        template<class Value>
        std::pair<handle, bool> insert(Value &&value);

        template<class Value>
        [[nodiscard]] handle find(const Value &value);

        template<class Value>
        [[nodiscard]] const_handle find(const Value &value) const {
            return const_cast<atomic_marked_list *>(this)->find(value);
        }

        template<class Value>
        bool erase(const Value &value);

        [[nodiscard]] constexpr handle end() { return {}; }

        [[nodiscard]] constexpr const_handle cend() const { return {}; }

        [[nodiscard]] constexpr const_handle end() const { return {}; }

        [[nodiscard]] constexpr bool empty() const { return m_head.load(std::memory_order_relaxed).get() == nullptr; }

        [[nodiscard]] std::size_t size() const {
            std::size_t n = 0;
            for (auto node = m_head.load(std::memory_order_acquire); node;) {
                auto next = node->next.load(std::memory_order_acquire);
                if (!next.is_marked()) {
                    n++;
                }
                node = next;
            }
            return n;
        }

        /**
         * UNSAFE: Deallocates all nodes.
         */
        void clear() {
            auto node = m_head.exchange(marked_ptr<Node<T>>{nullptr});
            for (; node;) {
                auto next = node->next.load(std::memory_order_relaxed);
                m_node_manager.destroy_node(node.get_unmarked());
                node = next;
            }
            m_node_manager = node_manager{};
        }

        [[nodiscard]] constexpr static std::string_view name() { return "atomic_marked_list"; }

    private:
        std::atomic<marked_ptr<Node<T>>> m_head{nullptr};
        node_manager m_node_manager;

        bool try_skip_node(std::atomic<marked_ptr<Node<T>>> *&prev_next_ptr, marked_ptr<Node<T>> &prev_next,
                           marked_ptr<Node<T>> node_to_skip, marked_ptr<Node<T>> next) {
            if (prev_next_ptr) {
                assert(!prev_next.is_marked());
                assert(next.is_marked());
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

    template<class T, class Allocator>
    template<class Value>
    auto atomic_marked_list<T, Allocator>::insert(Value &&value) -> std::pair<handle, bool> {
        static_assert(std::is_constructible_v<value_type, Value &&>);
        auto new_node = m_node_manager.create_node(std::forward<Value>(value));

        restart:

        auto prev_next_ptr = &m_head;
        marked_ptr<Node<T>> node;
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

        assert(!node);
        assert(prev_next.get_unmarked() == node.get_unmarked() || node.is_marked());

        if (prev_next_ptr && prev_next_ptr->compare_exchange_weak(prev_next, new_node)) {
            for (auto n = prev_next; n;) {
                auto next = n->next.load(std::memory_order_relaxed);
                assert(prev_next_ptr == &m_head || next.is_marked());
                m_node_manager.reclaim_node(n.get_unmarked());
                n = next;
            }
            return {handle(new_node), true};
        }
        goto restart;
    }

    template<class T, class Allocator>
    template<class Value>
    auto atomic_marked_list<T, Allocator>::find(const Value &value) -> handle {
        static_assert(std::is_invocable_r_v<bool, Compare, const value_type &, const Value &>);
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

    template<class T, class Allocator>
    template<class Value>
    bool atomic_marked_list<T, Allocator>::erase(const Value &value) {
        static_assert(std::is_invocable_r_v<bool, Compare, const value_type &, const Value &>);
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
                            try_skip_node(prev_next_ptr, prev_next, node, next.as_marked());
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


    template<class T, class Allocator>
    atomic_marked_list<T, Allocator>::~atomic_marked_list() {
        for (auto node = m_head.load(std::memory_order_relaxed); node;) {
            auto next = node->next.load(std::memory_order_relaxed);
            m_node_manager.destroy_node(node.get_unmarked());
            node = next;
        }
    }

    template<class T, class Allocator>
    class atomic_marked_list<T, Allocator>::node_manager {
    public:
        explicit node_manager(const Allocator &alloc) noexcept: m_allocator(alloc) {}

        template<class ...Args>
        [[nodiscard]] Node<T>* create_node(Args &&...args) {
            auto ptr = m_allocator.allocate(1);
            std::construct_at(ptr, std::forward<Args>(args)...);
            return ptr;
        }

        void destroy_node(Node<T>* node) {
            std::destroy_at(node);
            m_allocator.deallocate(node, 1);
        }

        void reclaim_node(Node<T>* node) {
            auto n = marked_ptr(new Node<Node<T>*>(node));
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
                destroy_node(n);
            }
        }

    private:
        using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node<T>>;

        std::atomic<marked_ptr<Node<Node<T> *>>> m_reclaimed;
        NodeAllocator m_allocator;
    };

    template<class T, class Allocator>
    template<class InnerValue>
    struct atomic_marked_list<T, Allocator>::Node {
        Node(const Node &) = delete;

        Node(Node &&) = delete;

        template<class ...Args>
        explicit Node(Args &&...args) : value(std::forward<Args>(args)...) {
            static_assert(std::is_constructible_v<InnerValue, Args...>);
        }

        InnerValue value;
        std::atomic<marked_ptr<Node>> next{nullptr};
    };


    template<class T, class Allocator>
    template<class ValueType>
    class atomic_marked_list<T, Allocator>::Handle {
    public:
        using value_type = ValueType;
        using pointer = value_type *;
        using reference = value_type &;

        constexpr Handle() noexcept = default;

        constexpr Handle(const Handle &other) noexcept: m_node(other.m_node) {};

        constexpr Handle(Handle &&other) noexcept: m_node(std::move(other.m_node)) {};

        [[nodiscard]] constexpr reference operator*() const noexcept {
            assert(m_node);
            return m_node->value;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept {
            assert(m_node);
            return &m_node->value;
        }

        constexpr void reset() { m_node = nullptr; }

        [[nodiscard]] constexpr explicit operator bool() const { return (bool) m_node; }

        [[nodiscard]] constexpr bool operator==(const Handle &other) const { return m_node == other.m_node; }

        [[nodiscard]] constexpr std::weak_ordering operator<=>(const Handle &other) const {
            return m_node <=> other.m_node;
        };

        operator Handle<const ValueType>() const { return {m_node}; }

    private:
        friend class atomic_marked_list;

        constexpr explicit Handle(Node<T> *node) noexcept: m_node(std::move(node)) {}

        Node<T> *m_node;
    };
}

#endif //EXERCISE5_ATOMIC_MARKED_LIST_H
