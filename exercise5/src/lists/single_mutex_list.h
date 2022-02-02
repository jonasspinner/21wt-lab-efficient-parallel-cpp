#ifndef EXERCISE5_SINGLE_MUTEX_LIST_H
#define EXERCISE5_SINGLE_MUTEX_LIST_H

#include <memory>
#include <mutex>
#include <shared_mutex>

#include "list_concept.h"

namespace epcpp {
    template<class T>
    class single_mutex_list {
    private:
        struct node;

    public:
        using value_type = T;

        class handle;

        class node_manager {
        };

        single_mutex_list() : m_mutex(std::make_unique<std::shared_mutex>()) {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        explicit single_mutex_list(node_manager &) : single_mutex_list() {}

        template<class Key, class Compare = std::equal_to<>>
        handle find(const Key &key, Compare compare = Compare{}) const;

        std::pair<handle, bool> insert(value_type e);

        template<class Key, class Compare = std::equal_to<>>
        bool erase(const Key &key, Compare compare = Compare{});

        handle end() const { return handle(); }

    private:
        using node_ptr = std::shared_ptr<node>;

        std::unique_ptr<std::shared_mutex> m_mutex;
        node_ptr m_head{};
    };

    template<class T>
    struct single_mutex_list<T>::node {
        explicit node(value_type &&value) : value(std::move(value)), next() {}

        T value;
        std::shared_ptr<node> next;
    };

    template<class T>
    class single_mutex_list<T>::handle {
    private:
        using node_ptr = std::shared_ptr<node>;

        friend class single_mutex_list;

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


    template<class T>
    template<class Key, class Compare>
    auto single_mutex_list<T>::find(const Key &key, Compare compare) const -> handle {
        std::shared_lock lock(*m_mutex);

        auto node = m_head;

        while (node && !(compare(node->value, key))) {
            node = node->next;
        }
        return handle(std::move(node));
    }


    template<class T>
    auto single_mutex_list<T>::insert(value_type e) -> std::pair<handle, bool> {
        std::unique_lock lock(*m_mutex);
        auto node = m_head;
        if (node) {
            while (node->next) {
                if (node->value == e) {
                    node->value = std::move(e);
                    return {handle(std::move(node)), false};
                }
                node = node->next;
            }
            if (node->value == e) {
                node->value = std::move(e);
                return {handle(std::move(node)), false};
            }

            assert(!node->next);
            node->next = std::make_shared<single_mutex_list<T>::node>(std::move(e));
            return {handle(node->next), true};
        } else {
            assert(!m_head);
            m_head = std::make_shared<single_mutex_list<T>::node>(std::move(e));
            return {handle(m_head), true};
        }
    }

    template<class T>
    template<class Key, class Compare>
    bool single_mutex_list<T>::erase(const Key &key, Compare compare) {
        std::unique_lock lock(*m_mutex);

        node_ptr prev_node;
        auto node = m_head;

        while (node && !(compare(node->value(), key))) {
            prev_node = node;
            node = node->m_next();
        }
        if (node) {
            prev_node->m_next = node->m_next;
            return true;
        }
        return false;
    }
}

#endif //EXERCISE5_SINGLE_MUTEX_LIST_H
