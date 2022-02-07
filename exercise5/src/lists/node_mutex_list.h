
#ifndef EXERCISE5_NODE_MUTEX_LIST_H
#define EXERCISE5_NODE_MUTEX_LIST_H

#include <memory>
#include <mutex>
#include <shared_mutex>


#include "list_concept.h"

namespace epcpp {
    template<class T>
    class node_mutex_list {
    private:
        struct node;

        template<class ValueType>
        class Handle;

        using Compare = std::equal_to<>;
        constexpr static Compare compare{};

    public:
        using value_type = T;

        using handle = Handle<T>;
        using const_handle = Handle<const T>;

        class node_manager {
        };

        node_mutex_list() {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        explicit node_mutex_list(node_manager &) : node_mutex_list() {}

        template<class Value>
        std::pair<handle, bool> insert(Value &&value);

        template<class Value>
        handle find(const Value &value);

        template<class Value>
        const_handle find(const Value &value) const {
            return const_cast<node_mutex_list*>(this)->find(value);
        }

        template<class Value>
        bool erase(const Value &value);

        handle end() { return {}; }
        const_handle cend() const { return {}; }
        const_handle end() const { return {}; }

    private:
        mutable std::shared_mutex m_head_mutex;
        std::shared_ptr<node> m_head;
    };

    template<class T>
    template<class Value>
    auto node_mutex_list<T>::insert(Value &&value) -> std::pair<handle, bool> {
        std::shared_ptr<node> prev_node;

        m_head_mutex.lock();
        auto node = m_head;

        if (!node) {
            node = std::make_shared<node_mutex_list::node>(std::forward<Value>(value));
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
            node = std::make_shared<node_mutex_list::node>(std::forward<Value>(value));
            prev_node->next = node;
            prev_node->next_mutex.unlock();
            return {handle(std::move(node)), true};
        }
    }


    template<class T>
    template<class Value>
    auto node_mutex_list<T>::find(const Value &value) -> handle {
        std::shared_ptr<node> prev_node;

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


    template<class T>
    template<class Key>
    bool node_mutex_list<T>::erase(const Key &key) {
        m_head_mutex.lock(); // (1)

        if (!m_head) {
            m_head_mutex.unlock(); // (1)
            return false;
        }

        auto node = m_head; // (1)
        if (compare(m_head->value, key)) {
            node->next_mutex.lock(); // (2)

            m_head = node->next; // (1) + (2)

            m_head_mutex.unlock(); // (1)
            node->next_mutex.unlock(); // (2)
            return true;
        }

        node->next_mutex.lock(); // (2')
        m_head_mutex.unlock(); // (1)
        auto prev_node = std::exchange(node, node->next);


        while (node && !compare(node->value, key)) {
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


    template<class T>
    struct node_mutex_list<T>::node {
        node(const node &) = delete;

        node(node &&) = delete;

        template<class Value>
        explicit node(Value &&value) : value(std::forward<Value>(value)) {}

        T value;

        std::shared_mutex next_mutex;
        std::shared_ptr<node> next;

        friend class node_mutex_list;
    };

    template<class T>
    template<class ValueType>
    class node_mutex_list<T>::Handle {
    private:
        using node_ptr = std::shared_ptr<node>;

        friend class node_mutex_list;

    public:
        using value_type = T;
        using pointer = value_type *;
        using reference = value_type &;

        Handle() = default;

        Handle(const Handle &other) : m_node(other.m_node) {};

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

        operator Handle<const ValueType>() { return {m_node}; }

    private:
        explicit Handle(node_ptr node) : m_node(std::move(node)) {}

        node_ptr m_node;
    };
}

#endif //EXERCISE5_NODE_MUTEX_LIST_H
