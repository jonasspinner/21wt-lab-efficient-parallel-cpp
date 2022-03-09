#ifndef EXERCISE5_SINGLE_MUTEX_LIST_H
#define EXERCISE5_SINGLE_MUTEX_LIST_H

#include <memory>
#include <mutex>
#include <shared_mutex>

#include "list_concept.h"

namespace epcpp {
    template<class T, class Allocator = std::allocator<T>>
    class single_mutex_list {
    private:
        struct Node;

        template<class ValueType>
        class Handle;

        using Compare = std::equal_to<>;
        constexpr static Compare compare{};

        using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    public:
        using value_type = T;

        using allocator_type = Allocator;

        using handle = Handle<T>;
        using const_handle = Handle<const T>;

        explicit single_mutex_list(const Allocator &alloc = Allocator()) : m_allocator(alloc) {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        template<class Value>
        std::pair<handle, bool> insert(Value &&value);

        template<class Value>
        [[nodiscard]] handle find(const Value &value);

        template<class Value>
        [[nodiscard]] const_handle find(const Value &value) const {
            return const_cast<single_mutex_list *>(this)->find(value, compare);
        }

        template<class Value>
        bool erase(const Value &value);

        [[nodiscard]] constexpr handle end() { return {}; }

        [[nodiscard]] constexpr const_handle cend() const { return {}; }

        [[nodiscard]] constexpr const_handle end() const { return {}; }

        [[nodiscard]] bool empty() const {
            std::shared_lock lock(m_mutex);
            return m_head == nullptr;
        }

        std::size_t size() const {
            std::size_t n = 0;
            std::shared_lock lock(m_mutex);

            for (auto node = m_head; node; node = node->next) {
                n++;
            }
            return n;
        }

        void clear() {
            std::unique_lock lock(m_mutex);
            m_head = nullptr;
        }

        [[nodiscard]] constexpr static std::string_view name() { return "single_mutex_list"; }

    private:
        using node_ptr = std::shared_ptr<Node>;

        mutable std::shared_mutex m_mutex;
        node_ptr m_head{};

        NodeAllocator m_allocator;
    };


    template<class T, class Allocator>
    template<class Value>
    auto single_mutex_list<T, Allocator>::insert(Value &&value) -> std::pair<handle, bool> {
        std::unique_lock lock(m_mutex);
        auto node = m_head;
        if (node) {
            while (node->next) {
                if (compare(node->value, value)) {
                    return {handle(std::move(node)), false};
                }
                node = node->next;
            }
            if (compare(node->value, value)) {
                return {handle(std::move(node)), false};
            }

            assert(!node->next);
            node->next = std::allocate_shared<Node>(m_allocator, std::forward<Value>(value));
            return {handle(node->next), true};
        } else {
            assert(!m_head);
            m_head = std::allocate_shared<Node>(m_allocator, std::forward<Value>(value));
            return {handle(m_head), true};
        }
    }


    template<class T, class Allocator>
    template<class Value>
    auto single_mutex_list<T, Allocator>::find(const Value &value) -> handle {
        std::shared_lock lock(m_mutex);

        for (auto node = m_head; node; node = node->next) {
            if (compare(node->value, value)) {
                return handle(std::move(node));
            }
        }
        return {};
    }


    template<class T, class Allocator>
    template<class Value>
    bool single_mutex_list<T, Allocator>::erase(const Value &value) {
        std::unique_lock lock(m_mutex);

        node_ptr prev_node;
        auto node = m_head;

        while (node && !compare(node->value, value)) {
            prev_node = node;
            node = node->next;
        }
        if (node) {
            if (prev_node) {
                prev_node->next = node->next;
            } else {
                m_head = node->next;
            }
            return true;
        }
        return false;
    }


    template<class T, class Allocator>
    struct single_mutex_list<T, Allocator>::Node {
        Node(const Node &) = delete;

        Node(Node &&) = delete;

        template<class ...Args>
        explicit Node(Args &&...args) : value(std::forward<Args>(args)...) {
            static_assert(std::is_constructible_v<T, Args...>);
        }

        T value;
        std::shared_ptr<Node> next;
    };

    template<class T, class Allocator>
    template<class ValueType>
    class single_mutex_list<T, Allocator>::Handle {
    private:
        friend class single_mutex_list;

        node_ptr m_node;

        explicit Handle(node_ptr node) : m_node(std::move(node)) {}

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

        bool operator==(const handle &other) const { return m_node == other.m_node; }

        std::weak_ordering operator<=>(const handle &other) const { return m_node <=> other.m_node; };

        operator Handle<const ValueType>() const { return {m_node}; }
    };
}

#endif //EXERCISE5_SINGLE_MUTEX_LIST_H
