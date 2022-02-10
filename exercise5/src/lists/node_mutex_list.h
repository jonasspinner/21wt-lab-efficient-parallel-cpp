
#ifndef EXERCISE5_NODE_MUTEX_LIST_H
#define EXERCISE5_NODE_MUTEX_LIST_H

#include <memory>
#include <mutex>
#include <shared_mutex>


#include "list_concept.h"

namespace epcpp {
    template<class T, class Allocator = std::allocator<T>>
    class node_mutex_list {
    private:
        struct Node;

        template<class ValueType>
        class Handle;

        using Compare = std::equal_to<>;
        constexpr static Compare compare{};

    public:
        using value_type = T;

        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;

        using handle = Handle<T>;
        using const_handle = Handle<const T>;

        explicit node_mutex_list(const Allocator& alloc = Allocator()) : m_allocator(alloc) {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        template<class Value>
        std::pair<handle, bool> insert(Value &&value);

        template<class Value>
        [[nodiscard]] handle find(const Value &value);

        template<class Value>
        [[nodiscard]] const_handle find(const Value &value) const {
            return const_cast<node_mutex_list *>(this)->find(value);
        }

        template<class Value>
        bool erase(const Value &value);

        [[nodiscard]] constexpr handle end() { return {}; }

        [[nodiscard]] constexpr const_handle cend() const { return {}; }

        [[nodiscard]] constexpr const_handle end() const { return {}; }

        [[nodiscard]] bool empty() const {
            std::shared_lock lock(m_head_mutex);
            return m_head == nullptr;
        }

        std::size_t size() const {
            std::size_t n = 0;
            std::shared_ptr<Node> prev_node;

            m_head_mutex.lock_shared();
            auto node = m_head;

            if (node) {
                node->next_mutex.lock_shared();
                m_head_mutex.unlock_shared();

                prev_node = std::exchange(node, node->next);
                n++;
            } else {
                m_head_mutex.unlock_shared();
                return n;
            }

            while (node) {
                node->next_mutex.lock_shared();
                prev_node->next_mutex.unlock_shared();

                prev_node = std::exchange(node, node->next);
                n++;
            }

            assert(prev_node);
            prev_node->next_mutex.unlock_shared();

            return n;
        }

        [[nodiscard]] constexpr static std::string_view name() { return "node_mutex_list"; }

    private:
        mutable std::shared_mutex m_head_mutex;
        std::shared_ptr<Node> m_head;

        allocator_type m_allocator;
    };

    template<class T, class Allocator>
    template<class Value>
    auto node_mutex_list<T, Allocator>::insert(Value &&value) -> std::pair<handle, bool> {
        std::shared_ptr<Node> prev_node;

        m_head_mutex.lock();
        auto node = m_head;

        if (!node) {
            node = std::allocate_shared<node_mutex_list::Node>(m_allocator, std::forward<Value>(value));
            m_head = node;
            m_head_mutex.unlock();
            return {handle(std::move(node)), true};
        }

        if (compare(node->value, value)) {
            m_head_mutex.unlock();
            return {handle(std::move(node)), false};
        } else {
            node->next_mutex.lock();
            m_head_mutex.unlock();

            prev_node = std::move(node);
            node = prev_node->next;
        }

        while (node && !compare(node->value, value)) {
            node->next_mutex.lock();
            prev_node->next_mutex.unlock();

            prev_node = std::move(node);
            node = prev_node->next;
        }

        assert(prev_node);

        if (node) {
            assert(compare(node->value, value));
            prev_node->next_mutex.unlock();
            return {handle(std::move(node)), false};
        } else {
            assert(prev_node->value != value);
            assert(!prev_node->next);
            node = std::allocate_shared<node_mutex_list::Node>(m_allocator, std::forward<Value>(value));
            prev_node->next = node;
            prev_node->next_mutex.unlock();
            return {handle(std::move(node)), true};
        }
    }


    template<class T, class Allocator>
    template<class Value>
    auto node_mutex_list<T, Allocator>::find(const Value &value) -> handle {
        std::shared_ptr<Node> prev_node;

        m_head_mutex.lock_shared();
        auto node = m_head;

        if (node && !compare(node->value, value)) {
            node->next_mutex.lock_shared();
            m_head_mutex.unlock_shared();

            prev_node = std::exchange(node, node->next);
        } else {
            m_head_mutex.unlock_shared();
            return handle(std::move(node));
        }

        while (node && !compare(node->value, value)) {
            node->next_mutex.lock_shared();
            prev_node->next_mutex.unlock_shared();

            prev_node = std::exchange(node, node->next);
        }

        assert(prev_node);
        prev_node->next_mutex.unlock_shared();

        return handle(std::move(node));
    }


    template<class T, class Allocator>
    template<class Value>
    bool node_mutex_list<T, Allocator>::erase(const Value &value) {
        m_head_mutex.lock(); // (1)

        if (!m_head) {
            m_head_mutex.unlock(); // (1)
            return false;
        }

        auto node = m_head; // (1)
        if (compare(m_head->value, value)) {
            node->next_mutex.lock(); // (2)

            m_head = node->next; // (1) + (2)

            m_head_mutex.unlock(); // (1)
            node->next_mutex.unlock(); // (2)
            return true;
        }

        node->next_mutex.lock(); // (2')
        m_head_mutex.unlock(); // (1)
        auto prev_node = std::exchange(node, node->next);


        while (node && !compare(node->value, value)) {
            node->next_mutex.lock(); // (i)
            prev_node->next_mutex.unlock(); // (i-1)

            prev_node = std::exchange(node, node->next);
        }

        assert(prev_node);

        if (node) {
            node->next_mutex.lock();

            prev_node->next = node->next;

            prev_node->next_mutex.unlock();
            node->next_mutex.unlock();

            return true;
        } else {
            prev_node->next_mutex.unlock();
            return false;
        }
    }


    template<class T, class Allocator>
    struct node_mutex_list<T, Allocator>::Node {
        Node(const Node &) = delete;

        Node(Node &&) = delete;

        template<class ...Args>
        explicit Node(Args &&...args) : value(std::forward<Args>(args)...) {
            static_assert(std::is_constructible_v<T, Args...>);
        }

        T value;

        std::shared_mutex next_mutex;
        std::shared_ptr<Node> next;
    };

    template<class T, class Allocator>
    template<class ValueType>
    class node_mutex_list<T, Allocator>::Handle {
    private:
        using node_ptr = std::shared_ptr<Node>;

        friend class node_mutex_list;

    public:
        using value_type = T;
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

        [[nodiscard]] constexpr bool operator==(const handle &other) const { return m_node == other.m_node; }

        [[nodiscard]] constexpr std::weak_ordering operator<=>(const handle &other) const {
            return m_node <=> other.m_node;
        };

        operator Handle<const ValueType>() { return {m_node}; }

    private:
        constexpr explicit Handle(node_ptr node) noexcept: m_node(std::move(node)) {}

        node_ptr m_node;
    };
}

#endif //EXERCISE5_NODE_MUTEX_LIST_H
