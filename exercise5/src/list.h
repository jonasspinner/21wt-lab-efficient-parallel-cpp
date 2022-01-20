#ifndef EXERCISE5_CONCURRENT_LIST_HPP
#define EXERCISE5_CONCURRENT_LIST_HPP

#include <memory>
#include <atomic>

namespace epcpp {

    template<class L>
    concept List = requires(L list) {
        { list.insert() };
    };

    template<class T, class Equal>
    class concurrent_list {
    public:
        using value_type = T;
        using value_equal = Equal;
    private:
        struct Node {
            T m_element;
            std::shared_ptr<Node> m_next;
        };
        std::shared_ptr<Node> m_head{};
    public:
        void find(T e) {
            auto node_ptr = std::atomic_load(m_head);
            while (node_ptr) {
                if (Equal{}(node_ptr->m_element, e)) {
                    break;
                }
                node_ptr = std::atomic_load(node_ptr->m_next);
            }
        }
    };
}

#endif //EXERCISE5_CONCURRENT_LIST_HPP
