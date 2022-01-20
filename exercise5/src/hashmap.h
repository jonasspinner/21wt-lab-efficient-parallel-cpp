
#ifndef EXERCISE5_CONCURRENT_HASHMAP_H
#define EXERCISE5_CONCURRENT_HASHMAP_H

#include <optional>
#include <list>
#include <mutex>

#include "bucket.h"


namespace epcpp {

    template<class H>
    concept HashMap = requires(
            H hash_map,
            typename H::key_type key,
            const typename H::key_type &const_key,
            typename H::value_type value,
            typename H::key_value_type key_value,
            typename H::key_equal key_equal
    ) {
        { hash_map.insert(std::move(key_value)) } -> std::convertible_to<bool>;
        { hash_map.remove(const_key) } -> std::convertible_to<bool>;
    };

    template<class Key, class Value, class Hash, class Equal, class Allocator, class Bucket = ListBucket<Key, Value, Equal>>
    class concurrent_hashmap {
    public:
        bool insert(std::pair<Key, Value> &&);

        bool insert(std::pair<Key, Value> &key_value_pair) {
            auto kv = key_value_pair;
            return insert(std::move(kv));
        }

        std::optional<Value> find(Key);

        bool remove(Key);

    private:
    };
}

#endif //EXERCISE5_CONCURRENT_HASHMAP_H
