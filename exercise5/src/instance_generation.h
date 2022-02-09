#ifndef EXERCISE5_INSTANCE_GENERATION_H
#define EXERCISE5_INSTANCE_GENERATION_H

#include <vector>
#include <random>
#include <exception>
#include <iostream>

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


    std::vector<Operation<int>>
    generate_list_int_instances(std::size_t num_operations, float p_insert, float p_find, float p_remove) {
        if (std::abs(1.0f - (p_insert + p_find + p_remove)) > 1e-4f) {
            throw std::invalid_argument("");
        }

        std::mt19937_64 gen;
        std::geometric_distribution<int> key_dist(0.4);
        std::bernoulli_distribution find_dist(p_find);
        std::bernoulli_distribution insert_vs_remove_dist(p_insert / (p_insert + p_remove));

        std::vector<Operation<int>> result;
        result.reserve(num_operations);

        for (std::size_t i = 0; i < num_operations; ++i) {
            int key = key_dist(gen);
            if (find_dist(gen)) {
                result.push_back(Operation<int>{OperationKind::Find, key});
            } else if (insert_vs_remove_dist(gen)) {
                result.push_back(Operation<int>{OperationKind::Insert, key});
            } else {
                result.push_back(Operation<int>{OperationKind::Erase, key});
            }
        }
        return result;
    }


    std::vector<Operation<int>> generate_operations(std::size_t num_finds, std::size_t num_modifications) {
        std::mt19937_64 gen;
        std::geometric_distribution<int> key_dist(0.4);

        std::vector<Operation<int>> result;
        result.reserve(num_finds + num_modifications);
        for (std::size_t i = 0; i < num_finds; ++i) {
            int key = key_dist(gen);
            result.emplace_back(OperationKind::Find, key);
        }
        for (std::size_t i = 0; i < num_modifications / 2; ++i) {
            int key = key_dist(gen);
            result.emplace_back(OperationKind::Insert, key);
            result.emplace_back(OperationKind::Erase, key);
        }

        std::shuffle(result.begin(), result.end(), gen);

        return result;
    }

    class successful_find_benchmark {
    public:
        constexpr static std::string_view name() { return "successful_find"; }

        std::pair<std::vector<Operation<int>>, std::vector<Operation<int>>>
        generate(std::size_t num_elements, std::size_t num_queries) {
            std::vector<Operation<int>> setup;
            std::vector<Operation<int>> queries;

            for (std::size_t i = 0; i < num_elements; ++i) {
                int key = i;
                setup.emplace_back(OperationKind::Insert, key);
            }

            std::mt19937_64 gen;
            std::uniform_int_distribution<int> key_dist(0, num_elements - 1);

            for (std::size_t i = 0; i < num_queries; ++i) {
                int key = key_dist(gen);
                queries.emplace_back(OperationKind::Find, key);
            }

            std::shuffle(setup.begin(), setup.end(), gen);
            std::shuffle(queries.begin(), queries.end(), gen);

            return {setup, queries};
        }
    };

    class unsuccessful_find_benchmark {
    public:
        constexpr static std::string_view name() { return "unsuccessful_find"; }

        std::pair<std::vector<Operation<int>>, std::vector<Operation<int>>>
        generate(std::size_t num_elements, std::size_t num_queries) {
            std::vector<Operation<int>> setup;
            std::vector<Operation<int>> queries;

            for (std::size_t i = 0; i < num_elements; ++i) {
                int key = i;
                setup.emplace_back(OperationKind::Insert, key);
            }

            std::mt19937_64 gen;
            std::uniform_int_distribution<int> key_dist(num_elements, 2 * num_elements - 1);

            for (std::size_t i = 0; i < num_queries; ++i) {
                int key = key_dist(gen);
                queries.emplace_back(OperationKind::Find, key);
            }

            std::shuffle(setup.begin(), setup.end(), gen);
            std::shuffle(queries.begin(), queries.end(), gen);

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
