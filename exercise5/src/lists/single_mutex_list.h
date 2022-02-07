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

        single_mutex_list() {
            static_assert(concepts::List<std::remove_reference_t<decltype(*this)>>);
        }

        explicit single_mutex_list(node_manager &) : single_mutex_list() {}

        template<class Value>
        std::pair<handle, bool> insert(Value &&value);

        template<class Value>
        handle find(const Value &value);

        template<class Value>
        const_handle find(const Value &value) const {
            return const_cast<single_mutex_list *>(this)->find(value, compare);
        }

        template<class Value>
        bool erase(const Value &value);

        handle end() { return {}; }

        const_handle cend() const { return {}; }

        const_handle end() const { return {}; }

    private:
        using node_ptr = std::shared_ptr<node>;

        mutable std::shared_mutex m_mutex;
        node_ptr m_head{};
    };


    template<class T>
    template<class Value>
    auto single_mutex_list<T>::insert(Value &&value) -> std::pair<handle, bool> {
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
            node->next = std::make_shared<single_mutex_list<T>::node>(std::forward<Value>(value));
            return {handle(node->next), true};
        } else {
            assert(!m_head);
            m_head = std::make_shared<single_mutex_list<T>::node>(std::forward<Value>(value));
            return {handle(m_head), true};
        }
    }


    template<class T>
    template<class Value>
    auto single_mutex_list<T>::find(const Value &value) -> handle {
        std::shared_lock lock(m_mutex);

        for (auto node = m_head; node; node = node->next) {
            if (compare(node->value, value)) {
                return handle(std::move(node));
            }
        }
        return {};
    }


    template<class T>
    template<class Value>
    bool single_mutex_list<T>::erase(const Value &value) {
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


    template<class T>
    struct single_mutex_list<T>::node {
        node(const node &) = delete;

        node(node &&) = delete;

        template<class Value>
        explicit node(Value &&value) : value(std::forward<Value>(value)) {}

        T value;
        std::shared_ptr<node> next;
    };

    template<class T>
    template<class ValueType>
    class single_mutex_list<T>::Handle {
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
