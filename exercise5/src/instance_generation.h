#ifndef EXERCISE5_INSTANCE_GENERATION_H
#define EXERCISE5_INSTANCE_GENERATION_H

#include <vector>
#include <random>
#include <exception>
#include <iostream>
#include <sstream>

namespace epcpp {
    enum class OperationKind {
        Insert,
        Find,
        Erase
    };

    template<class Value>
    struct Operation {
        Operation(OperationKind kind, Value value) : kind(kind), value(value) {}

        OperationKind kind;
        Value value;
    };

    class successful_find_benchmark {
    public:
        constexpr static std::string_view name() { return "successful_find"; }

        std::pair<std::vector<Operation<int>>, std::vector<Operation<int>>>
        generate(std::size_t num_elements, std::size_t num_queries, std::size_t seed) {
            std::mt19937_64 gen(seed);

            std::vector<int> keys_in_map(num_elements);
            std::vector<Operation<int>> setup;
            std::vector<Operation<int>> queries;

            std::uniform_int_distribution<int> key_dist(0, std::numeric_limits<int>::max());
            std::uniform_int_distribution<int> index_dist(0, num_elements - 1);

            for (auto &key: keys_in_map) {
                key = key_dist(gen);
                setup.emplace_back(OperationKind::Insert, key);
            }

            std::shuffle(setup.begin(), setup.end(), gen);

            for (std::size_t i = 0; i < num_queries; ++i) {
                queries.emplace_back(OperationKind::Find, keys_in_map[index_dist(gen)]);
            }

            return {setup, queries};
        }
    };

    class unsuccessful_find_benchmark {
    public:
        constexpr static std::string_view name() { return "unsuccessful_find"; }

        std::pair<std::vector<Operation<int>>, std::vector<Operation<int>>>
        generate(std::size_t num_elements, std::size_t num_queries, std::size_t seed) {
            std::mt19937_64 gen(seed);

            std::vector<int> keys_in_map(num_elements);
            std::vector<Operation<int>> setup;
            std::vector<Operation<int>> queries;

            std::uniform_int_distribution<int> key_dist(0, std::numeric_limits<int>::max());
            std::uniform_int_distribution<int> query_dist(std::numeric_limits<int>::min(), -1);

            for (auto &key: keys_in_map) {
                key = key_dist(gen);
                setup.emplace_back(OperationKind::Insert, key);
            }

            std::shuffle(setup.begin(), setup.end(), gen);

            for (std::size_t i = 0; i < num_queries; ++i) {
                queries.emplace_back(OperationKind::Find, query_dist(gen));
            }

            return {setup, queries};
        }
    };

    class find_benchmark {
        float successful_find_probability = 1.0;
    public:
        find_benchmark(float successful_find_probability) : successful_find_probability(successful_find_probability) {}

        std::string name() {
            std::stringstream ss;
            ss << "find<p=" << successful_find_probability << ">";
            return ss.str();
        }

        std::pair<std::vector<Operation<int>>, std::vector<Operation<int>>>
        generate(std::size_t num_elements, std::size_t num_queries, std::size_t seed) {
            std::mt19937_64 gen(seed);

            std::vector<int> keys_in_map(num_elements);
            std::vector<Operation<int>> setup;
            std::vector<Operation<int>> queries;

            std::uniform_int_distribution<int> key_dist(0, std::numeric_limits<int>::max());
            std::uniform_int_distribution<int> fail_key_dist(std::numeric_limits<int>::min(), -1);
            std::uniform_int_distribution<int> index_dist(0, num_elements - 1);
            std::bernoulli_distribution success_dist(successful_find_probability);

            for (auto &key: keys_in_map) {
                key = key_dist(gen);
                setup.emplace_back(OperationKind::Insert, key);
            }

            std::shuffle(setup.begin(), setup.end(), gen);

            for (std::size_t i = 0; i < num_queries; ++i) {
                bool successful_find = !((successful_find_probability == 0.0) ||
                                         ((successful_find_probability != 1.0) && !success_dist(gen)));
                int key = successful_find ? keys_in_map[index_dist(gen)] : fail_key_dist(gen);
                queries.emplace_back(OperationKind::Find, key);
            }

            return {setup, queries};
        }
    };

    class find_and_modifiy_benchmark {
        float successful_find_probability = 1.0;
        float modification_probability = 0.0;
    public:
        find_and_modifiy_benchmark(float successful_find_probability, float modification_probability)
                : successful_find_probability(successful_find_probability),
                  modification_probability(modification_probability) {}

        std::string name() {
            std::stringstream ss;
            ss << "find_and_modifiy<p=" << successful_find_probability << ",q=" << modification_probability << ">";
            return ss.str();
        }

        std::pair<std::vector<Operation<int>>, std::vector<Operation<int>>>
        generate(std::size_t num_elements, std::size_t num_queries, std::size_t seed) {
            std::mt19937_64 gen(seed);

            std::vector<int> keys_in_map(num_elements);
            std::vector<Operation<int>> setup;
            std::vector<Operation<int>> queries;

            std::uniform_int_distribution<int> key_dist(0, std::numeric_limits<int>::max());
            std::uniform_int_distribution<int> fail_key_dist(std::numeric_limits<int>::min(), -1);
            std::uniform_int_distribution<int> index_dist(0, num_elements - 1);
            std::bernoulli_distribution is_success_dist(successful_find_probability);
            std::bernoulli_distribution is_modification_dist(modification_probability);

            for (auto &key: keys_in_map) {
                key = key_dist(gen);
                setup.emplace_back(OperationKind::Insert, key);
            }

            std::shuffle(setup.begin(), setup.end(), gen);

            for (std::size_t i = 0; i < num_queries; ++i) {
                bool successful_find = !((successful_find_probability == 0.0) ||
                                         ((successful_find_probability != 1.0) && !is_success_dist(gen)));
                int key = successful_find ? keys_in_map[index_dist(gen)] : fail_key_dist(gen);
                if (i + 1 < num_queries && ((modification_probability == 1.0) ||
                                            ((modification_probability != 0.0) && is_modification_dist(gen)))) {
                    if (successful_find) {
                        queries.emplace_back(OperationKind::Erase, key);
                        queries.emplace_back(OperationKind::Insert, key);
                    } else {
                        queries.emplace_back(OperationKind::Insert, key);
                        queries.emplace_back(OperationKind::Erase, key);
                    }
                    ++i;
                } else {
                    queries.emplace_back(OperationKind::Find, key);
                }
            }

            return {setup, queries};
        }
    };


    class skewed_find_benchmark {
        float geometric_dist_param = 1.0;
    public:
        skewed_find_benchmark(float geometric_dist_param) : geometric_dist_param(geometric_dist_param) {}

        std::string name() {
            std::stringstream ss;
            ss << "skewed_find<p=" << geometric_dist_param << ">";
            return ss.str();
        }

        std::pair<std::vector<Operation<int>>, std::vector<Operation<int>>>
        generate(std::size_t num_elements, std::size_t num_queries, std::size_t seed) {
            std::mt19937_64 gen(seed);

            std::vector<int> keys_in_map(num_elements);
            std::vector<Operation<int>> setup;
            std::vector<Operation<int>> queries;

            std::geometric_distribution<int> key_dist(geometric_dist_param);
            std::uniform_int_distribution<int> index_dist(0, num_elements - 1);

            for (auto &key: keys_in_map) {
                key = key_dist(gen);
                setup.emplace_back(OperationKind::Insert, key);
            }

            std::shuffle(setup.begin(), setup.end(), gen);


            for (std::size_t i = 0; i < num_queries; ++i) {
                queries.emplace_back(OperationKind::Find, keys_in_map[index_dist(gen)]);
            }

            return {setup, queries};
        }
    };
}

std::ostream &operator<<(std::ostream &os, epcpp::OperationKind kind) {
    using epcpp::OperationKind;
    if (kind == OperationKind::Find) {
        os << "Find";
    } else if (kind == OperationKind::Insert) {
        os << "Insert";
    } else if (kind == OperationKind::Erase) {
        os << "Erase";
    } else {
        os.setstate(std::ios_base::failbit);
    }
    return os;
}

std::istream &operator>>(std::istream &is, epcpp::OperationKind &kind) {
    using epcpp::OperationKind;
    std::string s;
    is >> s;
    if (s == "Find") {
        kind = OperationKind::Find;
    } else if (s == "Insert") {
        kind = OperationKind::Insert;
    } else if (s == "Erase") {
        kind = OperationKind::Erase;
    } else {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

template<class Value>
std::ostream &operator<<(std::ostream &os, epcpp::Operation<Value> operation) {
    return os << operation.kind << " " << operation.value;
}

template<class Value>
std::istream &operator>>(std::istream &is, epcpp::Operation<Value> &operation) {
    return is >> operation.kind >> operation.value;
}

#endif //EXERCISE5_INSTANCE_GENERATION_H
