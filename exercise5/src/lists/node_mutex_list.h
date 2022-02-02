
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
        class Node;

    public:
        using value_type = T;

        class handle;

        class node_manager {};

        node_mutex_list() : m_head_mutex(std::make_unique<std::shared_mutex>()) {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        explicit node_mutex_list(node_manager&) : node_mutex_list() {}

        template<class Key, class Compare = std::equal_to<>>
        handle find(const Key& key, Compare compare = Compare{}) const {
            std::shared_ptr<Node> prev_node;

            m_head_mutex->lock_shared();
            auto node = m_head;

            if (node && !compare(node->value(), key)) {
                node->m_next_mutex.lock_shared();
                m_head_mutex->unlock_shared();

                prev_node = std::exchange(node, node->m_next);
            } else {
                m_head_mutex->unlock_shared();
                return handle(std::move(node));
            }

            while (node && !compare(node->value(), key)) {
                node->m_next_mutex.lock_shared();
                prev_node->m_next_mutex.unlock_shared();

                prev_node = std::exchange(node, node->m_next);
            }

            assert(prev_node);
            prev_node->m_next_mutex.unlock_shared();

            return handle(std::move(node));
        }

        handle end() const { return handle(); }

        std::pair<handle, bool> insert(value_type value);

        template<class Key, class Compare = std::equal_to<>>
        bool erase(const Key& key, Compare compare = Compare{}) {
            static_assert(std::is_invocable_r_v<bool, Compare, const value_type&, const Key&>);

            std::shared_ptr<Node> prev_node;

            m_head_mutex->lock_shared();
            auto node = m_head;

            if (node && !compare(node->value(), key)) {
                node->m_next_mutex.lock_shared();
                m_head_mutex->unlock_shared();

                prev_node = std::exchange(node, node->m_next);
            } else {
                m_head = m_head->m_next;
                m_head_mutex->unlock_shared();
                return true;
            }

            while (node && !compare(node->value()), key) {
                node->m_next_mutex.lock_shared();
                prev_node->m_next_mutex.unlock_shared();

                prev_node = std::exchange(node, node->m_next);
            }

            assert(prev_node);

            if (node) {
                prev_node->m_next = node->m_next;
                prev_node->m_next_mutex.unlock_shared();
                return true;
            } else {
                prev_node->m_next_mutex.unlock_shared();
                return false;
            }
        }

    private:
        std::unique_ptr<std::shared_mutex> m_head_mutex;
        std::shared_ptr<Node> m_head;
    };

    template<class T>
    std::pair<typename node_mutex_list<T>::handle, bool> node_mutex_list<T>::insert(value_type value) {
        std::shared_ptr<Node> prev_node;

        m_head_mutex->lock();
        auto node = m_head;

        if (node && node->value() != value) {
            node->m_next_mutex.lock();
            m_head_mutex->unlock();

            prev_node = std::exchange(node, node->m_next);
        } else {
            if (node) {
                node->m_element = std::move(value);
                m_head_mutex->unlock();
                return {handle(std::move(node)), false};
            } else {
                m_head = std::make_shared<Node>(std::move(value));;
                m_head_mutex->unlock();
                return {handle(m_head), true};
            }
        }

        while (node && node->value() != value) {
            node->m_next_mutex.lock();
            prev_node->m_next_mutex.unlock();

            prev_node = std::exchange(node, node->m_next);
        }

        assert(prev_node);

        if (node) {
            node->m_element = std::move(value);
            prev_node->m_next_mutex.unlock();
            return {handle(node), false};
        } else {
            node = std::make_shared<Node>(std::move(value));
            prev_node->m_next = node;
            prev_node->m_next_mutex.unlock();
            return {handle(std::move(node)), true};
        }
    }


    template<class T>
    class node_mutex_list<T>::Node {
    public:
        explicit Node(const T &element) : m_element(element) {}

        explicit Node(T &&element) : m_element(std::move(element)) {}

        const T &value() const { return m_element; }

    private:
        T m_element;

        std::shared_mutex m_next_mutex;
        std::shared_ptr<Node> m_next;

        friend class node_mutex_list;
    };

    template<class T>
    class node_mutex_list<T>::handle {
    private:
        using node_ptr = std::shared_ptr<Node>;

        friend class node_mutex_list;

        template<bool C>
        friend
        class Handle;

    public:
        using value_type = T;
        using pointer = value_type *;
        using reference = value_type &;

        handle() = default;

        handle(const handle &other) : m_node(other.m_node) {};

        reference operator*() const noexcept {
            assert(m_node);
            return m_node->m_element;
        }

        pointer operator->() const noexcept {
            assert(m_node);
            return &m_node->m_element;
        }

        bool operator==(const handle &other) const { return m_node == other.m_node; }

        std::weak_ordering operator<=>(const handle &other) const { return m_node <=> other.m_node; };

    private:
        explicit handle(node_ptr node) : m_node(std::move(node)) {}

        node_ptr m_node;
    };
}

#endif //EXERCISE5_NODE_MUTEX_LIST_H
