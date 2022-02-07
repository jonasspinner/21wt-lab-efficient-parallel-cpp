#ifndef EXERCISE5_CONCURRENT_LIST_HPP
#define EXERCISE5_CONCURRENT_LIST_HPP

namespace epcpp::concepts {
    template<class L>
    concept List = requires(
            L list,
            const L const_list,
            typename L::value_type element,
            const typename L::value_type const_element,
            typename L::handle handle
    ) {
        { *handle } -> std::same_as<typename L::value_type &>;
        //{ list.insert(element) } -> std::same_as<std::pair<typename L::handle, bool>>;
        { list.find(const_element) } -> std::same_as<typename L::handle>;
        { list.erase(const_element) } -> std::same_as<bool>;
        { list.end() } -> std::same_as<typename L::handle>;
    };
}

#endif //EXERCISE5_CONCURRENT_LIST_HPP
